#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#define MAX_NODES 1024

typedef enum {
    NODE_DECLARATION,
    NODE_ASSIGNATION,
    NODE_PRINT
} NodeType;

typedef struct {
    NodeType type;
    char nom[64];
    char valeur[256];
} Node;

typedef struct {
    Node nodes[MAX_NODES];
    int count;
} Program;

void parser_init(void);

Program parse_program(void);

#endif