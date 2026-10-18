%{
#include "AST.h"
#include <iostream>
#include <string>
#include "parser.tab.hpp"
%}

%option noyywrap
%option yylineno

DIGITS   [0-9]+
IDENT    [a-zA-Z_][a-zA-Z0-9_]*
STR_LIT  \"[^\"]*\"
CHR_LIT  \'[^\']\'

%%

[ \t\r\n]+              { /* пропускаем пробелы */ }

"i32"                   { return KW_I32; }
"char"                  { return KW_CHAR; }
"bool"                  { return KW_BOOL; }
"ptr\$"                  { return KW_PTR; }

"if"                    { return KW_IF; }
"elif"                  { return KW_ELIF; }
"else"                  { return KW_ELSE; }
"while"                 { return KW_WHILE; }
"return"                { return KW_RETURN; }
"true"                  { return KW_TRUE; }
"false"                 { return KW_FALSE; }

"+="                    { return OP_ADD_ASSIGN; }
"=="                    { return OP_EQ; }
"!="                    { return OP_NEQ; }
"<="                    { return OP_LEQ; }
">="                    { return OP_GEQ; }
"+"                     { return OP_PLUS; }
"-"                     { return OP_MINUS; }
"*"                     { return OP_STAR; }
"/"                     { return OP_SLASH; }
"="                     { return OP_ASSIGN; }
"<"                     { return OP_LT; }
">"                     { return OP_GT; }
"&"                     { return OP_AMP; }
"|"                     { return OP_PIPE; }
"^"                     { return OP_CARET; }

"["                     { return SYM_LBRACKET; }
"]"                     { return SYM_RBRACKET; }
"("                     { return SYM_LPAREN; }
")"                     { return SYM_RPAREN; }
"{"                     { return SYM_LBRACE; }
"}"                     { return SYM_RBRACE; }
";"                     { return SYM_SEMI; }
","                     { return SYM_COMMA; }

{DIGITS}                { yylval.int_val = std::stoi(yytext); return NUM_INT; }
{IDENT}                 { yylval.str_val = new std::string(yytext); return IDENT; }
{STR_LIT}               {
                            std::string s(yytext);
                            yylval.str_val = new std::string(s.substr(1, s.size() - 2));
                            return LIT_STRING;
                        }
{CHR_LIT}               { yylval.char_val = yytext[1]; return LIT_CHAR; }

.                       {
                            std::cerr << "Lexical error: unexpected symbol '" << yytext << "'\n";
                            exit(1);
                        }
%%