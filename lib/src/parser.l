%{
#include "Parser.hpp"
#include "parser.tab.hh"
#include <string>
#include <iostream>

// Track line numbers for error reporting
int line_num = 1;

%}

%option noyywrap
%option yylineno
%option nounput
%option noinput

DIGIT           [0-9]
NUMBER          [-+]?{DIGIT}+
FLOAT           [-+]?{DIGIT}+\.{DIGIT}+
BOOLEAN         (true|false|TRUE|FALSE)
IDENTIFIER      [a-zA-Z_][a-zA-Z0-9_]*
STRING          [^\n,]+

%%

\n              {
                    line_num++;
                    return NEWLINE;
                }

","             {
                    return COMMA;
                }

{BOOLEAN}       {
                    yylval.string_val = new std::string(yytext);
                    return BOOLEAN_VAL;
                }

{FLOAT}         {
                    yylval.string_val = new std::string(yytext);
                    return FLOAT_VAL;
                }

{NUMBER}        {
                    yylval.string_val = new std::string(yytext);
                    return NUMBER_VAL;
                }

{STRING}        {
                    yylval.string_val = new std::string(yytext);
                    return STRING_VAL;
                }

[ \t\r]         {
                    // Skip whitespace
                }

.               {
                    std::cerr << "Unexpected character: " << yytext << std::endl;
                }

%%