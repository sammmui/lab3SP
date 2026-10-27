#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *ast_strdup(const char *text)
{
    size_t size = strlen(text) + 1;
    char *copy = malloc(size);

    if (!copy) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    memcpy(copy, text, size);
    return copy;
}

Ast *ast_new(
    AstKind kind,
    const char *text,
    const char *type,
    double number,
    Ast *a,
    Ast *b,
    Ast *c,
    AstList *items
)
{
    Ast *node = calloc(1, sizeof(Ast));

    if (!node) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    node->kind = kind;
    node->text = text ? ast_strdup(text) : NULL;
    node->type = type ? ast_strdup(type) : NULL;
    node->number = number;
    node->a = a;
    node->b = b;
    node->c = c;
    node->items = items;

    return node;
}

AstList *ast_list_append(AstList *list, Ast *item)
{
    AstList *entry = malloc(sizeof(AstList));

    if (!entry) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    entry->item = item;
    entry->next = NULL;

    if (!list) return entry;

    AstList *last = list;
    while (last->next) last = last->next;
    last->next = entry;

    return list;
}

void ast_free(Ast *node)
{
    if (!node) return;

    ast_free(node->a);
    ast_free(node->b);
    ast_free(node->c);

    AstList *item = node->items;
    while (item) {
        AstList *next = item->next;
        ast_free(item->item);
        free(item);
        item = next;
    }

    free(node->text);
    free(node->type);
    free(node);
}


/* ---------- AST -> JSON ---------- */

static const char *kind_name(AstKind kind)
{
    switch (kind) {
        case AST_PROGRAM:  return "Program";
        case AST_BLOCK:    return "Block";
        case AST_DECL:     return "Declaration";
        case AST_ASSIGN:   return "Assignment";
        case AST_PRINT:    return "Print";
        case AST_IF:       return "If";
        case AST_WHILE:    return "While";
        case AST_BINARY:   return "Binary";
        case AST_UNARY:    return "Unary";
        case AST_NUMBER:   return "Number";
        case AST_VARIABLE: return "Variable";
    }
    return "Unknown";
}

static void json_string(FILE *file, const char *text)
{
    fputc('"', file);

    if (text) {
        for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
            if (*p == '"') fputs("\\\"", file);
            else if (*p == '\\') fputs("\\\\", file);
            else if (*p == '\n') fputs("\\n", file);
            else if (*p == '\r') fputs("\\r", file);
            else if (*p == '\t') fputs("\\t", file);
            else fputc(*p, file);
        }
    }

    fputc('"', file);
}

static void json_node(FILE *file, Ast *node);

static void json_list(FILE *file, AstList *list)
{
    fputc('[', file);

    for (AstList *item = list; item; item = item->next) {
        if (item != list) fputc(',', file);
        json_node(file, item->item);
    }

    fputc(']', file);
}

static void json_children(FILE *file, Ast *node)
{
    fputc('[', file);
    int first = 1;

    Ast *children[] = {node->a, node->b, node->c};

    for (int i = 0; i < 3; i++) {
        if (!children[i]) continue;
        if (!first) fputc(',', file);
        json_node(file, children[i]);
        first = 0;
    }

    fputc(']', file);
}

static void json_node(FILE *file, Ast *node)
{
    if (!node) {
        fputs("null", file);
        return;
    }

    char value[128];
    const char *display = node->text;

    if (node->kind == AST_DECL && node->type && node->text) {
        snprintf(value, sizeof(value), "%s %s", node->type, node->text);
        display = value;
    } else if (node->kind == AST_NUMBER) {
        snprintf(value, sizeof(value), "%g", node->number);
        display = value;
    }

    fputs("{\"name\":", file);
    json_string(file, kind_name(node->kind));
    fputs(",\"val\":", file);

    if (display) json_string(file, display);
    else fputs("null", file);

    fputs(",\"children\":", file);

    if (node->kind == AST_PROGRAM || node->kind == AST_BLOCK) {
        json_list(file, node->items);
    } else {
        json_children(file, node);
    }

    fputc('}', file);
}

void ast_write_json(FILE *file, Ast *root)
{
    json_node(file, root);
    fputc('\n', file);
}


/* ---------- AST interpreter ---------- */

typedef struct Variable {
    char *name;
    char *type;
    double value;
    struct Variable *next;
} Variable;

static Variable *variables = NULL;
static long execution_steps = 0;

static void runtime_error(const char *message)
{
    fprintf(stderr, "Помилка виконання: %s\n", message);
    exit(EXIT_FAILURE);
}

static Variable *find_variable(const char *name)
{
    for (Variable *v = variables; v; v = v->next) {
        if (strcmp(v->name, name) == 0) return v;
    }
    return NULL;
}

