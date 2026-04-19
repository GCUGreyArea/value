#include "Parser.hpp"
#include "ParserDriver.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace
{

    std::string trimCopy(const std::string &input)
    {
        const size_t first = input.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            return "";
        }

        const size_t last = input.find_last_not_of(" \t\r\n");
        return input.substr(first, last - first + 1);
    }

    bool isBoolean(const std::string &str, bool &result)
    {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower == "true")
        {
            result = true;
            return true;
        }

        if (lower == "false")
        {
            result = false;
            return true;
        }

        return false;
    }

    bool parseFixedWidthNumber(const std::string &input,
                               size_t offset,
                               size_t width,
                               int &result)
    {
        if (offset + width > input.size())
        {
            return false;
        }

        int value = 0;
        for (size_t index = 0; index < width; ++index)
        {
            const unsigned char ch = static_cast<unsigned char>(input[offset + index]);
            if (!std::isdigit(ch))
            {
                return false;
            }

            value = (value * 10) + static_cast<int>(ch - '0');
        }

        result = value;
        return true;
    }

    bool isLeapYear(int year)
    {
        return (year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0));
    }

    bool isValidDateTimeUtc(const std::string &value, std::string &errorMessage)
    {
        if (value.size() != 20 || value[4] != '-' || value[7] != '-' ||
            value[10] != 'T' || value[13] != ':' || value[16] != ':' ||
            value[19] != 'Z')
        {
            errorMessage = "expected format YYYY-MM-DDTHH:MM:SSZ";
            return false;
        }

        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        if (!parseFixedWidthNumber(value, 0, 4, year) ||
            !parseFixedWidthNumber(value, 5, 2, month) ||
            !parseFixedWidthNumber(value, 8, 2, day) ||
            !parseFixedWidthNumber(value, 11, 2, hour) ||
            !parseFixedWidthNumber(value, 14, 2, minute) ||
            !parseFixedWidthNumber(value, 17, 2, second))
        {
            errorMessage = "expected numeric components in YYYY-MM-DDTHH:MM:SSZ";
            return false;
        }

        if (month < 1 || month > 12)
        {
            errorMessage = "month must be between 01 and 12";
            return false;
        }

        static const int daysPerMonth[] = {31, 28, 31, 30, 31, 30,
                                           31, 31, 30, 31, 30, 31};
        int maxDay = daysPerMonth[month - 1];
        if (month == 2 && isLeapYear(year))
        {
            maxDay = 29;
        }

        if (day < 1 || day > maxDay)
        {
            errorMessage = "day is out of range for the month";
            return false;
        }

        if (hour < 0 || hour > 23)
        {
            errorMessage = "hour must be between 00 and 23";
            return false;
        }

        if (minute < 0 || minute > 59)
        {
            errorMessage = "minute must be between 00 and 59";
            return false;
        }

        if (second < 0 || second > 59)
        {
            errorMessage = "second must be between 00 and 59";
            return false;
        }

        return true;
    }

    std::map<std::string, StringFieldValidator> &validatorRegistry()
    {
        static std::map<std::string, StringFieldValidator> registry = []
        {
            std::map<std::string, StringFieldValidator> validators;
            validators.emplace("datetime_utc", createDateTimeUtcValidator());
            return validators;
        }();

        return registry;
    }

} // namespace

FlgParser::FlgParser() : expectedKVPairs(0), delimiter(',') {}

FlgParser::~FlgParser() {}

void FlgParser::setExpectedKeyValuePairs(size_t count)
{
    expectedKVPairs = count;
}

void FlgParser::setDelimiter(char newDelimiter)
{
    delimiter = newDelimiter;
}

void FlgParser::setColumnType(size_t columnIndex,
                              const std::string &columnName,
                              ValueType type)
{
    columnIndexTypeMap[columnIndex] = type;
    columnTypeMap[columnName] = type;

    if (columnIndex < columns.size())
    {
        columns[columnIndex].type = type;
    }
}

void FlgParser::setColumnTypeByName(const std::string &columnName,
                                    ValueType type)
{
    columnTypeMap[columnName] = type;

    for (auto &column : columns)
    {
        if (column.name == columnName)
        {
            column.type = type;
            break;
        }
    }
}

void FlgParser::setRequiredHeading(const std::string &heading,
                                   DiagnosticSeverity severity)
{
    requiredHeadings[heading] = severity;
}

void FlgParser::setOptionalHeading(const std::string &heading)
{
    requiredHeadings.erase(heading);
}

void FlgParser::setFieldValidator(const std::string &fieldName,
                                  const std::string &validatorName)
{
    if (!hasValidator(validatorName))
    {
        throw std::runtime_error("Unknown validator: " + validatorName);
    }

    fieldValidatorMap[fieldName] = validatorName;
}

void FlgParser::setFieldValidator(const std::string &fieldName,
                                  StringFieldValidator validator)
{
    registerValidator(fieldName, std::move(validator));
    fieldValidatorMap[fieldName] = fieldName;
}

