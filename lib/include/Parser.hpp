#ifndef PARSER_HPP
#define PARSER_HPP

#include "Values.hpp"
#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

enum class ValueType {
    INT,
    LONG,
    STRING,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    BOOL,
    DOUBLE,
    FLOAT
};

enum class DiagnosticSeverity {
    WARNING,
    ERROR
};

struct ColumnDefinition {
    std::string name;
    ValueType type;
};

struct ParseDiagnostic {
    size_t line;
    size_t column;
    DiagnosticSeverity severity;
    std::string message;
};

using ParseError = ParseDiagnostic;
using StringFieldValidator =
    std::function<bool(const std::string& value, std::string& errorMessage)>;

class FlgParser {
public:
    FlgParser();
    ~FlgParser();

    // Configure the number of expected key-value pairs (default: unlimited)
    void setExpectedKeyValuePairs(size_t count);

    // Configure the delimiter used by the file format (default: ',')
    void setDelimiter(char delimiter);
    char getDelimiter() const { return delimiter; }

    // Configure column types by position (0-based index)
    void setColumnType(size_t columnIndex, const std::string& columnName,
                       ValueType type);

    // Configure column types by header name
    void setColumnTypeByName(const std::string& columnName, ValueType type);

    // Configure required headings. Optional headings are ignored when absent.
    void setRequiredHeading(
        const std::string& heading,
        DiagnosticSeverity severity = DiagnosticSeverity::ERROR);
    void setOptionalHeading(const std::string& heading);
    void setFieldValidator(const std::string& fieldName,
                           const std::string& validatorName);
    void setFieldValidator(const std::string& fieldName,
                           StringFieldValidator validator);
    void clearFieldValidator(const std::string& fieldName);

    // Write the current state as FLG data.
    bool serialise(std::ostream& output) const;

    // Read FLG data from a stream.
    bool deserialise(std::istream& input);

    // Get the key-value pairs from the file header
    ValueMap& getKeyValuePairs() { return kvPairs; }
    const ValueMap& getKeyValuePairs() const { return kvPairs; }

    // Get the column definitions
    const std::vector<ColumnDefinition>& getColumnDefinitions() const {
        return columns;
    }

    // Get all data rows
    const std::vector<ValueVector>& getDataRows() const { return dataRows; }

    // Get a specific row
    const ValueVector& getRow(size_t rowIndex) const {
        if (rowIndex >= dataRows.size()) {
            throw std::out_of_range("Row index out of range");
        }
        return dataRows[rowIndex];
    }

    // Get number of rows
    size_t getRowCount() const { return dataRows.size(); }

    // Get parse diagnostics from the most recent parse attempt.
    const std::vector<ParseDiagnostic>& getDiagnostics() const {
        return diagnostics;
    }
    std::vector<ParseDiagnostic> getErrors() const;
    std::vector<ParseDiagnostic> getWarnings() const;
    const ParseDiagnostic* getLastError() const;
    const ParseDiagnostic* getLastWarning() const;

    // Clear all parsed data
    void clear();

    // Get type for a column (throws if not configured)
    ValueType getColumnType(const std::string& columnName) const;

private:
    ValueMap kvPairs;
    std::vector<ColumnDefinition> columns;
    std::vector<ValueVector> dataRows;
    std::map<std::string, ValueType> columnTypeMap;
    std::map<size_t, ValueType> columnIndexTypeMap;
    std::map<std::string, DiagnosticSeverity> requiredHeadings;
    std::map<std::string, std::string> fieldValidatorMap;
    size_t expectedKVPairs; // Number of expected key-value pairs (0 = unlimited)
    char delimiter;
    std::vector<ParseDiagnostic> diagnostics;
    std::vector<std::string> kvPairOrder;

    friend class ParserDriver;
};

bool tryParseValue(const std::string& str, ValueType type, Value& result);
Value parseValue(const std::string& str, ValueType type);
std::string valueToString(const Value& v);
void registerValidator(const std::string& validatorName,
                       StringFieldValidator validator);
bool hasValidator(const std::string& validatorName);
void clearValidator(const std::string& validatorName);
StringFieldValidator getValidator(const std::string& validatorName);
std::vector<std::string> listValidatorNames();
StringFieldValidator createDateTimeUtcValidator();

#endif // PARSER_HPP
