#include "lexer.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

char *source;
int position;

void lexer_init(char *src) {
    source = src;
    position = 0;
}

Token lexer_next() {
    Token token;
    token.type = TOKEN_FIN;
    token.valeur[0] = '\0';
    
    while(source[position]==' ' || source[position]=='\n' || source[position]=='\t') position++;

    char c = source[position];

    if(c == '\0') {
        token.type = TOKEN_FIN;
        return token;
    }
    
    if(c == '"') {
        position++;
        int i=0;
        while(source[position]!='"' && source[position]!='\0') token.valeur[i++] = source[position++];
        token.valeur[i]='\0';
        if(source[position]=='"') position++;
        token.type = TOKEN_STRING;
        return token;
    }

    if(isdigit(c)) {
        int i=0;
        while(isdigit(source[position])) token.valeur[i++] = source[position++];
        token.valeur[i]='\0';
        token.type = TOKEN_NOMBRE;
        return token;
    }

    if(isalpha(c)) {
        int i=0;
        while(isalnum(source[position])) token.valeur[i++] = source[position++];
        token.valeur[i]='\0';
        if(strcmp(token.valeur,"Imprimer")==0) token.type = TOKEN_AFFICHER;
        else if(strcmp(token.valeur,"Entier")==0) token.type = TOKEN_ENTIER;
        else token.type = TOKEN_IDENT;
        return token;
    }

    if(c=='<' && source[position++]=='-') {
        position++;
        position++;
        token.type = TOKEN_AFFECT;
        return token;
    }

    printf("Caractere inconnu : %c\n",c);
    position++;
    return token;
}