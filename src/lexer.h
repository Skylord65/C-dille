#ifndef LEXER_H
#define LEXER_H


typedef enum {
    TOKEN_FIN,
    TOKEN_AFFICHER,
    TOKEN_ENTIER,
    TOKEN_IDENT,
    TOKEN_STRING,
    TOKEN_NOMBRE,
    TOKEN_AFFECT
} TokenType;

typedef struct {
    TokenType type;
    char valeur[256];
} Token;

void lexer_init(char *source);

Token lexer_next();

#endif