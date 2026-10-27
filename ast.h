#ifndef AST_H
#define AST_H

#include <stdio.h>

typedef enum {
    AST_PROGRAM,
    AST_BLOCK,
    AST_DECL,
    AST_ASSIGN,
    AST_PRINT,
    AST_IF,
    AST_WHILE,
    AST_BINARY,
    AST_UNARY,
    AST_NUMBER,
    AST_VARIABLE
} AstKind;

typedef struct Ast Ast;
typedef struct AstList AstList;

struct AstList {
    Ast *item;
    AstList *next;
};

struct Ast {
    AstKind kind;
    char *text;        /* ім'я змінної або оператор */
    char *type;        /* int або float */
    double number;     /* значення числового літерала */
    Ast *a;
    Ast *b;
    Ast *c;
    AstList *items;    /* список інструкцій у програмі або блоці */
};

char *ast_strdup(const char *text);

Ast *ast_new(
    AstKind kind,
    const char *text,
    const char *type,
    double number,
    Ast *a,
    Ast *b,
    Ast *c,
    AstList *items
);

AstList *ast_list_append(AstList *list, Ast *item);

void ast_write_json(FILE *file, Ast *root);
void ast_write_python(FILE *file, Ast *root);
void ast_execute(Ast *root);
void ast_free(Ast *root);

#endif