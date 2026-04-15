#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    ST_START,
    ST_ID,
    ST_INT,
    ST_REAL_DOT,
    ST_REAL,
    ST_EXP_MARK,
    ST_EXP_SIGN,
    ST_EXP_BODY,
    ST_COMMENT
} AutomatonState;

static void to_lowercase(char *text) {
    for (int i = 0; text[i] != '\0'; i++) {
        text[i] = (char)tolower((unsigned char)text[i]);
    }
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TK_EOF: return "EOF";
        case TK_ERRO: return "ERRO";
        case TK_ID: return "ID";
        case TK_NUM_INT: return "NUM_INT";
        case TK_NUM_REAL: return "NUM_REAL";
        case TK_KW_PROGRAM: return "KW_PROGRAM";
        case TK_KW_VAR: return "KW_VAR";
        case TK_KW_INTEGER: return "KW_INTEGER";
        case TK_KW_REAL: return "KW_REAL";
        case TK_KW_BEGIN: return "KW_BEGIN";
        case TK_KW_END: return "KW_END";
        case TK_KW_IF: return "KW_IF";
        case TK_KW_THEN: return "KW_THEN";
        case TK_KW_ELSE: return "KW_ELSE";
        case TK_KW_WHILE: return "KW_WHILE";
        case TK_KW_DO: return "KW_DO";
        case TK_OP_EQ: return "OP_EQ";
        case TK_OP_NE: return "OP_NE";
        case TK_OP_LT: return "OP_LT";
        case TK_OP_LE: return "OP_LE";
        case TK_OP_GT: return "OP_GT";
        case TK_OP_GE: return "OP_GE";
        case TK_OP_AD: return "OP_AD";
        case TK_OP_MIN: return "OP_MIN";
        case TK_OP_MUL: return "OP_MUL";
        case TK_OP_DIV: return "OP_DIV";
        case TK_OP_ASS: return "OP_ASS";
        case TK_SMB_SEM: return "SMB_SEM";
        case TK_SMB_COM: return "SMB_COM";
        case TK_SMB_OPA: return "SMB_OPA";
        case TK_SMB_CPA: return "SMB_CPA";
        case TK_SMB_COL: return "SMB_COL";
        case TK_SMB_DOT: return "SMB_DOT";
        default: return "UNKNOWN";
    }
}

static Token make_token(TokenType type, const char *lexeme, int line, int column) {
    Token tk;
    tk.type = type;
    strncpy(tk.nome, token_type_name(type), sizeof(tk.nome) - 1);
    tk.nome[sizeof(tk.nome) - 1] = '\0';
    strncpy(tk.lexema, lexeme, sizeof(tk.lexema) - 1);
    tk.lexema[sizeof(tk.lexema) - 1] = '\0';
    tk.linha = line;
    tk.coluna = column;
    return tk;
}

static TokenType keyword_or_identifier(const char *lexeme) {
    if (strcmp(lexeme, "program") == 0) return TK_KW_PROGRAM;
    if (strcmp(lexeme, "var") == 0) return TK_KW_VAR;
    if (strcmp(lexeme, "integer") == 0) return TK_KW_INTEGER;
    if (strcmp(lexeme, "real") == 0) return TK_KW_REAL;
    if (strcmp(lexeme, "begin") == 0) return TK_KW_BEGIN;
    if (strcmp(lexeme, "end") == 0) return TK_KW_END;
    if (strcmp(lexeme, "if") == 0) return TK_KW_IF;
    if (strcmp(lexeme, "then") == 0) return TK_KW_THEN;
    if (strcmp(lexeme, "else") == 0) return TK_KW_ELSE;
    if (strcmp(lexeme, "while") == 0) return TK_KW_WHILE;
    if (strcmp(lexeme, "do") == 0) return TK_KW_DO;
    return TK_ID;
}

