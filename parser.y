%code requires {
    #include "ast.h"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

int yylex(void);
void yyerror(const char *message);

extern int yylineno;

Ast *root = NULL;
%}

%union {
    double number;
    char *text;
    Ast *node;
    AstList *list;
}

%token <text> ID
%token <number> INT_LIT FLOAT_LIT

%token INT FLOAT IF ELSE WHILE PRINT
%token EQ NE LE GE AND OR NOT
%token PLUS MINUS TIMES DIVIDE ASSIGN LT GT
%token LPAREN RPAREN LBRACE RBRACE SEMI
%token INVALID

%type <node> program statement expression type
%type <list> statement_list

%destructor { free($$); } <text>

%nonassoc IFX
%nonassoc ELSE

%left OR
%left AND
%left EQ NE
%left LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE
%right NOT UMINUS

%start program

%%

program:
    statement_list
    {
        root = ast_new(
            AST_PROGRAM, NULL, NULL, 0,
            NULL, NULL, NULL, $1
        );
        $$ = root;
    }
    ;

statement_list:
    %empty
    {
        $$ = NULL;
    }
    | statement_list statement
    {
        $$ = ast_list_append($1, $2);
    }
    ;

type:
    INT
    {
        $$ = ast_new(AST_NUMBER, NULL, "int", 0, NULL, NULL, NULL, NULL);
    }
    | FLOAT
    {
        $$ = ast_new(AST_NUMBER, NULL, "float", 0, NULL, NULL, NULL, NULL);
    }
    ;

statement:
    type ID SEMI
    {
        $$ = ast_new(AST_DECL, $2, $1->type, 0,
                     NULL, NULL, NULL, NULL);
        free($2);
        ast_free($1);
    }
    | type ID ASSIGN expression SEMI
    {
        $$ = ast_new(AST_DECL, $2, $1->type, 0,
                     $4, NULL, NULL, NULL);
        free($2);
        ast_free($1);
    }
    | ID ASSIGN expression SEMI
    {
        $$ = ast_new(AST_ASSIGN, $1, NULL, 0,
                     $3, NULL, NULL, NULL);
        free($1);
    }
    | PRINT LPAREN expression RPAREN SEMI
    {
        $$ = ast_new(AST_PRINT, NULL, NULL, 0,
                     $3, NULL, NULL, NULL);
    }
    | IF LPAREN expression RPAREN statement %prec IFX
    {
        $$ = ast_new(AST_IF, NULL, NULL, 0,
                     $3, $5, NULL, NULL);
    }
    | IF LPAREN expression RPAREN statement ELSE statement
    {
        $$ = ast_new(AST_IF, NULL, NULL, 0,
                     $3, $5, $7, NULL);
    }
    | WHILE LPAREN expression RPAREN statement
    {
        $$ = ast_new(AST_WHILE, NULL, NULL, 0,
                     $3, $5, NULL, NULL);
    }
    | LBRACE statement_list RBRACE
    {
        $$ = ast_new(AST_BLOCK, NULL, NULL, 0,
                     NULL, NULL, NULL, $2);
    }
    ;

expression:
    expression PLUS expression
    {
        $$ = ast_new(AST_BINARY, "+", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression MINUS expression
    {
        $$ = ast_new(AST_BINARY, "-", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression TIMES expression
    {
        $$ = ast_new(AST_BINARY, "*", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression DIVIDE expression
    {
        $$ = ast_new(AST_BINARY, "/", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression EQ expression
    {
        $$ = ast_new(AST_BINARY, "==", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression NE expression
    {
        $$ = ast_new(AST_BINARY, "!=", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression LT expression
    {
        $$ = ast_new(AST_BINARY, "<", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression LE expression
    {
        $$ = ast_new(AST_BINARY, "<=", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression GT expression
    {
        $$ = ast_new(AST_BINARY, ">", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression GE expression
    {
        $$ = ast_new(AST_BINARY, ">=", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression AND expression
    {
        $$ = ast_new(AST_BINARY, "&&", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | expression OR expression
    {
        $$ = ast_new(AST_BINARY, "||", NULL, 0,
                     $1, $3, NULL, NULL);
    }
    | MINUS expression %prec UMINUS
    {
        $$ = ast_new(AST_UNARY, "-", NULL, 0,
                     $2, NULL, NULL, NULL);
    }
    | NOT expression
    {
        $$ = ast_new(AST_UNARY, "!", NULL, 0,
                     $2, NULL, NULL, NULL);
    }
    | LPAREN expression RPAREN
    {
        $$ = $2;
    }
    | INT_LIT
    {
        $$ = ast_new(AST_NUMBER, NULL, "int", $1,
                     NULL, NULL, NULL, NULL);
    }
    | FLOAT_LIT
    {
        $$ = ast_new(AST_NUMBER, NULL, "float", $1,
                     NULL, NULL, NULL, NULL);
    }
    | ID
    {
        $$ = ast_new(AST_VARIABLE, $1, NULL, 0,
                     NULL, NULL, NULL, NULL);
        free($1);
    }
    ;

%%

void yyerror(const char *message)
{
    fprintf(stderr, "Синтаксична помилка у рядку %d: %s\n",
            yylineno, message);
}