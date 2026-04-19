%option reentrant
%option bison-bridge
%option bison-locations
%option noyywrap
%option nodefault
%option extra-type="ParserDriver *"

%{
#include "ParserDriver.hpp"
#include "parser.tab.hpp"
#include <string>

#define YY_DECL int yylex(YYSTYPE* yylval_param, YYLTYPE* yylloc_param, yyscan_t yyscanner, ParserDriver& driver)
#define YY_INPUT(buf, result, max_size) result = yyextra->readInput(reinterpret_cast<char*>(buf), max_size)
#define HANDLE_PENDING_BOUNDARY()                                                \
    do {                                                                         \
        if (driver.lexerHasPendingBoundary()) {                                  \
            yylloc_param->first_line =                                           \
                static_cast<int>(driver.lexerPendingBoundaryLine());             \
            yylloc_param->last_line =                                            \
                static_cast<int>(driver.lexerPendingBoundaryLine());             \
            yylloc_param->first_column =                                         \
                static_cast<int>(driver.lexerPendingBoundaryColumn());           \
            yylloc_param->last_column =                                          \
                static_cast<int>(driver.lexerPendingBoundaryColumn());           \
            const ParserDriver::BoundaryToken boundary =                         \
                driver.lexerPendingBoundary();                                   \
            driver.lexerClearPendingBoundary();                                  \
            yyless(0);                                                           \
            if (boundary == ParserDriver::BoundaryToken::DELIMITER) {            \
                driver.lexerConsumeDelimiter();                                  \
                return DELIMITER;                                                \
            }                                                                    \
            driver.lexerConsumeNewline();                                        \
            return NEWLINE;                                                      \
        }                                                                        \
    } while (0)
%}

%%

\n  {
        HANDLE_PENDING_BOUNDARY();

        if (driver.lexerExpectField()) {
            if (driver.lexerLineHasContent()) {
                yylloc_param->first_line = static_cast<int>(driver.lexerLine());
                yylloc_param->last_line = static_cast<int>(driver.lexerLine());
                yylloc_param->first_column = static_cast<int>(driver.lexerColumn());
                yylloc_param->last_column = static_cast<int>(driver.lexerColumn());
                yylval_param->string_val = new std::string();
                driver.lexerEmitSyntheticField();
                driver.lexerStorePendingBoundary(ParserDriver::BoundaryToken::NEWLINE,
                                                driver.lexerLine(),
                                                driver.lexerColumn());
                return FIELD;
            }

            driver.lexerConsumeNewline();
            yylloc_param->first_line = static_cast<int>(driver.lexerLine() - 1);
            yylloc_param->last_line = static_cast<int>(driver.lexerLine() - 1);
            yylloc_param->first_column = 1;
            yylloc_param->last_column = 1;
            return NEWLINE;
        }

        yylloc_param->first_line = static_cast<int>(driver.lexerLine());
        yylloc_param->last_line = static_cast<int>(driver.lexerLine());
        yylloc_param->first_column = static_cast<int>(driver.lexerColumn());
        yylloc_param->last_column = static_cast<int>(driver.lexerColumn());
        driver.lexerConsumeNewline();
        return NEWLINE;
    }

.   {
        HANDLE_PENDING_BOUNDARY();

        const char current = yytext[0];
        const size_t line = driver.lexerLine();
        const size_t column = driver.lexerColumn();

        if (current == driver.getDelimiter()) {
            if (driver.lexerExpectField()) {
                yylloc_param->first_line = static_cast<int>(line);
                yylloc_param->last_line = static_cast<int>(line);
                yylloc_param->first_column = static_cast<int>(column);
                yylloc_param->last_column = static_cast<int>(column);
                yylval_param->string_val = new std::string();
                driver.lexerEmitSyntheticField();
                driver.lexerStorePendingBoundary(
                    ParserDriver::BoundaryToken::DELIMITER, line, column);
                return FIELD;
            }

            yylloc_param->first_line = static_cast<int>(line);
            yylloc_param->last_line = static_cast<int>(line);
            yylloc_param->first_column = static_cast<int>(column);
            yylloc_param->last_column = static_cast<int>(column);
            driver.lexerConsumeDelimiter();
            return DELIMITER;
        }

        std::string field(1, current);
        int next = 0;

        while ((next = yyinput(yyscanner)) > 0) {
            if (next == '\n' || next == driver.getDelimiter()) {
                driver.lexerStorePendingBoundary(
                    next == '\n' ? ParserDriver::BoundaryToken::NEWLINE
                                 : ParserDriver::BoundaryToken::DELIMITER,
                    line, column + field.size());
                break;
            }

            field.push_back(static_cast<char>(next));
        }

        yylloc_param->first_line = static_cast<int>(line);
        yylloc_param->last_line = static_cast<int>(line);
        yylloc_param->first_column = static_cast<int>(column);
        yylloc_param->last_column =
            static_cast<int>(column + field.size() - 1);
        yylval_param->string_val = new std::string(field);
        driver.lexerConsumeField(field.size());
        return FIELD;
    }

<<EOF>> {
        if (driver.lexerHasPendingBoundary()) {
            yylloc_param->first_line =
                static_cast<int>(driver.lexerPendingBoundaryLine());
            yylloc_param->last_line =
                static_cast<int>(driver.lexerPendingBoundaryLine());
            yylloc_param->first_column =
                static_cast<int>(driver.lexerPendingBoundaryColumn());
            yylloc_param->last_column =
                static_cast<int>(driver.lexerPendingBoundaryColumn());

            const ParserDriver::BoundaryToken boundary =
                driver.lexerPendingBoundary();
            driver.lexerClearPendingBoundary();

            if (boundary == ParserDriver::BoundaryToken::DELIMITER) {
                driver.lexerConsumeDelimiter();
                return DELIMITER;
            }

            driver.lexerConsumeNewline();
            return NEWLINE;
        }

        if (driver.lexerExpectField() && driver.lexerLineHasContent()) {
            yylloc_param->first_line = static_cast<int>(driver.lexerLine());
            yylloc_param->last_line = static_cast<int>(driver.lexerLine());
            yylloc_param->first_column = static_cast<int>(driver.lexerColumn());
            yylloc_param->last_column = static_cast<int>(driver.lexerColumn());
            yylval_param->string_val = new std::string();
            driver.lexerEmitSyntheticField();
            return FIELD;
        }

        return 0;
    }

%%
