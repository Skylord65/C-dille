#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

static const char *source = NULL;
static int position = 0;
static int current_line = 1;
static int current_col = 1;

void lexer_init(const char *src) {
    source = src;
    position = 0;
    current_line = 1;
    current_col = 1;
}

static char peek(void) {
    return source[position];
}

static char advance(void) {
    char c = source[position++];
    if (c == '\n') {
        current_line++;
        current_col = 1;
    } else {
        current_col++;
    }
    return c;
}

static int is_ident_start(unsigned char c) {
    return isalpha(c) || c == '_' || c >= 0x80;
}

static int is_ident_part(unsigned char c) {
    return isalnum(c) || c == '_' || c >= 0x80;
}

static void skip_whitespace_and_comments(void) {
    while (source[position] != '\0') {
        unsigned char c = (unsigned char)source[position];

        // Espaces et retours à la ligne
        if (isspace(c)) {
            advance();
            continue;
        }

        // Commentaires mono-ligne // ou #
        if ((c == '/' && source[position + 1] == '/') || c == '#') {
            while (source[position] != '\0' && source[position] != '\n') {
                advance();
            }
            continue;
        }

        // Commentaires multi-lignes /* ... */
        if (c == '/' && source[position + 1] == '*') {
            advance(); // '/'
            advance(); // '*'
            while (source[position] != '\0') {
                if (source[position] == '*' && source[position + 1] == '/') {
                    advance(); // '*'
                    advance(); // '/'
                    break;
                }
                advance();
            }
            continue;
        }

        break;
    }
}

// Vérifie si le mot-clé kw suit (avec espaces avant)
static int peek_keyword(int start_pos, const char *kw, int *out_lines, int *out_cols) {
    int pos = start_pos;
    int extra_lines = 0;
    int extra_cols = 0;

    while (source[pos] != '\0' && (source[pos] == ' ' || source[pos] == '\t' || source[pos] == '\r' || source[pos] == '\n')) {
        if (source[pos] == '\n') {
            extra_lines++;
            extra_cols = 1;
        } else {
            extra_cols++;
        }
        pos++;
    }

    size_t kw_len = strlen(kw);
    if (strncasecmp(&source[pos], kw, kw_len) == 0) {
        unsigned char next_c = (unsigned char)source[pos + kw_len];
        if (!is_ident_part(next_c)) {
            if (out_lines) *out_lines = extra_lines;
            if (out_cols) *out_cols = extra_cols + (int)kw_len;
            return pos + (int)kw_len;
        }
    }

    return -1;
}

