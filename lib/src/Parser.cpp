#include "Parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>

// TODO: Re-enable when Flex/Bison integration is fixed
// extern FILE* yyin;
// extern int yylex();
// extern int yyparse();

// Global parser instance for use by Flex/Bison (when re-enabled)
FlgParser* g_parser = nullptr;

namespace {

struct FieldSpan {
    std::string value;
    size_t column;
};

std::string trimCopy(const std::string& input) {
    const size_t first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const size_t last = input.find_last_not_of(" \t\r\n");
    return input.substr(first, last - first + 1);
}

size_t findFirstNonWhitespaceColumn(const std::string& line) {
    const size_t first = line.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? 1 : first + 1;
}

size_t countDelimiters(const std::string& line, char delimiter) {
    return static_cast<size_t>(std::count(line.begin(), line.end(), delimiter));
}

std::vector<FieldSpan> splitDelimitedPreserveEmpty(const std::string& line,
                                                   char delimiter) {
    std::vector<FieldSpan> fields;
    size_t start = 0;

    while (true) {
        const size_t delimiter_pos = line.find(delimiter, start);
        if (delimiter_pos == std::string::npos) {
            fields.push_back({line.substr(start), start + 1});
            break;
        }

        fields.push_back({line.substr(start, delimiter_pos - start), start + 1});
        start = delimiter_pos + 1;
    }

    return fields;
}

std::string valueTypeToString(ValueType type) {
    switch (type) {
    case ValueType::INT:
        return "INT";
    case ValueType::LONG:
        return "LONG";
    case ValueType::STRING:
        return "STRING";
    case ValueType::UINT8:
        return "UINT8";
    case ValueType::UINT16:
        return "UINT16";
    case ValueType::UINT32:
        return "UINT32";
    case ValueType::UINT64:
        return "UINT64";
    case ValueType::BOOL:
        return "BOOL";
    case ValueType::DOUBLE:
        return "DOUBLE";
    case ValueType::FLOAT:
        return "FLOAT";
    }
    return "UNKNOWN";
}

bool isBoolean(const std::string& str, bool& result) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "true") {
        result = true;
        return true;
    } else if (lower == "false") {
        result = false;
        return true;
    }
    return false;
}

bool tryParseValue(const std::string& str, ValueType type, Value& result) {
    const std::string trimmed = trimCopy(str);

    if (trimmed.empty()) {
        result = Value(trimmed);
        return true;
    }

    try {
        switch (type) {
        case ValueType::INT:
            result = Value(std::stoi(trimmed));
            return true;
        case ValueType::LONG:
            result = Value(static_cast<long>(std::stoll(trimmed)));
            return true;
        case ValueType::UINT8:
            result = Value(static_cast<std::uint8_t>(std::stoul(trimmed)));
            return true;
        case ValueType::UINT16:
            result = Value(static_cast<std::uint16_t>(std::stoul(trimmed)));
            return true;
        case ValueType::UINT32:
            result = Value(static_cast<std::uint32_t>(std::stoul(trimmed)));
            return true;
        case ValueType::UINT64:
            result = Value(static_cast<std::uint64_t>(std::stoull(trimmed)));
            return true;
        case ValueType::BOOL: {
            bool bool_result;
            if (isBoolean(trimmed, bool_result)) {
                result = Value(bool_result);
                return true;
            }
            result = Value(std::stoll(trimmed) != 0);
            return true;
        }
        case ValueType::DOUBLE:
            result = Value(std::stod(trimmed));
            return true;
        case ValueType::FLOAT:
            result = Value(std::stof(trimmed));
            return true;
        case ValueType::STRING:
            result = Value(trimmed);
            return true;
        }
    } catch (...) {
        result = Value(trimmed);
        return type == ValueType::STRING;
    }

    result = Value(trimmed);
    return false;
}

