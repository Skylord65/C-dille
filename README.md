# C-dille

Langage de programmation simple façon pseudo code théorique transpilé en C et en Français.

## Exécution

Le pseudo code doit être écrit dans un fichier avec l'extension `.cll` et doit ensuite être executé avec : 
```bash
./Cdille fichier.cll
```  

## Fonctionnement

```
  Lexeur
    |
  Parseur
    |
  Générateur de code en C
    |
  Complilateur
    |
  Executable
```

## Syntaxe

### Déclaration de variable

```
Entier x
```

### Affectation de variable

```
x <- 1
```

### Affichage dans le terminal

```
Imprimer "test à afficher"
```

### Condition

```
Si expression alors
    code
fin si
```

### Boucle

```
Tant que expression faire
    code
fin tant que
```

## Compilation et Installation

Pour compiler `Cdille` à l'aide du `Makefile` :

```bash
make
```

Pour exécuter les tests :

```bash
make test
```

## Options de la ligne de commande

```bash
./Cdille fichier.cll [options]
```

- `-o <nom>` : Spécifie le nom de l'exécutable généré (par défaut : `programme`).
- `-r`, `--run` : Compile puis exécute immédiatement le programme.
- `-h`, `--help` : Affiche l'aide.
