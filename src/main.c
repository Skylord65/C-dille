#include "lexer.h"
#include "parser.h"
#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog_name) {
    printf("C-dille - Transpileur de pseudo-code français vers C\n");
    printf("Usage : %s <fichier.cll> [-o <executable>] [-r|--run]\n", prog_name);
    printf("Options :\n");
    printf("  -o <nom>    Nom du binaire généré (défaut: programme)\n");
    printf("  -r, --run   Exécute le programme immédiatement après la compilation\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *input_file = NULL;
    const char *output_bin = "programme";
    int run_after = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--run") == 0) {
            run_after = 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_bin = argv[++i];
            } else {
                fprintf(stderr, "Erreur : argument manquant après -o\n");
                return 1;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Option inconnue : %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            if (input_file == NULL) {
                input_file = argv[i];
            } else {
                fprintf(stderr, "Erreur : un seul fichier source peut être fourni\n");
                return 1;
            }
        }
    }

    if (input_file == NULL) {
        fprintf(stderr, "Erreur : aucun fichier source .cll fourni\n");
        print_usage(argv[0]);
        return 1;
    }

    FILE *f = fopen(input_file, "rb");
    if (f == NULL) {
        perror("Erreur ouverture fichier source");
        return 1;
    }

    size_t capacity = 4096;
    size_t taille = 0;
    char *code = (char *)malloc(capacity);
    if (code == NULL) {
        perror("malloc");
        fclose(f);
        return 1;
    }

    size_t n;
    while ((n = fread(code + taille, 1, capacity - taille - 1, f)) > 0) {
        taille += n;
        if (capacity - taille <= 1) {
            capacity *= 2;
            char *new_code = (char *)realloc(code, capacity);
            if (!new_code) {
                perror("realloc");
                free(code);
                fclose(f);
                return 1;
            }
            code = new_code;
        }
    }
    code[taille] = '\0';
    fclose(f);

    lexer_init(code);
    parser_init();

    Program prog = parse_program();
    printf("Programme analysé avec succès (%d instruction(s) racine).\n", prog.root.count);

    const char *output_c = "output.c";
    generate_c(&prog, output_c);
    printf("Code C généré : %s\n", output_c);

    char gcc_cmd[512];
    snprintf(gcc_cmd, sizeof(gcc_cmd), "gcc %s -o %s", output_c, output_bin);
    int result = system(gcc_cmd);

    if (result != 0) {
        fprintf(stderr, "Erreur lors de la compilation GCC\n");
        free_program(&prog);
        free(code);
        return 1;
    }

    printf("Exécutable généré : %s\n", output_bin);

    if (run_after) {
        printf("--- Exécution du programme ---\n");
        char run_cmd[512];
        snprintf(run_cmd, sizeof(run_cmd), "./%s", output_bin);
        system(run_cmd);
    }

    free_program(&prog);
    free(code);

    return 0;
}
