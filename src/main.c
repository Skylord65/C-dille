#include "lexer.h"
#include "parser.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("Usage : %s fichier.mlg\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");

    if(f == NULL) {
        perror("Erreur ouverture fichier");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long taille = ftell(f);
    rewind(f);

    char *code = malloc(taille + 1);

    if(code == NULL) {
        perror("malloc");
        fclose(f);
        return 1;
    }
    fread(code, 1, taille, f);
    code[taille] = '\0';
    fclose(f);

    lexer_init(code);

    parser_init();

    Program prog = parse_program();
    printf("Programme parse avec %d instruction(s)\n", prog.count);

    generate_c(&prog, "output.c");
    printf("Code C genere : output.c\n");

    int result = system("gcc output.c -o programme");

    if(result != 0) {
        printf("Erreur compilation GCC\n");
        free(code);
        return 1;
    }

    printf("Executable genere : programme\n");
    free(code);

    return 0;
}