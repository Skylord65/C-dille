CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -pedantic -g
SRC = src/main.c src/lexer.c src/parser.c src/codegen.c
TARGET = Cdille

all: $(TARGET)

$(TARGET): $(SRC) src/lexer.h src/parser.h src/codegen.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET) programme output.c

test: $(TARGET)
	./$(TARGET) tests/afficher.cll -r

.PHONY: all clean test