void FlgParser::clearFieldValidator(const std::string &fieldName)
{
    fieldValidatorMap.erase(fieldName);
}

bool FlgParser::serialise(std::ostream &output) const
{
    const auto writeField = [this, &output](const std::string &field,
                                            bool lastField)
    {
        output << field;
        output << (lastField ? '\n' : delimiter);
        return static_cast<bool>(output);
    };

    if (columns.empty())
    {
        return kvPairs.size() == 0 && dataRows.empty();
    }

    const auto &kvEntries = kvPairs.entries();
    for (const auto &key : kvPairOrder)
    {
        const auto it = kvEntries.find(key);
        if (it == kvEntries.end())
        {
            continue;
        }

        if (!writeField(key, false) ||
            !writeField(valueToString(it->second), true))
        {
            return false;
        }
    }

    std::vector<std::string> unorderedKeys;
    unorderedKeys.reserve(kvEntries.size());
    for (const auto &entry : kvEntries)
    {
        if (std::find(kvPairOrder.begin(), kvPairOrder.end(), entry.first) ==
            kvPairOrder.end())
        {
            unorderedKeys.push_back(entry.first);
        }
    }
    std::sort(unorderedKeys.begin(), unorderedKeys.end());

    for (const auto &key : unorderedKeys)
    {
        if (!writeField(key, false) ||
            !writeField(valueToString(kvEntries.at(key)), true))
        {
            return false;
        }
    }

    for (size_t columnIndex = 0; columnIndex < columns.size(); ++columnIndex)
    {
        if (!writeField(columns[columnIndex].name,
                        columnIndex + 1 == columns.size()))
        {
            return false;
        }
    }

    for (const auto &row : dataRows)
    {
        if (row.size() != columns.size())
        {
            return false;
        }

        for (size_t columnIndex = 0; columnIndex < row.size(); ++columnIndex)
        {
            if (!writeField(valueToString(row.at(columnIndex)),
                            columnIndex + 1 == row.size()))
            {
                return false;
            }
        }
    }

    return true;
}

bool FlgParser::deserialise(std::istream &input)
{
    ParserDriver driver(*this);
    return driver.parse(input);
}

std::vector<ParseDiagnostic> FlgParser::getErrors() const
{
    std::vector<ParseDiagnostic> errors;
    for (const auto &diagnostic : diagnostics)
    {
        if (diagnostic.severity == DiagnosticSeverity::ERROR)
        {
            errors.push_back(diagnostic);
        }
    }
    return errors;
}

std::vector<ParseDiagnostic> FlgParser::getWarnings() const
{
    std::vector<ParseDiagnostic> warnings;
    for (const auto &diagnostic : diagnostics)
    {
        if (diagnostic.severity == DiagnosticSeverity::WARNING)
        {
            warnings.push_back(diagnostic);
        }
    }
    return warnings;
}

const ParseDiagnostic *FlgParser::getLastError() const
{
    for (auto it = diagnostics.rbegin(); it != diagnostics.rend(); ++it)
    {
        if (it->severity == DiagnosticSeverity::ERROR)
        {
            return &(*it);
        }
    }

    return nullptr;
}

const ParseDiagnostic *FlgParser::getLastWarning() const
{
    for (auto it = diagnostics.rbegin(); it != diagnostics.rend(); ++it)
    {
        if (it->severity == DiagnosticSeverity::WARNING)
        {
            return &(*it);
        }
    }

    return nullptr;
}

ValueType FlgParser::getColumnType(const std::string &columnName) const
{
    const auto configuredType = columnTypeMap.find(columnName);
    if (configuredType != columnTypeMap.end())
    {
        return configuredType->second;
    }

    for (const auto &column : columns)
    {
        if (column.name == columnName)
        {
            return column.type;
        }
    }

    throw std::runtime_error("Column not found: " + columnName);
}

void FlgParser::clear()
{
    kvPairs.clear();
    columns.clear();
    dataRows.clear();
    columnTypeMap.clear();
    columnIndexTypeMap.clear();
    requiredHeadings.clear();
    fieldValidatorMap.clear();
    diagnostics.clear();
    kvPairOrder.clear();
}