static int expr_is_int(Ast *node)
{
    if (node->kind == AST_NUMBER)
        return strcmp(node->type, "int") == 0;

    if (node->kind == AST_VARIABLE) {
        Variable *v = find_variable(node->text);
        if (!v) runtime_error("змінну не оголошено");
        return strcmp(v->type, "int") == 0;
    }

    if (node->kind == AST_UNARY) {
        if (strcmp(node->text, "!") == 0) return 1;
        return expr_is_int(node->a);
    }

    if (node->kind == AST_BINARY) {
        const char *op = node->text;

        if (!strcmp(op, "==") || !strcmp(op, "!=") ||
            !strcmp(op, "<") || !strcmp(op, "<=") ||
            !strcmp(op, ">") || !strcmp(op, ">=") ||
            !strcmp(op, "&&") || !strcmp(op, "||")) {
            return 1;
        }

        return expr_is_int(node->a) && expr_is_int(node->b);
    }

    return 0;
}

static double eval_expr(Ast *node)
{
    if (node->kind == AST_NUMBER) return node->number;

    if (node->kind == AST_VARIABLE) {
        Variable *v = find_variable(node->text);
        if (!v) runtime_error("змінну не оголошено");
        return v->value;
    }

    if (node->kind == AST_UNARY) {
        double value = eval_expr(node->a);

        if (!strcmp(node->text, "-")) return -value;
        if (!strcmp(node->text, "!")) return value == 0;
    }

    if (node->kind == AST_BINARY) {
        const char *op = node->text;
        double left = eval_expr(node->a);

        if (!strcmp(op, "&&"))
            return left != 0 && eval_expr(node->b) != 0;

        if (!strcmp(op, "||"))
            return left != 0 || eval_expr(node->b) != 0;

        double right = eval_expr(node->b);

        if (!strcmp(op, "+")) return left + right;
        if (!strcmp(op, "-")) return left - right;
        if (!strcmp(op, "*")) return left * right;

        if (!strcmp(op, "/")) {
            if (right == 0) runtime_error("ділення на нуль");
            if (expr_is_int(node->a) && expr_is_int(node->b))
                return (int)left / (int)right;
            return left / right;
        }

        if (!strcmp(op, "==")) return left == right;
        if (!strcmp(op, "!=")) return left != right;
        if (!strcmp(op, "<")) return left < right;
        if (!strcmp(op, "<=")) return left <= right;
        if (!strcmp(op, ">")) return left > right;
        if (!strcmp(op, ">=")) return left >= right;
    }

    runtime_error("невідомий тип виразу");
    return 0;
}

static double cast_value(const char *type, double value)
{
    if (!strcmp(type, "int")) return (int)value;
    return value;
}

static void execute(Ast *node);

static void execute_list(AstList *list)
{
    for (AstList *item = list; item; item = item->next)
        execute(item->item);
}

static void execute(Ast *node)
{
    if (!node) return;

    if (++execution_steps > 1000000)
        runtime_error("перевищено ліміт кроків; перевір цикл");

    switch (node->kind) {
        case AST_PROGRAM:
        case AST_BLOCK:
            execute_list(node->items);
            break;

        case AST_DECL: {
            if (find_variable(node->text))
                runtime_error("повторне оголошення змінної");

            double value = node->a ? eval_expr(node->a) : 0;

            Variable *v = calloc(1, sizeof(Variable));
            if (!v) runtime_error("не вистачило пам'яті");

            v->name = ast_strdup(node->text);
            v->type = ast_strdup(node->type);
            v->value = cast_value(node->type, value);
            v->next = variables;
            variables = v;
            break;
        }

        case AST_ASSIGN: {
            Variable *v = find_variable(node->text);
            if (!v) runtime_error("присвоєння змінній, яку не оголошено");
            v->value = cast_value(v->type, eval_expr(node->a));
            break;
        }

        case AST_PRINT:
            printf("%.10g\n", eval_expr(node->a));
            break;

        case AST_IF:
            if (eval_expr(node->a) != 0)
                execute(node->b);
            else if (node->c)
                execute(node->c);
            break;

        case AST_WHILE:
            while (eval_expr(node->a) != 0) {
                if (++execution_steps > 1000000)
                    runtime_error("перевищено ліміт кроків циклу");
                execute(node->b);
            }
            break;

        default:
            runtime_error("AST-вузол не є інструкцією");
    }
}

void ast_execute(Ast *root)
{
    execution_steps = 0;
    execute(root);

    while (variables) {
        Variable *next = variables->next;
        free(variables->name);
        free(variables->type);
        free(variables);
        variables = next;
    }
}


/* ---------- AST -> Python ---------- */

typedef struct PyType {
    char *name;
    char *type;
    struct PyType *next;
} PyType;

static PyType *python_types = NULL;

static const char *python_type_of(const char *name)
{
    for (PyType *item = python_types; item; item = item->next) {
        if (!strcmp(item->name, name)) return item->type;
    }
    return NULL;
}

static void python_add_type(const char *name, const char *type)
{
    PyType *item = calloc(1, sizeof(PyType));
    if (!item) runtime_error("не вистачило пам'яті");

    item->name = ast_strdup(name);
    item->type = ast_strdup(type);
    item->next = python_types;
    python_types = item;
}