Token lexer_next(void) {
    skip_whitespace_and_comments();

    Token token;
    token.type = TOKEN_FIN;
    token.valeur[0] = '\0';
    token.ligne = current_line;
    token.colonne = current_col;

    if (source[position] == '\0') {
        token.type = TOKEN_FIN;
        strcpy(token.valeur, "EOF");
        return token;
    }

    token.ligne = current_line;
    token.colonne = current_col;
    unsigned char c = (unsigned char)source[position];

    // Chaîne de caractères
    if (c == '"') {
        advance();
        int i = 0;
        while (source[position] != '\0' && source[position] != '"') {
            if (source[position] == '\\' && source[position + 1] != '\0') {
                advance();
                char esc = advance();
                if (esc == 'n') token.valeur[i++] = '\n';
                else if (esc == 't') token.valeur[i++] = '\t';
                else if (esc == '"') token.valeur[i++] = '"';
                else if (esc == '\\') token.valeur[i++] = '\\';
                else {
                    token.valeur[i++] = '\\';
                    token.valeur[i++] = esc;
                }
            } else {
                token.valeur[i++] = advance();
            }
            if (i >= (int)sizeof(token.valeur) - 1) break;
        }
        token.valeur[i] = '\0';

        if (source[position] == '"') {
            advance();
        }

        token.type = TOKEN_STRING;
        return token;
    }

    // Nombres (entiers ou réels)
    if (isdigit(c)) {
        int i = 0;
        int is_reel = 0;
        while (isdigit((unsigned char)source[position]) && i < (int)sizeof(token.valeur) - 1) {
            token.valeur[i++] = advance();
        }

        if (source[position] == '.' && isdigit((unsigned char)source[position + 1])) {
            is_reel = 1;
            token.valeur[i++] = advance(); // '.'
            while (isdigit((unsigned char)source[position]) && i < (int)sizeof(token.valeur) - 1) {
                token.valeur[i++] = advance();
            }
        }

        token.valeur[i] = '\0';
        token.type = is_reel ? TOKEN_NOMBRE_REEL : TOKEN_NOMBRE;
        return token;
    }

    // Mots-clés et identifiants
    if (is_ident_start(c)) {
        int i = 0;
        while (is_ident_part((unsigned char)source[position]) && i < (int)sizeof(token.valeur) - 1) {
            token.valeur[i++] = advance();
        }
        token.valeur[i] = '\0';

        // Correspondance des mots-clés simples
        if (strcasecmp(token.valeur, "imprimer") == 0 || strcasecmp(token.valeur, "afficher") == 0) {
            token.type = TOKEN_IMPRIMER;
            return token;
        }
        if (strcasecmp(token.valeur, "lire") == 0 || strcasecmp(token.valeur, "saisir") == 0) {
            token.type = TOKEN_LIRE;
            return token;
        }
        if (strcasecmp(token.valeur, "entier") == 0) {
            token.type = TOKEN_TYPE_ENTIER;
            return token;
        }
        if (strcasecmp(token.valeur, "reel") == 0 || strcmp(token.valeur, "réel") == 0 ||
            strcmp(token.valeur, "Réel") == 0 || strcasecmp(token.valeur, "flottant") == 0) {
            token.type = TOKEN_TYPE_REEL;
            return token;
        }
        if (strcasecmp(token.valeur, "chaine") == 0 || strcmp(token.valeur, "chaîne") == 0 ||
            strcmp(token.valeur, "Chaîne") == 0 || strcasecmp(token.valeur, "texte") == 0) {
            token.type = TOKEN_TYPE_CHAINE;
            return token;
        }
        if (strcasecmp(token.valeur, "booleen") == 0 || strcmp(token.valeur, "booléen") == 0 ||
            strcmp(token.valeur, "Booléen") == 0) {
            token.type = TOKEN_TYPE_BOOLEEN;
            return token;
        }
        if (strcasecmp(token.valeur, "vrai") == 0 || strcasecmp(token.valeur, "true") == 0) {
            token.type = TOKEN_VRAI;
            return token;
        }
        if (strcasecmp(token.valeur, "faux") == 0 || strcasecmp(token.valeur, "false") == 0) {
            token.type = TOKEN_FAUX;
            return token;
        }
        if (strcasecmp(token.valeur, "si") == 0) {
            token.type = TOKEN_SI;
            return token;
        }
        if (strcasecmp(token.valeur, "alors") == 0) {
            token.type = TOKEN_ALORS;
            return token;
        }
        if (strcasecmp(token.valeur, "sinon") == 0) {
            token.type = TOKEN_SINON;
            return token;
        }
        if (strcasecmp(token.valeur, "faire") == 0) {
            token.type = TOKEN_FAIRE;
            return token;
        }
        if (strcasecmp(token.valeur, "et") == 0) {
            token.type = TOKEN_ET;
            return token;
        }
        if (strcasecmp(token.valeur, "ou") == 0) {
            token.type = TOKEN_OU;
            return token;
        }
        if (strcasecmp(token.valeur, "non") == 0) {
            token.type = TOKEN_NON;
            return token;
        }

        // Variantes compactes
        if (strcasecmp(token.valeur, "finsi") == 0) {
            token.type = TOKEN_FIN_SI;
            return token;
        }
        if (strcasecmp(token.valeur, "tantque") == 0) {
            token.type = TOKEN_TANT_QUE;
            return token;
        }
        if (strcasecmp(token.valeur, "fintantque") == 0) {
            token.type = TOKEN_FIN_TANT_QUE;
            return token;
        }

        // Mot-clé composé "Tant que"
        if (strcasecmp(token.valeur, "tant") == 0) {
            int lines = 0, cols = 0;
            int next_pos = peek_keyword(position, "que", &lines, &cols);
            if (next_pos != -1) {
                position = next_pos;
                current_line += lines;
                if (lines > 0) current_col = cols;
                else current_col += cols;
                strcpy(token.valeur, "tant que");
                token.type = TOKEN_TANT_QUE;
                return token;
            }
        }

        // Mots-clés composés débutant par "fin" : "fin si" ou "fin tant que"
        if (strcasecmp(token.valeur, "fin") == 0) {
            int l1 = 0, c1 = 0;
            int pos_si = peek_keyword(position, "si", &l1, &c1);
            if (pos_si != -1) {
                position = pos_si;
                current_line += l1;
                if (l1 > 0) current_col = c1;
                else current_col += c1;
                strcpy(token.valeur, "fin si");
                token.type = TOKEN_FIN_SI;
                return token;
            }

            int l2 = 0, c2 = 0;
            int pos_tant = peek_keyword(position, "tant", &l2, &c2);
            if (pos_tant != -1) {
                int l3 = 0, c3 = 0;
                int pos_que = peek_keyword(pos_tant, "que", &l3, &c3);
                if (pos_que != -1) {
                    position = pos_que;
                    current_line += (l2 + l3);
                    if (l3 > 0) current_col = c3;
                    else if (l2 > 0) current_col = c2 + c3;
                    else current_col += (c2 + c3);
                    strcpy(token.valeur, "fin tant que");
                    token.type = TOKEN_FIN_TANT_QUE;
                    return token;
                }
            }

            int pos_tq = peek_keyword(position, "tantque", &l2, &c2);
            if (pos_tq != -1) {
                position = pos_tq;
                current_line += l2;
                if (l2 > 0) current_col = c2;
                else current_col += c2;
                strcpy(token.valeur, "fin tant que");
                token.type = TOKEN_FIN_TANT_QUE;
                return token;
            }
        }

        // Sinon c'est un identifiant ordinaire
        token.type = TOKEN_IDENT;
        return token;
    }

    // Opérateur d'affectation : <-
    if (c == '<' && source[position + 1] == '-') {
        advance();
        advance();
        strcpy(token.valeur, "<-");
        token.type = TOKEN_AFFECT;
        return token;
    }

    // Opérateurs de comparaison et chevrons
    if (c == '<') {
        advance();
        if (peek() == '=') {
            advance();
            strcpy(token.valeur, "<=");
            token.type = TOKEN_INF_EGAL;
        } else if (peek() == '>') {
            advance();
            strcpy(token.valeur, "<>");
            token.type = TOKEN_DIFF;
        } else {
            strcpy(token.valeur, "<");
            token.type = TOKEN_INF;
        }
        return token;
    }

    if (c == '>') {
        advance();
        if (peek() == '=') {
            advance();
            strcpy(token.valeur, ">=");
            token.type = TOKEN_SUP_EGAL;
        } else {
            strcpy(token.valeur, ">");
            token.type = TOKEN_SUP;
        }
        return token;
    }

    if (c == '=') {
        advance();
        if (peek() == '=') {
            advance();
            strcpy(token.valeur, "==");
        } else {
            strcpy(token.valeur, "=");
        }
        token.type = TOKEN_EGAL;
        return token;
    }

    if (c == '!') {
        advance();
        if (peek() == '=') {
            advance();
            strcpy(token.valeur, "!=");
            token.type = TOKEN_DIFF;
            return token;
        }
        strcpy(token.valeur, "!");
        token.type = TOKEN_NON;
        return token;
    }

    if (c == '&' && source[position + 1] == '&') {
        advance();
        advance();
        strcpy(token.valeur, "&&");
        token.type = TOKEN_ET;
        return token;
    }

    if (c == '|' && source[position + 1] == '|') {
        advance();
        advance();
        strcpy(token.valeur, "||");
        token.type = TOKEN_OU;
        return token;
    }

    // Opérateurs arithmétiques
    if (c == '+') {
        advance();
        strcpy(token.valeur, "+");
        token.type = TOKEN_PLUS;
        return token;
    }
    if (c == '-') {
        advance();
        strcpy(token.valeur, "-");
        token.type = TOKEN_MOINS;
        return token;
    }
    if (c == '*') {
        advance();
        strcpy(token.valeur, "*");
        token.type = TOKEN_ETOILE;
        return token;
    }
    if (c == '/') {
        advance();
        strcpy(token.valeur, "/");
        token.type = TOKEN_SLASH;
        return token;
    }
    if (c == '%') {
        advance();
        strcpy(token.valeur, "%");
        token.type = TOKEN_POURCENT;
        return token;
    }

    // Ponctuation
    if (c == '(') {
        advance();
        strcpy(token.valeur, "(");
        token.type = TOKEN_PAREN_OUVR;
        return token;
    }
    if (c == ')') {
        advance();
        strcpy(token.valeur, ")");
        token.type = TOKEN_PAREN_FERM;
        return token;
    }
    if (c == ',') {
        advance();
        strcpy(token.valeur, ",");
        token.type = TOKEN_VIRGULE;
        return token;
    }
    if (c == ';') {
        advance();
        strcpy(token.valeur, ";");
        token.type = TOKEN_POINT_VIRGULE;
        return token;
    }

    // Caractère inconnu
    token.valeur[0] = advance();
    token.valeur[1] = '\0';
    token.type = TOKEN_ERREUR;
    fprintf(stderr, "Erreur lexicale [ligne %d, col %d] : caractère inconnu '%s' (code %d)\n",
            token.ligne, token.colonne, token.valeur, (unsigned char)token.valeur[0]);
    return token;
}

