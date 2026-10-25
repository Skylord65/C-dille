#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void escape_string(char *dest, size_t max, const char *src) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < max - 2; i++) {
        if (src[i] == '"') {
            dest[j++] = '\\';
            dest[j++] = '"';
        } else if (src[i] == '\\') {
            dest[j++] = '\\';
            dest[j++] = '\\';
        } else if (src[i] == '\n') {
            dest[j++] = '\\';
            dest[j++] = 'n';
        } else if (src[i] == '\t') {
            dest[j++] = '\\';
            dest[j++] = 't';
        } else {
            dest[j++] = src[i];
        }
    }
    dest[j] = '\0';
}

static void generate_block(FILE *out, Block *b, int indent) {
    if (!b) return;

    for (int i = 0; i < b->count; i++) {
        Node *n = b->statements[i];
        if (!n) continue;

        switch (n->type) {
        case NODE_DECLARATION:
            if (n->decl.var_type == TYPE_CHAINE) {
                if (n->decl.init_expr[0] != '\0') {
                    fprintf(out, "%*schar %s[256] = %s;\n", indent, "", n->decl.nom, n->decl.init_expr);
                } else {
                    fprintf(out, "%*schar %s[256] = \"\";\n", indent, "", n->decl.nom);
                }
            } else if (n->decl.var_type == TYPE_REEL) {
                if (n->decl.init_expr[0] != '\0') {
                    fprintf(out, "%*sdouble %s = %s;\n", indent, "", n->decl.nom, n->decl.init_expr);
                } else {
                    fprintf(out, "%*sdouble %s = 0.0;\n", indent, "", n->decl.nom);
                }
            } else if (n->decl.var_type == TYPE_BOOLEEN) {
                if (n->decl.init_expr[0] != '\0') {
                    fprintf(out, "%*sbool %s = %s;\n", indent, "", n->decl.nom, n->decl.init_expr);
                } else {
                    fprintf(out, "%*sbool %s = false;\n", indent, "", n->decl.nom);
                }
            } else { // TYPE_ENTIER
                if (n->decl.init_expr[0] != '\0') {
                    fprintf(out, "%*sint %s = %s;\n", indent, "", n->decl.nom, n->decl.init_expr);
                } else {
                    fprintf(out, "%*sint %s = 0;\n", indent, "", n->decl.nom);
                }
            }
            break;

        case NODE_ASSIGNATION:
            fprintf(out, "%*s%s = %s;\n", indent, "", n->assign.nom, n->assign.expr);
            break;

        case NODE_PRINT: {
            if (n->print.item_count == 0) {
                fprintf(out, "%*sprintf(\"\\n\");\n", indent, "");
                break;
            }

            for (int j = 0; j < n->print.item_count; j++) {
                PrintItem *item = &n->print.items[j];
                int is_last = (j == n->print.item_count - 1);

                if (item->is_string_literal) {
                    char escaped[1024];
                    escape_string(escaped, sizeof(escaped), item->text);
                    if (is_last) {
                        fprintf(out, "%*sprintf(\"%%s\\n\", \"%s\");\n", indent, "", escaped);
                    } else {
                        fprintf(out, "%*sprintf(\"%%s\", \"%s\");\n", indent, "", escaped);
                    }
                } else {
                    if (item->expr_type == TYPE_CHAINE) {
                        if (is_last) {
                            fprintf(out, "%*sprintf(\"%%s\\n\", %s);\n", indent, "", item->text);
                        } else {
                            fprintf(out, "%*sprintf(\"%%s\", %s);\n", indent, "", item->text);
                        }
                    } else if (item->expr_type == TYPE_REEL) {
                        if (is_last) {
                            fprintf(out, "%*sprintf(\"%%g\\n\", (double)(%s));\n", indent, "", item->text);
                        } else {
                            fprintf(out, "%*sprintf(\"%%g\", (double)(%s));\n", indent, "", item->text);
                        }
                    } else { // TYPE_ENTIER ou TYPE_BOOLEEN
                        if (is_last) {
                            fprintf(out, "%*sprintf(\"%%d\\n\", (int)(%s));\n", indent, "", item->text);
                        } else {
                            fprintf(out, "%*sprintf(\"%%d\", (int)(%s));\n", indent, "", item->text);
                        }
                    }
                }
            }
            break;
        }

        case NODE_LIRE:
            if (n->lire.var_type == TYPE_CHAINE) {
                fprintf(out, "%*sscanf(\"%%255s\", %s);\n", indent, "", n->lire.nom);
            } else if (n->lire.var_type == TYPE_REEL) {
                fprintf(out, "%*sscanf(\"%%lf\", &%s);\n", indent, "", n->lire.nom);
            } else {
                fprintf(out, "%*sscanf(\"%%d\", &%s);\n", indent, "", n->lire.nom);
            }
            break;

        case NODE_SI:
            fprintf(out, "%*sif (%s) {\n", indent, "", n->si.condition);
            generate_block(out, &n->si.then_block, indent + 4);
            if (n->si.has_else) {
                fprintf(out, "%*s} else {\n", indent, "");
                generate_block(out, &n->si.else_block, indent + 4);
            }
            fprintf(out, "%*s}\n", indent, "");
            break;

        case NODE_TANT_QUE:
            fprintf(out, "%*swhile (%s) {\n", indent, "", n->tant_que.condition);
            generate_block(out, &n->tant_que.body, indent + 4);
            fprintf(out, "%*s}\n", indent, "");
            break;
        }
    }
}

void generate_c(Program *prog, const char *filename) {
    FILE *out = fopen(filename, "w");
    if (!out) {
        perror("Erreur ouverture fichier de sortie C");
        exit(EXIT_FAILURE);
    }

    fprintf(out, "/* Code C genere par C-dille */\n");
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <stdbool.h>\n");
    fprintf(out, "#include <string.h>\n\n");
    fprintf(out, "int main(void)\n{\n");

    generate_block(out, &prog->root, 4);

    fprintf(out, "\n    return 0;\n}\n");
    fclose(out);
}
