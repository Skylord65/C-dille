#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token current;

typedef struct {
    char name[64];
    VarType type;
} Symbol;

#define MAX_SYMBOLS 512
static Symbol symbol_table[MAX_SYMBOLS];
static int symbol_count = 0;

static void symbol_clear(void) {
    symbol_count = 0;
}

static void symbol_add(const char *name, VarType type) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            symbol_table[i].type = type;
            return;
        }
    }
    if (symbol_count < MAX_SYMBOLS) {
        strncpy(symbol_table[symbol_count].name, name, sizeof(symbol_table[symbol_count].name) - 1);
        symbol_table[symbol_count].name[sizeof(symbol_table[symbol_count].name) - 1] = '\0';
        symbol_table[symbol_count].type = type;
        symbol_count++;
    }
}

static VarType symbol_lookup(const char *name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return symbol_table[i].type;
        }
    }
    return TYPE_UNKNOWN;
}

static void advance(void) {
    current = lexer_next();
    if (current.type == TOKEN_ERREUR) {
        fprintf(stderr, "Arrêt du parsing suite à une erreur lexicale.\n");
        exit(EXIT_FAILURE);
    }
}

static void expect(TokenType type) {
    if (current.type != type) {
        fprintf(stderr, "Erreur syntaxique [ligne %d, col %d] : attendu '%s', obtenu '%s' (\"%s\")\n",
                current.ligne, current.colonne, token_type_name(type), token_type_name(current.type), current.valeur);
        exit(EXIT_FAILURE);
    }
    advance();
}

static Node *node_new(NodeType type) {
    Node *n = (Node *)calloc(1, sizeof(Node));
    if (!n) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    n->type = type;
    return n;
}

static void block_append(Block *b, Node *n) {
    if (b->count >= b->capacity) {
        b->capacity = (b->capacity == 0) ? 8 : b->capacity * 2;
        b->statements = (Node **)realloc(b->statements, b->capacity * sizeof(Node *));
        if (!b->statements) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
    }
    b->statements[b->count++] = n;
}

static void free_block(Block *b) {
    if (!b || !b->statements) return;
    for (int i = 0; i < b->count; i++) {
        Node *n = b->statements[i];
        if (!n) continue;
        if (n->type == NODE_SI) {
            free_block(&n->si.then_block);
            if (n->si.has_else) {
                free_block(&n->si.else_block);
            }
        } else if (n->type == NODE_TANT_QUE) {
            free_block(&n->tant_que.body);
        }
        free(n);
    }
    free(b->statements);
    b->statements = NULL;
    b->count = 0;
    b->capacity = 0;
}

void free_program(Program *prog) {
    if (prog) {
        free_block(&prog->root);
    }
}

const char *var_type_to_c(VarType type) {
    switch (type) {
    case TYPE_ENTIER: return "int";
    case TYPE_REEL: return "double";
    case TYPE_CHAINE: return "char";
    case TYPE_BOOLEEN: return "bool";
    default: return "int";
    }
}

// Prototypes de l'analyseur d'expressions
static void parse_expression(char *out, size_t max_len, VarType *out_type);
static void parse_or(char *out, size_t max_len, VarType *out_type);
static void parse_and(char *out, size_t max_len, VarType *out_type);
static void parse_equality(char *out, size_t max_len, VarType *out_type);
static void parse_relational(char *out, size_t max_len, VarType *out_type);
static void parse_additive(char *out, size_t max_len, VarType *out_type);
static void parse_multiplicative(char *out, size_t max_len, VarType *out_type);
static void parse_unary(char *out, size_t max_len, VarType *out_type);
static void parse_primary(char *out, size_t max_len, VarType *out_type);

static void parse_expression(char *out, size_t max_len, VarType *out_type) {
    parse_or(out, max_len, out_type);
}