static int read_char(Lexer *lx) {
    int c;

    if (lx->has_pushed_char) {
        lx->has_pushed_char = 0;
        lx->line = lx->pushed_line;
        lx->column = lx->pushed_column;
        return lx->pushed_char;
    }

    c = fgetc(lx->source);
    if (c == EOF) return EOF;

    if (c == '\n') {
        lx->line++;
        lx->column = 0;
    } else {
        lx->column++;
    }

    return c;
}

static void unread_char(Lexer *lx, int c, int previous_line, int previous_column) {
    if (c == EOF) return;
    lx->has_pushed_char = 1;
    lx->pushed_char = c;
    lx->pushed_line = lx->line;
    lx->pushed_column = lx->column;
    lx->line = previous_line;
    lx->column = previous_column;
}

static void emit_lex_line(Lexer *lx, const Token *tk) {
    if (tk->type == TK_ERRO) return;
    fprintf(lx->lex_out, "<%s, %s> %d %d\n", tk->nome, tk->lexema, tk->linha, tk->coluna);
}

static void emit_error(Lexer *lx, const char *message, const char *lexeme, int line, int column) {
    if (lexeme && lexeme[0] != '\0') {
        fprintf(lx->err_out, "%s '%s' na linha %d, coluna %d\n", message, lexeme, line, column);
    } else {
        fprintf(lx->err_out, "%s na linha %d, coluna %d\n", message, line, column);
    }
    lx->error_count++;
}

static int is_identifier_tail(int c) {
    return isalpha((unsigned char)c) || isdigit((unsigned char)c);
}

static void skip_blank_and_comments(Lexer *lx) {
    while (1) {
        int before_line = lx->line;
        int before_col = lx->column;
        int c = read_char(lx);

        if (c == EOF) return;

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        }

        if (c == '{') {
            AutomatonState state = ST_COMMENT;
            int comment_line = lx->line;
            int comment_col = lx->column;

            while (state == ST_COMMENT) {
                c = read_char(lx);
                if (c == EOF) {
                    emit_error(lx, "Erro léxico: comentário não fechado", NULL, comment_line, comment_col);
                    return;
                }
                if (c == '}') {
                    state = ST_START;
                }
            }
            continue;
        }

        unread_char(lx, c, before_line, before_col);
        return;
    }
}

static Token scan_identifier(Lexer *lx, int first_char, int start_line, int start_col) {
    char lexeme[MAX_LEXEMA];
    int size = 0;
    lexeme[size++] = (char)first_char;

    while (1) {
        int before_line = lx->line;
        int before_col = lx->column;
        int c = read_char(lx);

        if (c != EOF && is_identifier_tail(c)) {
            if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)c;
            continue;
        }

        unread_char(lx, c, before_line, before_col);
        break;
    }

    lexeme[size] = '\0';
    to_lowercase(lexeme);

    TokenType type = keyword_or_identifier(lexeme);
    if (type == TK_ID) {
        st_insert(lx->symbols, lexeme, "identificador");
    }

    return make_token(type, lexeme, start_line, start_col);
}