void parseHeaderLine(const std::string& line,
                     char delimiter,
                     const std::map<size_t, ValueType>& columnIndexTypeMap,
                     const std::map<std::string, ValueType>& columnTypeMap,
                     std::vector<ColumnDefinition>& columns) {
    const std::vector<FieldSpan> header_fields =
        splitDelimitedPreserveEmpty(line, delimiter);

    for (size_t col_index = 0; col_index < header_fields.size(); ++col_index) {
        const std::string header = trimCopy(header_fields[col_index].value);

        ColumnDefinition col;
        col.name = header;
        col.type = ValueType::STRING;

        const auto index_it = columnIndexTypeMap.find(col_index);
        if (index_it != columnIndexTypeMap.end()) {
            col.type = index_it->second;
        } else {
            const auto name_it = columnTypeMap.find(header);
            if (name_it != columnTypeMap.end()) {
                col.type = name_it->second;
            }
        }

        columns.push_back(col);
    }
}

} // namespace

FlgParser::FlgParser() : expectedKVPairs(0), delimiter(',') {}

FlgParser::~FlgParser() {}

void FlgParser::setExpectedKeyValuePairs(size_t count) {
    expectedKVPairs = count;
}

void FlgParser::setDelimiter(char newDelimiter) {
    delimiter = newDelimiter;
}

void FlgParser::setColumnType(size_t columnIndex,
                               const std::string& columnName, ValueType type) {
    columnIndexTypeMap[columnIndex] = type;
    columnTypeMap[columnName] = type;

    if (columnIndex < columns.size()) {
        columns[columnIndex].type = type;
    }
}

void FlgParser::setColumnTypeByName(const std::string& columnName,
                                     ValueType type) {
    columnTypeMap[columnName] = type;

    for (auto& col : columns) {
        if (col.name == columnName) {
            col.type = type;
            break;
        }
    }
}

bool FlgParser::parseFile(const std::string& filename) {
    // TODO: Fix Flex/Bison integration
    // For now, use parseString with file contents
    std::ifstream file(filename);
    if (!file) {
        errors.clear();
        errors.push_back({1, 1, "Cannot open file: " + filename});
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str());
}

bool FlgParser::parseString(const std::string& content) {
    // Reset previously parsed data while preserving configured types.
    kvPairs.clear();
    columns.clear();
    dataRows.clear();
    errors.clear();

    std::istringstream iss(content);
    std::string line;
    bool parsing_kv = true;
    bool headers_parsed = false;
    size_t kv_count = 0;
    size_t line_number = 0;

    const auto addError = [this](size_t line, size_t column,
                                 const std::string& message) {
        errors.push_back({line, column, message});
        return false;
    };

    while (std::getline(iss, line)) {
        line_number++;

        if (trimCopy(line).empty()) {
            continue;
        }

        if (parsing_kv && !headers_parsed) {
            const std::string trimmed_line = trimCopy(line);
            const size_t delimiter_count = countDelimiters(trimmed_line, delimiter);

            if (expectedKVPairs > 0 && kv_count < expectedKVPairs) {
                if (delimiter_count == 0) {
                    return addError(line_number, findFirstNonWhitespaceColumn(line),
                                    "Expected a key-value pair separated by '" +
                                        std::string(1, delimiter) + "'");
                }
                if (delimiter_count > 1) {
                    const size_t second_delimiter =
                        trimmed_line.find(delimiter, trimmed_line.find(delimiter) + 1);
                    return addError(line_number, second_delimiter + 1,
                                    "Too many delimiters in key-value pair");
                }

                const size_t delimiter_pos = trimmed_line.find(delimiter);
                const std::string key = trimCopy(trimmed_line.substr(0, delimiter_pos));
                const std::string val =
                    trimCopy(trimmed_line.substr(delimiter_pos + 1));
                if (key.empty()) {
                    return addError(line_number, 1, "Key-value pair is missing a key");
                }
                kvPairs.set(key, parseValue(val, val.find('.') != std::string::npos
                                                     ? ValueType::DOUBLE
                                                     : ValueType::LONG));
                kv_count++;
                continue;
            }

            if (expectedKVPairs > 0 && kv_count == expectedKVPairs) {
                parsing_kv = false;
                parseHeaderLine(trimmed_line, delimiter, columnIndexTypeMap,
                                columnTypeMap, columns);
                headers_parsed = true;
                continue;
            }

            if (delimiter_count == 1) {
                const size_t delimiter_pos = trimmed_line.find(delimiter);
                const std::string key = trimCopy(trimmed_line.substr(0, delimiter_pos));
                const std::string val =
                    trimCopy(trimmed_line.substr(delimiter_pos + 1));
                if (key.empty()) {
                    return addError(line_number, 1, "Key-value pair is missing a key");
                }

                try {
                    if (val.find('.') != std::string::npos) {
                        kvPairs.set(key, std::stod(val));
                    } else {
                        kvPairs.set(key, static_cast<long>(std::stoll(val)));
                    }
                } catch (...) {
                    kvPairs.set(key, val);
                }
                kv_count++;
                continue;
            }

            if (delimiter_count > 1) {
                parsing_kv = false;
                parseHeaderLine(trimmed_line, delimiter, columnIndexTypeMap,
                                columnTypeMap, columns);
                headers_parsed = true;
                continue;
            }

            return addError(line_number, findFirstNonWhitespaceColumn(line),
                            "Expected either a key-value pair or a delimited header row");
        } else if (headers_parsed) {
            ValueVector row;
            const std::vector<FieldSpan> fields =
                splitDelimitedPreserveEmpty(line, delimiter);

            for (size_t col_index = 0;
                 col_index < fields.size() && col_index < columns.size();
                 ++col_index) {
                Value parsed;
                const std::string trimmed_field = trimCopy(fields[col_index].value);
                const ValueType expected_type = columns[col_index].type;

                if (!tryParseValue(trimmed_field, expected_type, parsed)) {
                    return addError(
                        line_number,
                        fields[col_index].column,
                        "Unexpected type for column '" + columns[col_index].name +
                            "': expected " + valueTypeToString(expected_type) +
                            ", got '" + trimmed_field + "'");
                }

                row.push(parsed);
            }

            if (fields.size() != columns.size()) {
                if (fields.size() > columns.size()) {
                    return addError(
                        line_number,
                        fields[columns.size()].column,
                        "Too many delimiters in data row");
                }

                return addError(line_number, line.size() + 1,
                                "Too few fields in data row");
            }

            dataRows.push_back(row);
        }
    }

    if (!headers_parsed) {
        return addError(line_number == 0 ? 1 : line_number, 1,
                        "Input did not contain a header row");
    }

    return true;
}