static void parse_or(char *out, size_t max_len, VarType *out_type) {
    char left[MAX_EXPR_LEN];
    VarType lt = TYPE_UNKNOWN;
    parse_and(left, sizeof(left), &lt);

    while (current.type == TOKEN_OU) {
        advance();
        char right[MAX_EXPR_LEN];
        VarType rt = TYPE_UNKNOWN;
        parse_and(right, sizeof(right), &rt);
        char temp[MAX_EXPR_LEN * 2 + 16];
        snprintf(temp, sizeof(temp), "(%s || %s)", left, right);
        strncpy(left, temp, sizeof(left) - 1);
        left[sizeof(left) - 1] = '\0';
        lt = TYPE_BOOLEEN;
    }

    strncpy(out, left, max_len - 1);
    out[max_len - 1] = '\0';
    if (out_type) *out_type = lt;
}

static void parse_and(char *out, size_t max_len, VarType *out_type) {
    char left[MAX_EXPR_LEN];
    VarType lt = TYPE_UNKNOWN;
    parse_equality(left, sizeof(left), &lt);

    while (current.type == TOKEN_ET) {
        advance();
        char right[MAX_EXPR_LEN];
        VarType rt = TYPE_UNKNOWN;
        parse_equality(right, sizeof(right), &rt);
        char temp[MAX_EXPR_LEN * 2 + 16];
        snprintf(temp, sizeof(temp), "(%s && %s)", left, right);
        strncpy(left, temp, sizeof(left) - 1);
        left[sizeof(left) - 1] = '\0';
        lt = TYPE_BOOLEEN;
    }

    strncpy(out, left, max_len - 1);
    out[max_len - 1] = '\0';
    if (out_type) *out_type = lt;
}

static void parse_equality(char *out, size_t max_len, VarType *out_type) {
    char left[MAX_EXPR_LEN];
    VarType lt = TYPE_UNKNOWN;
    parse_relational(left, sizeof(left), &lt);

    while (current.type == TOKEN_EGAL || current.type == TOKEN_DIFF) {
        const char *op = (current.type == TOKEN_EGAL) ? "==" : "!=";
        advance();
        char right[MAX_EXPR_LEN];
        VarType rt = TYPE_UNKNOWN;
        parse_relational(right, sizeof(right), &rt);
        char temp[MAX_EXPR_LEN * 2 + 16];
        snprintf(temp, sizeof(temp), "(%s %s %s)", left, op, right);
        strncpy(left, temp, sizeof(left) - 1);
        left[sizeof(left) - 1] = '\0';
        lt = TYPE_BOOLEEN;
    }

    strncpy(out, left, max_len - 1);
    out[max_len - 1] = '\0';
    if (out_type) *out_type = lt;
}

static void parse_relational(char *out, size_t max_len, VarType *out_type) {
    char left[MAX_EXPR_LEN];
    VarType lt = TYPE_UNKNOWN;
    parse_additive(left, sizeof(left), &lt);

    while (current.type == TOKEN_INF || current.type == TOKEN_SUP ||
           current.type == TOKEN_INF_EGAL || current.type == TOKEN_SUP_EGAL) {
        const char *op = "<";
        if (current.type == TOKEN_SUP) op = ">";
        else if (current.type == TOKEN_INF_EGAL) op = "<=";
        else if (current.type == TOKEN_SUP_EGAL) op = ">=";

        advance();
        char right[MAX_EXPR_LEN];
        VarType rt = TYPE_UNKNOWN;
        parse_additive(right, sizeof(right), &rt);
        char temp[MAX_EXPR_LEN * 2 + 16];
        snprintf(temp, sizeof(temp), "(%s %s %s)", left, op, right);
        strncpy(left, temp, sizeof(left) - 1);
        left[sizeof(left) - 1] = '\0';
        lt = TYPE_BOOLEEN;
    }

    strncpy(out, left, max_len - 1);
    out[max_len - 1] = '\0';
    if (out_type) *out_type = lt;
}

