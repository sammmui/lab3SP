%option noyywrap yylineno never-interactive noinput nounput

%x COMMENT

%{
#include <stdlib.h>
#include "ast.h"
#include "parser.tab.h"
%}

%%

"int"                   { return INT; }
"float"                 { return FLOAT; }
"if"                    { return IF; }
"else"                  { return ELSE; }
"while"                 { return WHILE; }
"print"                 { return PRINT; }

"=="                    { return EQ; }
"!="                    { return NE; }
"<="                    { return LE; }
">="                    { return GE; }
"&&"                    { return AND; }
"||"                    { return OR; }

[0-9]+"."[0-9]+         {
                            yylval.number = strtod(yytext, NULL);
                            return FLOAT_LIT;
                        }

[0-9]+                  {
                            yylval.number = strtod(yytext, NULL);
                            return INT_LIT;
                        }

[a-zA-Z_][a-zA-Z0-9_]*  {
                            yylval.text = ast_strdup(yytext);
                            return ID;
                        }

"+"                     { return PLUS; }
"-"                     { return MINUS; }
"*"                     { return TIMES; }
"/"                     { return DIVIDE; }
"="                     { return ASSIGN; }
"<"                     { return LT; }
">"                     { return GT; }
"!"                     { return NOT; }

"("                     { return LPAREN; }
")"                     { return RPAREN; }
"{"                     { return LBRACE; }
"}"                     { return RBRACE; }
";"                     { return SEMI; }

"//"[^\n]*              { /* однорядковий коментар */ }
"/*"                    { BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>\n             { /* Flex оновлює yylineno */ }
<COMMENT>.              { /* пропускаємо вміст коментаря */ }

[ \t\r\n]+              { /* пропускаємо пробіли */ }

.                       {
                            fprintf(stderr,
                                    "Лексична помилка у рядку %d: '%s'\n",
                                    yylineno, yytext);
                            return INVALID;
                        }

%%