static Token scan_number(Lexer *lx, int first_char, int start_line, int start_col) {
    char lexeme[MAX_LEXEMA];
    int size = 0;
    AutomatonState state = ST_INT;
    lexeme[size++] = (char)first_char;

    while (1) {
        int before_line = lx->line;
        int before_col = lx->column;
        int c = read_char(lx);

        if (state == ST_INT) {
            if (c != EOF && isdigit((unsigned char)c)) {
                if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)c;
                continue;
            }
            if (c == '.') {
                int dot_prev_line = before_line;
                int dot_prev_col = before_col;
                int next_prev_line = lx->line;
                int next_prev_col = lx->column;
                int next = read_char(lx);

                if (next != EOF && isdigit((unsigned char)next)) {
                    state = ST_REAL;
                    if (size < MAX_LEXEMA - 1) lexeme[size++] = '.';
                    if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)next;
                    continue;
                }

                unread_char(lx, next, next_prev_line, next_prev_col);
                unread_char(lx, '.', dot_prev_line, dot_prev_col);
                lexeme[size] = '\0';
                return make_token(TK_NUM_INT, lexeme, start_line, start_col);
            }

            unread_char(lx, c, before_line, before_col);
            lexeme[size] = '\0';
            return make_token(TK_NUM_INT, lexeme, start_line, start_col);
        }

        if (state == ST_REAL) {
            if (c != EOF && isdigit((unsigned char)c)) {
                if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)c;
                continue;
            }
            if (c == 'E' || c == 'e') {
                int e_prev_line = before_line;
                int e_prev_col = before_col;
                int next_prev_line = lx->line;
                int next_prev_col = lx->column;
                int next = read_char(lx);

                if (next == '+' || next == '-') {
                    int digit_prev_line = lx->line;
                    int digit_prev_col = lx->column;
                    int digit = read_char(lx);
                    if (digit != EOF && isdigit((unsigned char)digit)) {
                        if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)c;
                        if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)next;
                        if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)digit;
                        state = ST_EXP_BODY;
                        continue;
                    }
                    unread_char(lx, digit, digit_prev_line, digit_prev_col);
                    unread_char(lx, next, next_prev_line, next_prev_col);
                    unread_char(lx, c, e_prev_line, e_prev_col);
                    lexeme[size] = '\0';
                    return make_token(TK_NUM_REAL, lexeme, start_line, start_col);
                }

                if (next != EOF && isdigit((unsigned char)next)) {
                    if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)c;
                    if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)next;
                    state = ST_EXP_BODY;
                    continue;
                }

                unread_char(lx, next, next_prev_line, next_prev_col);
                unread_char(lx, c, e_prev_line, e_prev_col);
                lexeme[size] = '\0';
                return make_token(TK_NUM_REAL, lexeme, start_line, start_col);
            }

            unread_char(lx, c, before_line, before_col);
            lexeme[size] = '\0';
            return make_token(TK_NUM_REAL, lexeme, start_line, start_col);
        }

        if (state == ST_EXP_BODY) {
            if (c != EOF && isdigit((unsigned char)c)) {
                if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)c;
                continue;
            }
            unread_char(lx, c, before_line, before_col);
            lexeme[size] = '\0';
            return make_token(TK_NUM_REAL, lexeme, start_line, start_col);
        }
    }
}

static Token scan_invalid_string(Lexer *lx, int start_line, int start_col) {
    char lexeme[MAX_LEXEMA];
    int size = 0;
    lexeme[size++] = '\'';

    while (1) {
        int c = read_char(lx);

        if (c == EOF) {
            lexeme[size] = '\0';
            emit_error(lx, "Erro léxico: string não fechada antes do fim de arquivo", lexeme, start_line, start_col);
            return make_token(TK_ERRO, lexeme, start_line, start_col);
        }

        if (c == '\n') {
            lexeme[size] = '\0';
            emit_error(lx, "Erro léxico: string não fechada antes de quebra de linha", lexeme, start_line, start_col);
            return make_token(TK_ERRO, lexeme, start_line, start_col);
        }

        if (size < MAX_LEXEMA - 1) lexeme[size++] = (char)c;

        if (c == '\'') {
            lexeme[size] = '\0';
            emit_error(lx, "Erro léxico: string não suportada pela especificação", lexeme, start_line, start_col);
            return make_token(TK_ERRO, lexeme, start_line, start_col);
        }
    }
}

int lexer_open(Lexer *lx,
               SymbolTable *symbols,
               const char *input_path,
               const char *lex_path,
               const char *err_path) {
    lx->source = fopen(input_path, "r");
    if (!lx->source) return 0;

    lx->lex_out = fopen(lex_path, "w");
    lx->err_out = fopen(err_path, "w");
    if (!lx->lex_out || !lx->err_out) {
        lexer_close(lx);
        return 0;
    }

    lx->line = 1;
    lx->column = 0;
    lx->has_pushed_char = 0;
    lx->pushed_char = EOF;
    lx->pushed_line = 1;
    lx->pushed_column = 0;
    lx->error_count = 0;
    lx->symbols = symbols;
    return 1;
}

