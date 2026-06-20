#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

int main(int argc, char const *argv[])
{
    if(argc!=2) {
        printf("Usage : compiler fichier.cll");
        return 1;
    }

    FILE* f = fopen(argv[1], "r");

    if(f == NULL) {
        perror("Erreur dans l'ouverture du fichier");
        return 1;
    }

    FILE* o = fopen("output.c", "w");
    
    fprintf(o, "#include <stdio.h>\n\n int main()\n{\n");

    char* l = NULL;
    size_t n;
    while (getline(&l,&n, f)!=EOF) {
        char commande[n];
        char texte[n];

        if(sscanf(l, "%s %[^\n]", commande, texte)==2) {
            if(strcmp(commande, "Imprimer")==0) {
                fprintf(o, "printf(\"%s\\n\");\n", texte);
            }
            
        }
    }

    fprintf(o, "return 0;\n}\n");

    fclose(f);
    fclose(o);

    system("gcc output.c -o executable");
    return 0;
}
