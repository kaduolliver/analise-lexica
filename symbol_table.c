#include "symbol_table.h"

#include <string.h>

void st_init(SymbolTable *st) {
    st->count = 0;
    for (int i = 0; i < SYMBOL_TABLE_CAPACITY; i++) {
        st->entries[i].occupied = 0;
        st->entries[i].lexeme[0] = '\0';
        st->entries[i].category[0] = '\0';
    }
}

int st_contains(const SymbolTable *st, const char *lexeme) {
    for (int i = 0; i < SYMBOL_TABLE_CAPACITY; i++) {
        if (st->entries[i].occupied && strcmp(st->entries[i].lexeme, lexeme) == 0) {
            return 1;
        }
    }
    return 0;
}

void st_insert(SymbolTable *st, const char *lexeme, const char *category) {
    if (st_contains(st, lexeme)) return;

    for (int i = 0; i < SYMBOL_TABLE_CAPACITY; i++) {
        if (!st->entries[i].occupied) {
            st->entries[i].occupied = 1;
            strncpy(st->entries[i].lexeme, lexeme, sizeof(st->entries[i].lexeme) - 1);
            st->entries[i].lexeme[sizeof(st->entries[i].lexeme) - 1] = '\0';
            strncpy(st->entries[i].category, category, sizeof(st->entries[i].category) - 1);
            st->entries[i].category[sizeof(st->entries[i].category) - 1] = '\0';
            st->count++;
            return;
        }
    }
}

void st_seed_keywords(SymbolTable *st) {
    const char *keywords[] = {
        "program", "var", "integer", "real", "begin", "end",
        "if", "then", "else", "while", "do"
    };
    int total = (int)(sizeof(keywords) / sizeof(keywords[0]));
    for (int i = 0; i < total; i++) {
        st_insert(st, keywords[i], "palavra-reservada");
    }
}

void st_write(const SymbolTable *st, FILE *out) {
    fprintf(out, "Lexema\tTipo de token\n");
    for (int i = 0; i < SYMBOL_TABLE_CAPACITY; i++) {
        if (st->entries[i].occupied) {
            fprintf(out, "%s\t%s\n", st->entries[i].lexeme, st->entries[i].category);
        }
    }
}