bool tryParseValue(const std::string &str, ValueType type, Value &result)
{
    const std::string trimmed = trimCopy(str);

    if (trimmed.empty())
    {
        result = Value(trimmed);
        return true;
    }

    try
    {
        switch (type)
        {
        case ValueType::INT:
        {
            size_t parsedLength = 0;
            const int parsedValue = std::stoi(trimmed, &parsedLength);
            if (parsedLength != trimmed.size())
            {
                break;
            }
            result = Value(parsedValue);
            return true;
        }
        case ValueType::LONG:
        {
            size_t parsedLength = 0;
            const long long parsedValue = std::stoll(trimmed, &parsedLength);
            if (parsedLength != trimmed.size())
            {
                break;
            }
            result = Value(static_cast<long>(parsedValue));
            return true;
        }
        case ValueType::UINT8:
        {
            size_t parsedLength = 0;
            const unsigned long parsedValue = std::stoul(trimmed, &parsedLength);
            if (parsedLength != trimmed.size())
            {
                break;
            }
            result = Value(static_cast<std::uint8_t>(parsedValue));
            return true;
        }
        case ValueType::UINT16:
        {
            size_t parsedLength = 0;
            const unsigned long parsedValue = std::stoul(trimmed, &parsedLength);
            if (parsedLength != trimmed.size())
            {
                break;
            }
            result = Value(static_cast<std::uint16_t>(parsedValue));
            return true;
        }
        case ValueType::UINT32:
        {
            size_t parsedLength = 0;
            const unsigned long parsedValue = std::stoul(trimmed, &parsedLength);
            if (parsedLength != trimmed.size())
            {
                break;
            }
            result = Value(static_cast<std::uint32_t>(parsedValue));
            return true;
        }
        case ValueType::UINT64:
        {
            size_t parsedLength = 0;
            const unsigned long long parsedValue =
                std::stoull(trimmed, &parsedLength);
            if (parsedLength != trimmed.size())
            {
                break;
            }
            result = Value(static_cast<std::uint64_t>(parsedValue));
            return true;
        }
        case ValueType::BOOL:
        {
            bool boolResult = false;
            if (isBoolean(trimmed, boolResult))
            {
                result = Value(boolResult);
                return true;
            }

            size_t parsedLength = 0;
            const long long parsedValue = std::stoll(trimmed, &parsedLength);
            if (parsedLength != trimmed.size())
            {
                break;
            }
            result = Value(parsedValue != 0);
            return true;
        }
        case ValueType::DOUBLE:
        {
            size_t parsedLength = 0;
            const double parsedValue = std::stod(trimmed, &parsedLength);
            if (parsedLength != trimmed.size())
            {
                break;
            }
            result = Value(parsedValue);
            return true;
        }
        case ValueType::FLOAT:
        {
            size_t parsedLength = 0;
            const float parsedValue = std::stof(trimmed, &parsedLength);
            if (parsedLength != trimmed.size())
            {
                break;
            }
            result = Value(parsedValue);
            return true;
        }
        case ValueType::STRING:
            result = Value(trimmed);
            return true;
        }
    }
    catch (...)
    {
        result = Value(trimmed);
        return type == ValueType::STRING;
    }

    result = Value(trimmed);
    return false;
}

Value parseValue(const std::string &str, ValueType type)
{
    Value result;
    tryParseValue(str, type, result);
    return result;
}

std::string valueToString(const Value &value)
{
    if (std::holds_alternative<int>(value))
    {
        return std::to_string(std::get<int>(value));
    }
    if (std::holds_alternative<long>(value))
    {
        return std::to_string(std::get<long>(value));
    }
    if (std::holds_alternative<std::string>(value))
    {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<std::uint8_t>(value))
    {
        return std::to_string(std::get<std::uint8_t>(value));
    }
    if (std::holds_alternative<std::uint16_t>(value))
    {
        return std::to_string(std::get<std::uint16_t>(value));
    }
    if (std::holds_alternative<std::uint32_t>(value))
    {
        return std::to_string(std::get<std::uint32_t>(value));
    }
    if (std::holds_alternative<std::uint64_t>(value))
    {
        return std::to_string(std::get<std::uint64_t>(value));
    }
    if (std::holds_alternative<bool>(value))
    {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<double>(value))
    {
        return std::to_string(std::get<double>(value));
    }
    if (std::holds_alternative<float>(value))
    {
        return std::to_string(std::get<float>(value));
    }

    return "";
}

void registerValidator(const std::string &validatorName,
                       StringFieldValidator validator)
{
    validatorRegistry()[validatorName] = std::move(validator);
}

bool hasValidator(const std::string &validatorName)
{
    return validatorRegistry().find(validatorName) != validatorRegistry().end();
}

void clearValidator(const std::string &validatorName)
{
    validatorRegistry().erase(validatorName);
}

StringFieldValidator getValidator(const std::string &validatorName)
{
    const auto it = validatorRegistry().find(validatorName);
    if (it == validatorRegistry().end())
    {
        throw std::runtime_error("Unknown validator: " + validatorName);
    }

    return it->second;
}

std::vector<std::string> listValidatorNames()
{
    std::vector<std::string> names;
    names.reserve(validatorRegistry().size());
    for (const auto &entry : validatorRegistry())
    {
        names.push_back(entry.first);
    }
    return names;
}

StringFieldValidator createDateTimeUtcValidator()
{
    return [](const std::string &value, std::string &errorMessage)
    {
        return isValidDateTimeUtc(value, errorMessage);
    };
}
