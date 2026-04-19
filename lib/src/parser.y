%code requires {
#include <string>
#include "ParserDriver.hpp"

typedef void* yyscan_t;
}

%code provides {
int yylex(YYSTYPE* yylval_param,
          YYLTYPE* yylloc_param,
          yyscan_t yyscanner,
          ParserDriver& driver);
void yyerror(YYLTYPE* locp,
             ParserDriver& driver,
             yyscan_t scanner,
             const char* message);
}

%define api.pure full
%define parse.error verbose
%locations

%parse-param { ParserDriver& driver }
%parse-param { yyscan_t scanner }
%lex-param { yyscan_t scanner }
%lex-param { ParserDriver& driver }

%union {
    std::string* string_val;
    ParsedField* field_val;
    ParsedRow* row_val;
}

%token <string_val> FIELD
%token DELIMITER
%token NEWLINE

%type <field_val> field
%type <row_val> row

%%

document:
    %empty
    | lines
    ;

lines:
    line
    | lines line
    ;

line:
    row NEWLINE
    {
        driver.consumeRow(*$1,
                          static_cast<size_t>(@1.first_line),
                          static_cast<size_t>(@2.first_column));
        delete $1;
    }
    | row
    {
        driver.consumeRow(*$1,
                          static_cast<size_t>(@1.first_line),
                          static_cast<size_t>(@1.last_column + 1));
        delete $1;
    }
    ;

row:
    field
    {
        $$ = new ParsedRow();
        $$->push_back(*$1);
        delete $1;
    }
    | row DELIMITER field
    {
        $1->push_back(*$3);
        $$ = $1;
        delete $3;
    }
    ;

field:
    FIELD
    {
        $$ = new ParsedField{*$1, static_cast<size_t>(@1.first_column)};
        delete $1;
    }
    ;

%%

void yyerror(YYLTYPE* locp,
             ParserDriver& driver,
             yyscan_t,
             const char* message) {
    driver.addSyntaxError(static_cast<size_t>(locp->first_line),
                          static_cast<size_t>(locp->first_column),
                          message);
}
