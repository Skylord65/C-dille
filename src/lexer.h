#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_FIN = 0,
    TOKEN_ERREUR,

    // Types
    TOKEN_TYPE_ENTIER,    // Entier
    TOKEN_TYPE_REEL,      // Reel / Réel / Flottant
    TOKEN_TYPE_CHAINE,    // Chaine / Chaîne / Texte
    TOKEN_TYPE_BOOLEEN,   // Booleen / Booléen

    // Instructions
    TOKEN_IMPRIMER,       // Imprimer / Afficher
    TOKEN_LIRE,           // Lire / Saisir

    // Contrôle de flux
    TOKEN_SI,             // Si
    TOKEN_ALORS,          // Alors
    TOKEN_SINON,          // Sinon
    TOKEN_FIN_SI,         // Fin si / finsi
    TOKEN_TANT_QUE,       // Tant que / tantque
    TOKEN_FAIRE,          // Faire
    TOKEN_FIN_TANT_QUE,   // Fin tant que / fintantque

    // Identifiants & Littéraux
    TOKEN_IDENT,
    TOKEN_NOMBRE,         // 123
    TOKEN_NOMBRE_REEL,    // 123.45
    TOKEN_STRING,         // "texte"
    TOKEN_VRAI,           // vrai / true
    TOKEN_FAUX,           // faux / false

    // Opérateurs d'affectation et arithmétiques
    TOKEN_AFFECT,         // <-
    TOKEN_PLUS,           // +
    TOKEN_MOINS,          // -
    TOKEN_ETOILE,         // *
    TOKEN_SLASH,          // /
    TOKEN_POURCENT,       // %

    // Comparaisons
    TOKEN_EGAL,           // = ou ==
    TOKEN_DIFF,           // != ou <>
    TOKEN_INF,            // <
    TOKEN_SUP,            // >
    TOKEN_INF_EGAL,       // <=
    TOKEN_SUP_EGAL,       // >=

    // Opérateurs logiques
    TOKEN_ET,             // et / &&
    TOKEN_OU,             // ou / ||
    TOKEN_NON,            // non / !

    // Ponctuation
    TOKEN_PAREN_OUVR,     // (
    TOKEN_PAREN_FERM,     // )
    TOKEN_VIRGULE,        // ,
    TOKEN_POINT_VIRGULE   // ;
} TokenType;

typedef struct {
    TokenType type;
    char valeur[256];
    int ligne;
    int colonne;
} Token;

void lexer_init(const char *src);
Token lexer_next(void);
const char *token_type_name(TokenType type);

#endif