static void parse_additive(char *out, size_t max_len, VarType *out_type) {
    char left[MAX_EXPR_LEN];
    VarType lt = TYPE_UNKNOWN;
    parse_multiplicative(left, sizeof(left), &lt);

    while (current.type == TOKEN_PLUS || current.type == TOKEN_MOINS) {
        const char *op = (current.type == TOKEN_PLUS) ? "+" : "-";
        advance();
        char right[MAX_EXPR_LEN];
        VarType rt = TYPE_UNKNOWN;
        parse_multiplicative(right, sizeof(right), &rt);
        char temp[MAX_EXPR_LEN * 2 + 16];
        snprintf(temp, sizeof(temp), "(%s %s %s)", left, op, right);
        strncpy(left, temp, sizeof(left) - 1);
        left[sizeof(left) - 1] = '\0';
        if (rt == TYPE_REEL) lt = TYPE_REEL;
    }

    strncpy(out, left, max_len - 1);
    out[max_len - 1] = '\0';
    if (out_type) *out_type = lt;
}

static void parse_multiplicative(char *out, size_t max_len, VarType *out_type) {
    char left[MAX_EXPR_LEN];
    VarType lt = TYPE_UNKNOWN;
    parse_unary(left, sizeof(left), &lt);

    while (current.type == TOKEN_ETOILE || current.type == TOKEN_SLASH || current.type == TOKEN_POURCENT) {
        const char *op = "*";
        if (current.type == TOKEN_SLASH) op = "/";
        else if (current.type == TOKEN_POURCENT) op = "%";

        advance();
        char right[MAX_EXPR_LEN];
        VarType rt = TYPE_UNKNOWN;
        parse_unary(right, sizeof(right), &rt);
        char temp[MAX_EXPR_LEN * 2 + 16];
        snprintf(temp, sizeof(temp), "(%s %s %s)", left, op, right);
        strncpy(left, temp, sizeof(left) - 1);
        left[sizeof(left) - 1] = '\0';
        if (rt == TYPE_REEL) lt = TYPE_REEL;
    }

    strncpy(out, left, max_len - 1);
    out[max_len - 1] = '\0';
    if (out_type) *out_type = lt;
}

static void parse_unary(char *out, size_t max_len, VarType *out_type) {
    if (current.type == TOKEN_MOINS) {
        advance();
        char inner[MAX_EXPR_LEN];
        VarType it = TYPE_UNKNOWN;
        parse_unary(inner, sizeof(inner), &it);
        snprintf(out, max_len, "(-%s)", inner);
        if (out_type) *out_type = it;
        return;
    }
    if (current.type == TOKEN_PLUS) {
        advance();
        parse_unary(out, max_len, out_type);
        return;
    }
    if (current.type == TOKEN_NON) {
        advance();
        char inner[MAX_EXPR_LEN];
        VarType it = TYPE_UNKNOWN;
        parse_unary(inner, sizeof(inner), &it);
        snprintf(out, max_len, "(!%s)", inner);
        if (out_type) *out_type = TYPE_BOOLEEN;
        return;
    }
    parse_primary(out, max_len, out_type);
}

