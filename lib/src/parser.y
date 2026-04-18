%{
#include "Parser.hpp"
#include <iostream>
#include <vector>
#include <string>

extern int yylex();
extern int line_num;
extern FILE* yyin;

void yyerror(const char* s);

static std::vector<std::string> current_row;
static std::vector<std::string> headers;
static bool in_headers = false;

FlgParser* g_parser = nullptr;

%}

%union {
    std::string* string_val;
    int int_val;
    bool bool_val;
}

%token NEWLINE
%token COMMA
%token <string_val> STRING_VAL
%token <string_val> NUMBER_VAL
%token <string_val> FLOAT_VAL
%token <string_val> BOOLEAN_VAL

%type <string_val> kv_line
%type <string_val> HEADER_LINE
%type <string_val> header_value
%type <string_val> data_row
%type <string_val> row_value
%type <string_val> value

%%

file:
    kv_section headers data_rows
    {
        // Parser completed successfully
    }
    ;

kv_section:
    kv_line
    | kv_section kv_line
    ;

kv_line:
    value COMMA value NEWLINE
    {
        if (g_parser) {
            // Store KV pair
            std::string key = *$1;
            std::string val = *$3;
            
            // Try to parse as number first
            try {
                if (val.find('.') != std::string::npos) {
                    g_parser->getKeyValuePairs().set(key, std::stod(val));
                } else {
                    g_parser->getKeyValuePairs().set(key, static_cast<long>(std::stoll(val)));
                }
            } catch (...) {
                // Store as string if not a number
                g_parser->getKeyValuePairs().set(key, val);
            }
        }
        delete $1;
        delete $3;
    }
    ;

headers:
    STRING_VAL NEWLINE
    {
        // This is typically a section header like "HEADING1"
        delete $1;
    }
    HEADER_LINE
    {
        in_headers = true;
    }
    ;

HEADER_LINE:
    header_value
    {
        headers.clear();
        headers.push_back(*$1);
        delete $1;
    }
    | HEADER_LINE COMMA header_value
    {
        headers.push_back(*$3);
        delete $3;
    }
    NEWLINE
    {
        in_headers = false;
        if (g_parser) {
            // Store headers as column definitions
            // Types will be inferred or configured separately
            for (size_t i = 0; i < headers.size(); ++i) {
                ColumnDefinition col;
                col.name = headers[i];
                col.type = ValueType::STRING; // Default type
                g_parser->columns.push_back(col);
            }
        }
    }
    ;

header_value:
    STRING_VAL { $$ = $1; }
    ;

data_rows:
    data_row
    | data_rows data_row
    ;

data_row:
    row_value
    {
        current_row.clear();
        current_row.push_back(*$1);
        delete $1;
    }
    | data_row COMMA row_value
    {
        current_row.push_back(*$3);
        delete $3;
    }
    NEWLINE
    {
        if (g_parser && current_row.size() == headers.size()) {
            ValueVector row;
            for (size_t i = 0; i < current_row.size(); ++i) {
                ValueType type = ValueType::STRING;
                
                // Get type from configuration or defaults
                if (i < g_parser->columns.size()) {
                    type = g_parser->columns[i].type;
                }
                
                Value parsed = parseValue(current_row[i], type);
                row.push(parsed);
            }
            g_parser->dataRows.push_back(row);
        }
        current_row.clear();
    }
    ;

row_value:
    STRING_VAL { $$ = $1; }
    | NUMBER_VAL { $$ = $1; }
    | FLOAT_VAL { $$ = $1; }
    | BOOLEAN_VAL { $$ = $1; }
    ;

value:
    STRING_VAL { $$ = $1; }
    | NUMBER_VAL { $$ = $1; }
    | FLOAT_VAL { $$ = $1; }
    | BOOLEAN_VAL { $$ = $1; }
    ;

%%

void yyerror(const char* s) {
    std::cerr << "Parse error at line " << line_num << ": " << s << std::endl;
}