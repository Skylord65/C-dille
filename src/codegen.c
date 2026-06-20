#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>


void generate_c(Program *prog, const char *filename) {
    FILE *out = fopen(filename, "w");
    if(out == NULL) {
        perror("Erreur ouverture fichier");
        exit(EXIT_FAILURE);
    }

    fprintf(out,"#include <stdio.h>\n\nint main(void)\n{\n");

    for(int i = 0; i < prog->count; i++) {
        Node *n = &prog->nodes[i];
        switch(n->type) {
        case NODE_DECLARATION:
            fprintf(out, "int %s;\n", n->nom);
            break;
        case NODE_ASSIGNATION:
            fprintf(out, "%s = %s;\n", n->nom, n->valeur);
            break;
        case NODE_PRINT:
            fprintf(out, "printf(\"%%s\\n\", \"%s\");\n", n->valeur);
            break;
        }
    }

    fprintf(out,"\nreturn 0;\n}\n");

    fclose(out);
}