static void parse_primary(char *out, size_t max_len, VarType *out_type) {
    if (current.type == TOKEN_NOMBRE) {
        snprintf(out, max_len, "%s", current.valeur);
        if (out_type) *out_type = TYPE_ENTIER;
        advance();
        return;
    }
    if (current.type == TOKEN_NOMBRE_REEL) {
        snprintf(out, max_len, "%s", current.valeur);
        if (out_type) *out_type = TYPE_REEL;
        advance();
        return;
    }
    if (current.type == TOKEN_VRAI) {
        snprintf(out, max_len, "true");
        if (out_type) *out_type = TYPE_BOOLEEN;
        advance();
        return;
    }
    if (current.type == TOKEN_FAUX) {
        snprintf(out, max_len, "false");
        if (out_type) *out_type = TYPE_BOOLEEN;
        advance();
        return;
    }
    if (current.type == TOKEN_STRING) {
        snprintf(out, max_len, "\"%s\"", current.valeur);
        if (out_type) *out_type = TYPE_CHAINE;
        advance();
        return;
    }
    if (current.type == TOKEN_IDENT) {
        snprintf(out, max_len, "%s", current.valeur);
        if (out_type) *out_type = symbol_lookup(current.valeur);
        advance();
        return;
    }
    if (current.type == TOKEN_PAREN_OUVR) {
        advance();
        char inner[MAX_EXPR_LEN];
        parse_expression(inner, sizeof(inner), out_type);
        expect(TOKEN_PAREN_FERM);
        snprintf(out, max_len, "(%s)", inner);
        return;
    }

    fprintf(stderr, "Erreur syntaxique [ligne %d, col %d] : expression attendue, obtenu '%s' (\"%s\")\n",
            current.ligne, current.colonne, token_type_name(current.type), current.valeur);
    exit(EXIT_FAILURE);
}

static int is_si_end(TokenType t) {
    return (t == TOKEN_SINON || t == TOKEN_FIN_SI || t == TOKEN_FIN);
}

static int is_fin_si(TokenType t) {
    return (t == TOKEN_FIN_SI || t == TOKEN_FIN);
}

static int is_tant_que_end(TokenType t) {
    return (t == TOKEN_FIN_TANT_QUE || t == TOKEN_FIN);
}

static int is_root_end(TokenType t) {
    return (t == TOKEN_FIN);
}