static int python_expr_is_int(Ast *node)
{
    if (node->kind == AST_NUMBER)
        return !strcmp(node->type, "int");

    if (node->kind == AST_VARIABLE) {
        const char *type = python_type_of(node->text);
        return type && !strcmp(type, "int");
    }

    if (node->kind == AST_UNARY) {
        if (!strcmp(node->text, "!")) return 1;
        return python_expr_is_int(node->a);
    }

    if (node->kind == AST_BINARY) {
        const char *op = node->text;

        if (!strcmp(op, "==") || !strcmp(op, "!=") ||
            !strcmp(op, "<") || !strcmp(op, "<=") ||
            !strcmp(op, ">") || !strcmp(op, ">=") ||
            !strcmp(op, "&&") || !strcmp(op, "||")) {
            return 1;
        }

        return python_expr_is_int(node->a) &&
               python_expr_is_int(node->b);
    }

    return 0;
}

static void py_expr(FILE *file, Ast *node)
{
    if (node->kind == AST_NUMBER) {
        if (!strcmp(node->type, "float")) {
            fprintf(file, "%.10g", node->number);
            if (node->number == (long long)node->number)
                fputs(".0", file);
        } else {
            fprintf(file, "%d", (int)node->number);
        }
        return;
    }

    if (node->kind == AST_VARIABLE) {
        fputs(node->text, file);
        return;
    }

    if (node->kind == AST_UNARY) {
        if (!strcmp(node->text, "!")) {
            fputs("(not ", file);
            py_expr(file, node->a);
            fputc(')', file);
        } else {
            fputs("(-", file);
            py_expr(file, node->a);
            fputc(')', file);
        }
        return;
    }

    if (node->kind == AST_BINARY) {
        const char *op = node->text;

        if (!strcmp(op, "/") &&
            python_expr_is_int(node->a) &&
            python_expr_is_int(node->b)) {
            fputs("int((", file);
            py_expr(file, node->a);
            fputs(") / (", file);
            py_expr(file, node->b);
            fputs("))", file);
            return;
        }

        if (!strcmp(op, "&&")) op = "and";
        if (!strcmp(op, "||")) op = "or";

        fputc('(', file);
        py_expr(file, node->a);
        fprintf(file, " %s ", op);
        py_expr(file, node->b);
        fputc(')', file);
        return;
    }

    runtime_error("не вдалося згенерувати Python-вираз");
}

static void py_indent(FILE *file, int level)
{
    for (int i = 0; i < level; i++) fputs("    ", file);
}

static void py_stmt(FILE *file, Ast *node, int level);

static void py_list(FILE *file, AstList *list, int level)
{
    for (AstList *item = list; item; item = item->next)
        py_stmt(file, item->item, level);
}

static void py_body(FILE *file, Ast *node, int level)
{
    if (node->kind == AST_BLOCK) {
        if (!node->items) {
            py_indent(file, level);
            fputs("pass\n", file);
        } else {
            py_list(file, node->items, level);
        }
    } else {
        py_stmt(file, node, level);
    }
}

static void py_stmt(FILE *file, Ast *node, int level)
{
    py_indent(file, level);

    switch (node->kind) {
        case AST_DECL: {
            python_add_type(node->text, node->type);
            fprintf(file, "%s = %s(", node->text, node->type);

            if (node->a) py_expr(file, node->a);
            else if (!strcmp(node->type, "float")) fputs("0.0", file);
            else fputc('0', file);

            fputs(")\n", file);
            break;
        }

        case AST_ASSIGN: {
            const char *type = python_type_of(node->text);
            fprintf(file, "%s = ", node->text);
            if (type) fprintf(file, "%s(", type);
            py_expr(file, node->a);
            if (type) fputc(')', file);
            fputc('\n', file);
            break;
        }

        case AST_PRINT:
            fputs("print(", file);
            py_expr(file, node->a);
            fputs(")\n", file);
            break;

        case AST_BLOCK:
            if (node->items) py_list(file, node->items, level);
            else fputs("pass\n", file);
            break;

        case AST_IF:
            fputs("if ", file);
            py_expr(file, node->a);
            fputs(":\n", file);
            py_body(file, node->b, level + 1);

            if (node->c) {
                py_indent(file, level);
                fputs("else:\n", file);
                py_body(file, node->c, level + 1);
            }
            break;

        case AST_WHILE:
            fputs("while ", file);
            py_expr(file, node->a);
            fputs(":\n", file);
            py_body(file, node->b, level + 1);
            break;

        default:
            runtime_error("не вдалося згенерувати Python-інструкцію");
    }
}

void ast_write_python(FILE *file, Ast *root)
{
    fputs("def main():\n", file);

    if (root->items) py_list(file, root->items, 1);
    else fputs("    pass\n", file);

    fputs("\nif __name__ == \"__main__\":\n", file);
    fputs("    main()\n", file);

    while (python_types) {
        PyType *next = python_types->next;
        free(python_types->name);
        free(python_types->type);
        free(python_types);
        python_types = next;
    }
}