#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#define MAX_EXPR_LEN 1024

typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_ENTIER,
    TYPE_REEL,
    TYPE_CHAINE,
    TYPE_BOOLEEN
} VarType;

typedef enum {
    NODE_DECLARATION,
    NODE_ASSIGNATION,
    NODE_PRINT,
    NODE_LIRE,
    NODE_SI,
    NODE_TANT_QUE
} NodeType;

typedef struct PrintItem {
    int is_string_literal;
    char text[MAX_EXPR_LEN];
    VarType expr_type;
} PrintItem;

typedef struct Node Node;

typedef struct Block {
    Node **statements;
    int count;
    int capacity;
} Block;

struct Node {
    NodeType type;
    union {
        struct {
            VarType var_type;
            char nom[64];
            char init_expr[MAX_EXPR_LEN];
        } decl;

        struct {
            char nom[64];
            char expr[MAX_EXPR_LEN];
        } assign;

        struct {
            PrintItem items[32];
            int item_count;
        } print;

        struct {
            char nom[64];
            VarType var_type;
        } lire;

        struct {
            char condition[MAX_EXPR_LEN];
            Block then_block;
            Block else_block;
            int has_else;
        } si;

        struct {
            char condition[MAX_EXPR_LEN];
            Block body;
        } tant_que;
    };
};

typedef struct {
    Block root;
} Program;

void parser_init(void);
Program parse_program(void);
void free_program(Program *prog);
const char *var_type_to_c(VarType type);

#endif
