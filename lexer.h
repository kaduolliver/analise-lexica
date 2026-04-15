#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "token.h"
#include "symbol_table.h"

typedef struct {
    FILE *source;
    FILE *lex_out;
    FILE *err_out;
    int line;
    int column;
    int pushed_char;
    int has_pushed_char;
    int pushed_line;
    int pushed_column;
    int error_count;
    SymbolTable *symbols;
} Lexer;

int lexer_open(Lexer *lx,
               SymbolTable *symbols,
               const char *input_path,
               const char *lex_path,
               const char *err_path);

void lexer_close(Lexer *lx);
Token lexer_next_token(Lexer *lx);

#endif
