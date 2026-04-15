#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdio.h>

#define SYMBOL_TABLE_CAPACITY 1024
#define SYMBOL_CATEGORY_SIZE 32

typedef struct {
    char lexeme[256];
    char category[SYMBOL_CATEGORY_SIZE];
    int occupied;
} SymbolEntry;

typedef struct {
    SymbolEntry entries[SYMBOL_TABLE_CAPACITY];
    int count;
} SymbolTable;

void st_init(SymbolTable *st);
void st_seed_keywords(SymbolTable *st);
int st_contains(const SymbolTable *st, const char *lexeme);
void st_insert(SymbolTable *st, const char *lexeme, const char *category);
void st_write(const SymbolTable *st, FILE *out);

#endif