const char *token_type_name(TokenType type) {
    switch (type) {
    case TOKEN_FIN: return "fin de fichier";
    case TOKEN_ERREUR: return "erreur";
    case TOKEN_TYPE_ENTIER: return "Entier";
    case TOKEN_TYPE_REEL: return "Reel";
    case TOKEN_TYPE_CHAINE: return "Chaine";
    case TOKEN_TYPE_BOOLEEN: return "Booleen";
    case TOKEN_IMPRIMER: return "Imprimer";
    case TOKEN_LIRE: return "Lire";
    case TOKEN_SI: return "Si";
    case TOKEN_ALORS: return "alors";
    case TOKEN_SINON: return "sinon";
    case TOKEN_FIN_SI: return "fin si";
    case TOKEN_TANT_QUE: return "Tant que";
    case TOKEN_FAIRE: return "faire";
    case TOKEN_FIN_TANT_QUE: return "fin tant que";
    case TOKEN_IDENT: return "identifiant";
    case TOKEN_NOMBRE: return "nombre";
    case TOKEN_NOMBRE_REEL: return "nombre reel";
    case TOKEN_STRING: return "chaine de caracteres";
    case TOKEN_VRAI: return "vrai";
    case TOKEN_FAUX: return "faux";
    case TOKEN_AFFECT: return "<-";
    case TOKEN_PLUS: return "+";
    case TOKEN_MOINS: return "-";
    case TOKEN_ETOILE: return "*";
    case TOKEN_SLASH: return "/";
    case TOKEN_POURCENT: return "%";
    case TOKEN_EGAL: return "=";
    case TOKEN_DIFF: return "!=";
    case TOKEN_INF: return "<";
    case TOKEN_SUP: return ">";
    case TOKEN_INF_EGAL: return "<=";
    case TOKEN_SUP_EGAL: return ">=";
    case TOKEN_ET: return "et";
    case TOKEN_OU: return "ou";
    case TOKEN_NON: return "non";
    case TOKEN_PAREN_OUVR: return "(";
    case TOKEN_PAREN_FERM: return ")";
    case TOKEN_VIRGULE: return ",";
    case TOKEN_POINT_VIRGULE: return ";";
    default: return "inconnu";
    }
}