void lexer_close(Lexer *lx) {
    if (lx->source) fclose(lx->source);
    if (lx->lex_out) fclose(lx->lex_out);
    if (lx->err_out) {
        if (lx->error_count == 0) {
            fprintf(lx->err_out, "Nenhum erro léxico encontrado.\n");
        }
        fclose(lx->err_out);
    }

    lx->source = NULL;
    lx->lex_out = NULL;
    lx->err_out = NULL;
}

Token lexer_next_token(Lexer *lx) {
    skip_blank_and_comments(lx);

    int prev_line = lx->line;
    int prev_col = lx->column;
    int c = read_char(lx);

    if (c == EOF) {
        Token tk = make_token(TK_EOF, "EOF", prev_line, prev_col + 1);
        emit_lex_line(lx, &tk);
        return tk;
    }

    int start_line = lx->line;
    int start_col = lx->column;

    if (isalpha((unsigned char)c)) {
        Token tk = scan_identifier(lx, c, start_line, start_col);
        emit_lex_line(lx, &tk);
        return tk;
    }

    if (isdigit((unsigned char)c)) {
        Token tk = scan_number(lx, c, start_line, start_col);
        emit_lex_line(lx, &tk);
        return tk;
    }

    if (c == '\'') {
        return scan_invalid_string(lx, start_line, start_col);
    }

    switch (c) {
        case '+': {
            Token tk = make_token(TK_OP_AD, "+", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case '-': {
            Token tk = make_token(TK_OP_MIN, "-", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case '*': {
            Token tk = make_token(TK_OP_MUL, "*", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case '/': {
            Token tk = make_token(TK_OP_DIV, "/", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case '=': {
            Token tk = make_token(TK_OP_EQ, "=", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case ';': {
            Token tk = make_token(TK_SMB_SEM, ";", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case ',': {
            Token tk = make_token(TK_SMB_COM, ",", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case '(': {
            Token tk = make_token(TK_SMB_OPA, "(", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case ')': {
            Token tk = make_token(TK_SMB_CPA, ")", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case '.': {
            Token tk = make_token(TK_SMB_DOT, ".", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case ':': {
            int before_line = lx->line;
            int before_col = lx->column;
            int d = read_char(lx);
            if (d == '=') {
                Token tk = make_token(TK_OP_ASS, ":=", start_line, start_col);
                emit_lex_line(lx, &tk);
                return tk;
            }
            unread_char(lx, d, before_line, before_col);
            Token tk = make_token(TK_SMB_COL, ":", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case '<': {
            int before_line = lx->line;
            int before_col = lx->column;
            int d = read_char(lx);
            if (d == '=') {
                Token tk = make_token(TK_OP_LE, "<=", start_line, start_col);
                emit_lex_line(lx, &tk);
                return tk;
            }
            if (d == '>') {
                Token tk = make_token(TK_OP_NE, "<>", start_line, start_col);
                emit_lex_line(lx, &tk);
                return tk;
            }
            unread_char(lx, d, before_line, before_col);
            Token tk = make_token(TK_OP_LT, "<", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        case '>': {
            int before_line = lx->line;
            int before_col = lx->column;
            int d = read_char(lx);
            if (d == '=') {
                Token tk = make_token(TK_OP_GE, ">=", start_line, start_col);
                emit_lex_line(lx, &tk);
                return tk;
            }
            unread_char(lx, d, before_line, before_col);
            Token tk = make_token(TK_OP_GT, ">", start_line, start_col);
            emit_lex_line(lx, &tk);
            return tk;
        }
        default: {
            char bad[2] = {(char)c, '\0'};
            emit_error(lx, "Erro léxico: caractere inválido", bad, start_line, start_col);
            return make_token(TK_ERRO, bad, start_line, start_col);
        }
    }
}
