#include "ParserDriver.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <stdexcept>

using yyscan_t = void*;

int yyparse(ParserDriver& driver, yyscan_t scanner);
int yylex_init_extra(ParserDriver* userDefined, yyscan_t* scanner);
int yylex_destroy(yyscan_t scanner);
void yyset_in(FILE* inputFile, yyscan_t scanner);

namespace {

std::string trimCopy(const std::string& input) {
    const size_t first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const size_t last = input.find_last_not_of(" \t\r\n");
    return input.substr(first, last - first + 1);
}

size_t firstNonWhitespaceColumn(const ParsedField& field) {
    const size_t offset = field.value.find_first_not_of(" \t\r\n");
    return offset == std::string::npos ? field.column : field.column + offset;
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

void rememberKeyOrder(std::vector<std::string>& keyOrder, const std::string& key) {
    if (std::find(keyOrder.begin(), keyOrder.end(), key) == keyOrder.end()) {
        keyOrder.push_back(key);
    }
}

} // namespace

ParserDriver::ParserDriver(FlgParser& parserInstance) : parser(parserInstance) {
    reset();
}

bool ParserDriver::parse(std::istream& input) {
    reset();

    std::ostringstream buffer;
    buffer << input.rdbuf();
    sourceContent = buffer.str();
    sourceOffset = 0;

    yyscan_t scanner = nullptr;
    if (yylex_init_extra(this, &scanner) != 0) {
        addDiagnostic(DiagnosticSeverity::ERROR, 1, 1,
                      "Failed to initialise lexer");
        return false;
    }

    yyset_in(stdin, scanner);
    const int parseResult = yyparse(*this, scanner);
    yylex_destroy(scanner);

    if (!headersParsed && parser.getErrors().empty()) {
        addDiagnostic(DiagnosticSeverity::ERROR, lastLine == 0 ? 1 : lastLine, 1,
                      "Input did not contain a header row");
    }

    return parseResult == 0 && parser.getErrors().empty();
}

void ParserDriver::consumeRow(const ParsedRow& row, size_t line, size_t endColumn) {
    lastLine = line;

    if (!parser.getErrors().empty()) {
        return;
    }

    if (rowIsBlank(row)) {
        return;
    }

    if (parsingKeyValuePairs && !headersParsed) {
        if (parser.expectedKVPairs > 0 && kvCount < parser.expectedKVPairs) {
            parseKeyValueRow(row, line);
            return;
        }

        if (parser.expectedKVPairs > 0 && kvCount == parser.expectedKVPairs) {
            parsingKeyValuePairs = false;
            parseHeaderRow(row, line);
            return;
        }

        if (row.size() == 2) {
            parseKeyValueRow(row, line);
            return;
        }

        if (row.size() > 2) {
            parsingKeyValuePairs = false;
            parseHeaderRow(row, line);
            return;
        }

        addDiagnostic(DiagnosticSeverity::ERROR, line,
                      firstNonWhitespaceColumn(row.front()),
                      "Expected either a key-value pair or a delimited header row");
        return;
    }

    parseDataRow(row, line, endColumn);
}

void ParserDriver::addSyntaxError(size_t line,
                                  size_t column,
                                  const std::string& message) {
    lastLine = line;
    addDiagnostic(DiagnosticSeverity::ERROR, line, column, message);
}

char ParserDriver::getDelimiter() const {
    return parser.delimiter;
}

int ParserDriver::readInput(char* buffer, int maxSize) {
    const size_t remaining = sourceContent.size() - sourceOffset;
    const size_t count =
        std::min(static_cast<size_t>(maxSize), remaining);

    if (count == 0) {
        return 0;
    }

    std::copy_n(sourceContent.data() + sourceOffset, count, buffer);
    sourceOffset += count;
    return static_cast<int>(count);
}

size_t ParserDriver::lexerLine() const {
    return lexerState.line;
}

size_t ParserDriver::lexerColumn() const {
    return lexerState.column;
}

bool ParserDriver::lexerExpectField() const {
    return lexerState.expectField;
}

bool ParserDriver::lexerLineHasContent() const {
    return lexerState.lineHasContent;
}

void ParserDriver::lexerConsumeDelimiter() {
    lexerState.lineHasContent = true;
    lexerState.expectField = true;
    ++lexerState.column;
}

void ParserDriver::lexerConsumeNewline() {
    ++lexerState.line;
    lexerState.column = 1;
    lexerState.expectField = true;
    lexerState.lineHasContent = false;
}

void ParserDriver::lexerConsumeField(size_t length) {
    lexerState.lineHasContent = true;
    lexerState.expectField = false;
    lexerState.column += length;
}

void ParserDriver::lexerEmitSyntheticField() {
    lexerState.expectField = false;
}

bool ParserDriver::lexerHasPendingBoundary() const {
    return lexerState.pendingBoundary != BoundaryToken::NONE;
}

ParserDriver::BoundaryToken ParserDriver::lexerPendingBoundary() const {
    return lexerState.pendingBoundary;
}

size_t ParserDriver::lexerPendingBoundaryLine() const {
    return lexerState.pendingBoundaryLine;
}

size_t ParserDriver::lexerPendingBoundaryColumn() const {
    return lexerState.pendingBoundaryColumn;
}

void ParserDriver::lexerStorePendingBoundary(BoundaryToken token,
                                             size_t line,
                                             size_t column) {
    lexerState.pendingBoundary = token;
    lexerState.pendingBoundaryLine = line;
    lexerState.pendingBoundaryColumn = column;
}

void ParserDriver::lexerClearPendingBoundary() {
    lexerState.pendingBoundary = BoundaryToken::NONE;
    lexerState.pendingBoundaryLine = 0;
    lexerState.pendingBoundaryColumn = 0;
}

void ParserDriver::reset() {
    parser.kvPairs.clear();
    parser.columns.clear();
    parser.dataRows.clear();
    parser.diagnostics.clear();
    parser.kvPairOrder.clear();

    parsingKeyValuePairs = true;
    headersParsed = false;
    kvCount = 0;
    lastLine = 0;
    sourceContent.clear();
    sourceOffset = 0;

    lexerState.line = 1;
    lexerState.column = 1;
    lexerState.expectField = true;
    lexerState.lineHasContent = false;
    lexerState.pendingBoundary = BoundaryToken::NONE;
    lexerState.pendingBoundaryLine = 0;
    lexerState.pendingBoundaryColumn = 0;
}

void ParserDriver::addDiagnostic(DiagnosticSeverity severity,
                                 size_t line,
                                 size_t column,
                                 const std::string& message) {
    parser.diagnostics.push_back({line, column, severity, message});
}

void ParserDriver::parseKeyValueRow(const ParsedRow& row, size_t line) {
    if (row.size() < 2) {
        addDiagnostic(DiagnosticSeverity::ERROR, line,
                      firstNonWhitespaceColumn(row.front()),
                      "Expected a key-value pair separated by '" +
                          std::string(1, parser.delimiter) + "'");
        return;
    }

    if (row.size() > 2) {
        addDiagnostic(DiagnosticSeverity::ERROR, line, row[2].column - 1,
                      "Too many delimiters in key-value pair");
        return;
    }

    const std::string key = trimCopy(row[0].value);
    const std::string value = trimCopy(row[1].value);
    if (key.empty()) {
        addDiagnostic(DiagnosticSeverity::ERROR, line, 1,
                      "Key-value pair is missing a key");
        return;
    }

    if (!validateFieldValue(key, row[1], line)) {
        return;
    }

    Value parsedValue;
    if (value.find('.') != std::string::npos &&
        tryParseValue(value, ValueType::DOUBLE, parsedValue) &&
        std::holds_alternative<double>(parsedValue)) {
        parser.kvPairs.set(key, std::get<double>(parsedValue));
    } else if (tryParseValue(value, ValueType::LONG, parsedValue) &&
               std::holds_alternative<long>(parsedValue)) {
        parser.kvPairs.set(key, std::get<long>(parsedValue));
    } else {
        parser.kvPairs.set(key, value);
    }

    rememberKeyOrder(parser.kvPairOrder, key);
    ++kvCount;
}

void ParserDriver::parseHeaderRow(const ParsedRow& row, size_t line) {
    headersParsed = true;

    for (size_t columnIndex = 0; columnIndex < row.size(); ++columnIndex) {
        ColumnDefinition column;
        column.name = trimCopy(row[columnIndex].value);
        column.type = ValueType::STRING;

        const auto indexIt = parser.columnIndexTypeMap.find(columnIndex);
        if (indexIt != parser.columnIndexTypeMap.end()) {
            column.type = indexIt->second;
        } else {
            const auto nameIt = parser.columnTypeMap.find(column.name);
            if (nameIt != parser.columnTypeMap.end()) {
                column.type = nameIt->second;
            }
        }

        parser.columns.push_back(column);
    }

    validateRequiredHeadings(line);
}

void ParserDriver::parseDataRow(const ParsedRow& row,
                                size_t line,
                                size_t endColumn) {
    ValueVector parsedRow;

    for (size_t columnIndex = 0;
         columnIndex < row.size() && columnIndex < parser.columns.size();
         ++columnIndex) {
        Value parsedValue;
        const std::string trimmedField = trimCopy(row[columnIndex].value);
        const ValueType expectedType = parser.columns[columnIndex].type;

        if (!tryParseValue(trimmedField, expectedType, parsedValue)) {
            addDiagnostic(DiagnosticSeverity::ERROR, line, row[columnIndex].column,
                          "Unexpected type for column '" +
                              parser.columns[columnIndex].name + "': expected " +
                              valueTypeToString(expectedType) + ", got '" +
                              trimmedField + "'");
            return;
        }

        if (!validateFieldValue(parser.columns[columnIndex].name,
                                row[columnIndex],
                                line)) {
            return;
        }

        parsedRow.push(parsedValue);
    }

    if (row.size() != parser.columns.size()) {
        if (row.size() > parser.columns.size()) {
            addDiagnostic(DiagnosticSeverity::ERROR, line,
                          row[parser.columns.size()].column,
                          "Too many delimiters in data row");
            return;
        }

        addDiagnostic(DiagnosticSeverity::ERROR, line, endColumn,
                      "Too few fields in data row");
        return;
    }

    parser.dataRows.push_back(parsedRow);
}

bool ParserDriver::validateFieldValue(const std::string& fieldName,
                                      const ParsedField& field,
                                      size_t line) {
    const auto validatorIt = parser.fieldValidatorMap.find(fieldName);
    if (validatorIt == parser.fieldValidatorMap.end()) {
        return true;
    }

    std::string errorMessage;
    const std::string trimmedValue = trimCopy(field.value);
    StringFieldValidator validator;
    try {
        validator = getValidator(validatorIt->second);
    } catch (const std::exception& exception) {
        addDiagnostic(DiagnosticSeverity::ERROR,
                      line,
                      firstNonWhitespaceColumn(field),
                      "Validation setup error for field '" + fieldName +
                          "': " + exception.what());
        return false;
    }

    if (validator(trimmedValue, errorMessage)) {
        return true;
    }

    addDiagnostic(DiagnosticSeverity::ERROR,
                  line,
                  firstNonWhitespaceColumn(field),
                  errorMessage.empty()
                      ? "Validation failed for field '" + fieldName + "'"
                      : "Validation failed for field '" + fieldName + "': " +
                            errorMessage);
    return false;
}

bool ParserDriver::rowIsBlank(const ParsedRow& row) const {
    return row.size() == 1 && trimCopy(row.front().value).empty();
}

void ParserDriver::validateRequiredHeadings(size_t line) {
    for (const auto& requirement : parser.requiredHeadings) {
        const auto it = std::find_if(
            parser.columns.begin(), parser.columns.end(),
            [&requirement](const ColumnDefinition& column) {
                return column.name == requirement.first;
            });

        if (it == parser.columns.end()) {
            addDiagnostic(requirement.second, line, 1,
                          "Required heading missing: " + requirement.first);
        }
    }
}