static void parse_block(Block *block, int (*is_end)(TokenType)) {
    while (!is_end(current.type) && current.type != TOKEN_FIN) {
        if (current.type == TOKEN_POINT_VIRGULE) {
            advance();
            continue;
        }

        // 1. Déclaration de variables
        if (current.type == TOKEN_TYPE_ENTIER || current.type == TOKEN_TYPE_REEL ||
            current.type == TOKEN_TYPE_CHAINE || current.type == TOKEN_TYPE_BOOLEEN) {
            VarType vt = TYPE_ENTIER;
            if (current.type == TOKEN_TYPE_REEL) vt = TYPE_REEL;
            else if (current.type == TOKEN_TYPE_CHAINE) vt = TYPE_CHAINE;
            else if (current.type == TOKEN_TYPE_BOOLEEN) vt = TYPE_BOOLEEN;
            advance();

            while (1) {
                if (current.type != TOKEN_IDENT) {
                    fprintf(stderr, "Erreur syntaxique [ligne %d, col %d] : nom de variable attendu dans la déclaration\n",
                            current.ligne, current.colonne);
                    exit(EXIT_FAILURE);
                }

                Node *node = node_new(NODE_DECLARATION);
                node->decl.var_type = vt;
                strncpy(node->decl.nom, current.valeur, sizeof(node->decl.nom) - 1);
                node->decl.nom[sizeof(node->decl.nom) - 1] = '\0';
                symbol_add(node->decl.nom, vt);
                advance();

                if (current.type == TOKEN_AFFECT) {
                    advance();
                    VarType it = TYPE_UNKNOWN;
                    parse_expression(node->decl.init_expr, sizeof(node->decl.init_expr), &it);
                } else {
                    node->decl.init_expr[0] = '\0';
                }

                block_append(block, node);

                if (current.type == TOKEN_VIRGULE) {
                    advance();
                } else {
                    break;
                }
            }
            continue;
        }

        // 2. Affectation de variable
        if (current.type == TOKEN_IDENT) {
            char name[64];
            strncpy(name, current.valeur, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
            advance();

            expect(TOKEN_AFFECT);

            Node *node = node_new(NODE_ASSIGNATION);
            strncpy(node->assign.nom, name, sizeof(node->assign.nom) - 1);
            node->assign.nom[sizeof(node->assign.nom) - 1] = '\0';

            VarType et = TYPE_UNKNOWN;
            parse_expression(node->assign.expr, sizeof(node->assign.expr), &et);

            block_append(block, node);
            continue;
        }

        // 3. Affichage : Imprimer ... ou Afficher ...
        if (current.type == TOKEN_IMPRIMER) {
            advance();

            Node *node = node_new(NODE_PRINT);
            node->print.item_count = 0;

            while (1) {
                if (node->print.item_count >= (int)(sizeof(node->print.items) / sizeof(node->print.items[0]))) {
                    fprintf(stderr, "Erreur : trop d'arguments pour Imprimer (maximum 32)\n");
                    exit(EXIT_FAILURE);
                }

                PrintItem *item = &node->print.items[node->print.item_count++];

                if (current.type == TOKEN_STRING) {
                    item->is_string_literal = 1;
                    item->expr_type = TYPE_CHAINE;
                    strncpy(item->text, current.valeur, sizeof(item->text) - 1);
                    item->text[sizeof(item->text) - 1] = '\0';
                    advance();
                } else {
                    item->is_string_literal = 0;
                    parse_expression(item->text, sizeof(item->text), &item->expr_type);
                }

                if (current.type == TOKEN_VIRGULE) {
                    advance();
                } else {
                    break;
                }
            }

            block_append(block, node);
            continue;
        }

        // 4. Saisie : Lire / Saisir
        if (current.type == TOKEN_LIRE) {
            advance();

            if (current.type != TOKEN_IDENT) {
                fprintf(stderr, "Erreur syntaxique [ligne %d, col %d] : identifiant attendu après Lire\n",
                        current.ligne, current.colonne);
                exit(EXIT_FAILURE);
            }

            Node *node = node_new(NODE_LIRE);
            strncpy(node->lire.nom, current.valeur, sizeof(node->lire.nom) - 1);
            node->lire.nom[sizeof(node->lire.nom) - 1] = '\0';
            node->lire.var_type = symbol_lookup(node->lire.nom);
            advance();

            block_append(block, node);
            continue;
        }

        // 5. Condition : Si condition alors ... [sinon ...] fin si
        if (current.type == TOKEN_SI) {
            advance();

            Node *node = node_new(NODE_SI);
            node->si.has_else = 0;

            VarType ct = TYPE_UNKNOWN;
            parse_expression(node->si.condition, sizeof(node->si.condition), &ct);

            expect(TOKEN_ALORS);

            parse_block(&node->si.then_block, is_si_end);

            if (current.type == TOKEN_SINON) {
                advance();
                node->si.has_else = 1;
                parse_block(&node->si.else_block, is_fin_si);
            }

            expect(TOKEN_FIN_SI);

            block_append(block, node);
            continue;
        }

        // 6. Boucle : Tant que condition faire ... fin tant que
        if (current.type == TOKEN_TANT_QUE) {
            advance();

            Node *node = node_new(NODE_TANT_QUE);

            VarType ct = TYPE_UNKNOWN;
            parse_expression(node->tant_que.condition, sizeof(node->tant_que.condition), &ct);

            expect(TOKEN_FAIRE);

            parse_block(&node->tant_que.body, is_tant_que_end);

            expect(TOKEN_FIN_TANT_QUE);

            block_append(block, node);
            continue;
        }

        fprintf(stderr, "Erreur syntaxique [ligne %d, col %d] : instruction inattendue '%s' (\"%s\")\n",
                current.ligne, current.colonne, token_type_name(current.type), current.valeur);
        exit(EXIT_FAILURE);
    }
}

void parser_init(void) {
    symbol_clear();
    advance();
}

Program parse_program(void) {
    Program prog;
    memset(&prog, 0, sizeof(Program));
    parse_block(&prog.root, is_root_end);
    return prog;
}
