#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static Token current;

static void advance(void) {
    current = lexer_next();
}

static void expect(TokenType type) {
    if(current.type != type) {
        printf("Erreur syntaxique : attendu %d, obtenu %d\n", type, current.type);
        exit(EXIT_FAILURE);
    }
    advance();
}

static Node parse_declaration(void) {
    Node node;
    memset(&node, 0, sizeof(Node));
    node.type = NODE_DECLARATION;
    expect(TOKEN_ENTIER);
    strcpy(node.nom, current.valeur);
    expect(TOKEN_IDENT);
    return node;
}

static Node parse_print(void) {
    Node node;
    memset(&node, 0, sizeof(Node));
    node.type = NODE_PRINT;
    expect(TOKEN_AFFICHER);
    strcpy(node.valeur, current.valeur);
    expect(TOKEN_STRING);
    return node;
}

static Node parse_assignation(void) {
    Node node;
    memset(&node, 0, sizeof(Node));
    node.type = NODE_ASSIGNATION;
    strcpy(node.nom, current.valeur);
    expect(TOKEN_IDENT);
    expect(TOKEN_AFFECT);
    if(current.type != TOKEN_NOMBRE && current.type != TOKEN_IDENT) {
        printf("Valeur attendue\n");
        exit(EXIT_FAILURE);
    }
    strcpy(node.valeur, current.valeur);
    advance();
    return node;
}

static Node parse_statement(void) {
    switch(current.type) {
    case TOKEN_ENTIER:
        return parse_declaration();
    case TOKEN_AFFICHER:
        return parse_print();
    case TOKEN_IDENT:
        return parse_assignation();
    default:
        printf("Instruction inconnue\n");
        exit(EXIT_FAILURE);
    }
}

void parser_init(void) {
    advance();
}

Program parse_program(void) {
    Program prog;
    prog.count = 0;
    while(current.type != TOKEN_FIN) {
        prog.nodes[prog.count++] = parse_statement();
    }
    return prog;
}