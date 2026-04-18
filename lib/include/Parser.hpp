#ifndef PARSER_HPP
#define PARSER_HPP

#include "Values.hpp"
#include <functional>
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

struct ColumnDefinition {
    std::string name;
    ValueType type;
};

struct ParseError {
    size_t line;
    size_t column;
    std::string message;
};

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

    // Parse a .flg file
    bool parseFile(const std::string& filename);

    // Parse from a string
    bool parseString(const std::string& content);

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
    const std::vector<ParseError>& getErrors() const { return errors; }
    const ParseError* getLastError() const;

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
    size_t expectedKVPairs; // Number of expected key-value pairs (0 = unlimited)
    char delimiter;
    std::vector<ParseError> errors;

    friend class FlgLexer;
    friend int yyparse();
};

// Functions called from the parser
extern FlgParser* g_parser;

Value parseValue(const std::string& str, ValueType type);
std::string valueToString(const Value& v);

#endif // PARSER_HPP
