#ifndef TOKEN_H
#define TOKEN_H

#define MAX_LEXEMA 256

typedef enum {
    TK_EOF,
    TK_ERRO,

    TK_ID,
    TK_NUM_INT,
    TK_NUM_REAL,

    TK_KW_PROGRAM,
    TK_KW_VAR,
    TK_KW_INTEGER,
    TK_KW_REAL,
    TK_KW_BEGIN,
    TK_KW_END,
    TK_KW_IF,
    TK_KW_THEN,
    TK_KW_ELSE,
    TK_KW_WHILE,
    TK_KW_DO,

    TK_OP_EQ,
    TK_OP_NE,
    TK_OP_LT,
    TK_OP_LE,
    TK_OP_GT,
    TK_OP_GE,
    TK_OP_AD,
    TK_OP_MIN,
    TK_OP_MUL,
    TK_OP_DIV,
    TK_OP_ASS,

    TK_SMB_SEM,
    TK_SMB_COM,
    TK_SMB_OPA,
    TK_SMB_CPA,
    TK_SMB_COL,
    TK_SMB_DOT
} TokenType;

typedef struct {
    TokenType type;
    char nome[32];
    char lexema[MAX_LEXEMA];
    int linha;
    int coluna;
} Token;

const char *token_type_name(TokenType type);

#endif
