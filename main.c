#include <stdio.h>
#include <stdlib.h>

#include "ast.h"

extern FILE *yyin;
extern Ast *root;

int yyparse(void);

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Використання: %s файл.c\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("Не вдалося відкрити вхідний файл");
        return 1;
    }

    int result = yyparse();
    fclose(yyin);

    if (result != 0 || !root) {
        fprintf(stderr, "Програму не вдалося розібрати.\n");
        return 1;
    }

    FILE *json = fopen("ast.json", "w");
    if (!json) {
        perror("Не вдалося створити ast.json");
        ast_free(root);
        return 1;
    }
    ast_write_json(json, root);
    fclose(json);

    FILE *python = fopen("generated.py", "w");
    if (!python) {
        perror("Не вдалося створити generated.py");
        ast_free(root);
        return 1;
    }
    ast_write_python(python, root);
    fclose(python);

    puts("Синтаксичний аналіз успішний.");
    puts("Створено ast.json і generated.py.");
    puts("Виконання початкової програми:");

    ast_execute(root);
    ast_free(root);

    return 0;
}