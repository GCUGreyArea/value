#ifndef PARSER_DRIVER_HPP
#define PARSER_DRIVER_HPP

#include "Parser.hpp"
#include <istream>
#include <vector>

struct ParsedField {
    std::string value;
    size_t column;
};

using ParsedRow = std::vector<ParsedField>;

class ParserDriver {
public:
    enum class BoundaryToken {
        NONE,
        DELIMITER,
        NEWLINE
    };

    explicit ParserDriver(FlgParser& parser);

    bool parse(std::istream& input);

    void consumeRow(const ParsedRow& row, size_t line, size_t endColumn);
    void addSyntaxError(size_t line, size_t column, const std::string& message);

    char getDelimiter() const;
    int readInput(char* buffer, int maxSize);

    size_t lexerLine() const;
    size_t lexerColumn() const;
    bool lexerExpectField() const;
    bool lexerLineHasContent() const;
    void lexerConsumeDelimiter();
    void lexerConsumeNewline();
    void lexerConsumeField(size_t length);
    void lexerEmitSyntheticField();
    bool lexerHasPendingBoundary() const;
    BoundaryToken lexerPendingBoundary() const;
    size_t lexerPendingBoundaryLine() const;
    size_t lexerPendingBoundaryColumn() const;
    void lexerStorePendingBoundary(BoundaryToken token, size_t line, size_t column);
    void lexerClearPendingBoundary();

private:
    struct LexerState {
        size_t line;
        size_t column;
        bool expectField;
        bool lineHasContent;
        BoundaryToken pendingBoundary;
        size_t pendingBoundaryLine;
        size_t pendingBoundaryColumn;
    };

    FlgParser& parser;
    bool parsingKeyValuePairs;
    bool headersParsed;
    size_t kvCount;
    size_t lastLine;
    std::string sourceContent;
    size_t sourceOffset;
    LexerState lexerState;

    void reset();
    void addDiagnostic(DiagnosticSeverity severity,
                       size_t line,
                       size_t column,
                       const std::string& message);
    void parseKeyValueRow(const ParsedRow& row, size_t line);
    void parseHeaderRow(const ParsedRow& row, size_t line);
    void parseDataRow(const ParsedRow& row, size_t line, size_t endColumn);
    bool validateFieldValue(const std::string& fieldName,
                            const ParsedField& field,
                            size_t line);
    void validateRequiredKeyValuePairs(size_t line);
    bool rowIsBlank(const ParsedRow& row) const;
    void validateRequiredHeadings(size_t line);
};

#endif // PARSER_DRIVER_HPP
