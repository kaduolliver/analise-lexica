#include <stdio.h>
#include "lexer.h"
#include "symbol_table.h"

int main(int argc, char *argv[]) {
    const char *input_path = "fonte.pas";
    const char *lex_path = "saida.lex";
    const char *ts_path = "saida.ts";
    const char *err_path = "saida.err";

    if (argc > 1) input_path = argv[1];
    if (argc > 2) lex_path = argv[2];
    if (argc > 3) ts_path = argv[3];
    if (argc > 4) err_path = argv[4];

    SymbolTable symbols;
    st_init(&symbols);
    st_seed_keywords(&symbols);

    Lexer lexer = {0};
    if (!lexer_open(&lexer, &symbols, input_path, lex_path, err_path)) {
        fprintf(stderr, "Falha ao abrir arquivos de entrada ou saída.\n");
        return 1;
    }

    Token tk;
    do {
        tk = lexer_next_token(&lexer);
    } while (tk.type != TK_EOF);

    FILE *ts_out = fopen(ts_path, "w");
    if (!ts_out) {
        fprintf(stderr, "Falha ao criar arquivo de tabela de símbolos.\n");
        lexer_close(&lexer);
        return 1;
    }

    st_write(&symbols, ts_out);
    fclose(ts_out);
    lexer_close(&lexer);

    printf("Análise concluída.\n");
    printf("Entrada: %s\n", input_path);
    printf("Arquivos gerados: %s, %s, %s\n", lex_path, ts_path, err_path);
    return 0;
}