const ParseError* FlgParser::getLastError() const {
    if (errors.empty()) {
        return nullptr;
    }

    return &errors.back();
}

ValueType FlgParser::getColumnType(const std::string& columnName) const {
    auto it = columnTypeMap.find(columnName);
    if (it != columnTypeMap.end()) {
        return it->second;
    }

    for (const auto& col : columns) {
        if (col.name == columnName) {
            return col.type;
        }
    }

    throw std::runtime_error("Column not found: " + columnName);
}

void FlgParser::clear() {
    kvPairs.clear();
    columns.clear();
    dataRows.clear();
    columnTypeMap.clear();
    columnIndexTypeMap.clear();
}

Value parseValue(const std::string& str, ValueType type) {
    Value result;
    tryParseValue(str, type, result);
    return result;
}

std::string valueToString(const Value& v) {
    if (std::holds_alternative<int>(v)) {
        return std::to_string(std::get<int>(v));
    } else if (std::holds_alternative<long>(v)) {
        return std::to_string(std::get<long>(v));
    } else if (std::holds_alternative<std::string>(v)) {
        return std::get<std::string>(v);
    } else if (std::holds_alternative<std::uint8_t>(v)) {
        return std::to_string(std::get<std::uint8_t>(v));
    } else if (std::holds_alternative<std::uint16_t>(v)) {
        return std::to_string(std::get<std::uint16_t>(v));
    } else if (std::holds_alternative<std::uint32_t>(v)) {
        return std::to_string(std::get<std::uint32_t>(v));
    } else if (std::holds_alternative<std::uint64_t>(v)) {
        return std::to_string(std::get<std::uint64_t>(v));
    } else if (std::holds_alternative<bool>(v)) {
        return std::get<bool>(v) ? "true" : "false";
    } else if (std::holds_alternative<double>(v)) {
        return std::to_string(std::get<double>(v));
    } else if (std::holds_alternative<float>(v)) {
        return std::to_string(std::get<float>(v));
    }
    return "";
}
