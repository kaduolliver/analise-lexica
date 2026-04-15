# Relatório Técnico - Analisador Léxico para MicroPascal

A Tabela de Símbolos usa um arranjo sequencial com busca linear e sem hashing, e o reconhecimento léxico foi estruturado com rotinas específicas de varredura.

- `token.h`
- `symbol_table.h` / `symbol_table.c`
- `lexer.h` / `lexer.c`
- `main.c`

## Compilação

```bash
gcc main.c lexer.c symbol_table.c -o lexer -std=c11 -Wall -Wextra
```

## Execução

```bash
./lexer
```

ou

```bash
./lexer entrada.pas saida.lex saida.ts saida.err
```


## 1. Visão geral
O analisador foi implementado em C e organizado em módulos independentes. O programa lê um arquivo-fonte em MicroPascal, reconhece tokens, registra posição, mantém uma Tabela de Símbolos e produz arquivos `.lex`, `.ts` e `.err`.

## 2. Estruturas de dados

### 2.1 `Token`
Armazena nome do token, lexema, linha e coluna.

### 2.2 `SymbolEntry` e `SymbolTable`
A Tabela de Símbolos foi implementada como um vetor de registros fixos, com campo `occupied`. Essa decisão privilegia simplicidade e deixa a estrutura diferente de uma tabela hash.

### 2.3 `Lexer`
Agrupa o estado atual do escaneamento:
- ponteiros de arquivo
- linha e coluna
- um caractere de pushback
- contador de erros
- ponteiro para a TS

## 3. Funções principais

- `st_init`, `st_seed_keywords`, `st_insert`, `st_write`
- `lexer_open`, `lexer_close`, `lexer_next_token`
- `scan_identifier`, `scan_number`, `scan_invalid_string`
- `skip_blank_and_comments`

## 4. Explicação do AFD

O AFD foi dividido por famílias de lexemas:
- identificadores
- inteiros
- reais
- expoente
- comentários
- operadores compostos

### 4.1 Identificador
Estado inicial -> letra -> estado de identificador. Permanece ao ler letra ou dígito.

### 4.2 Inteiro
Estado inicial -> dígito -> estado inteiro. Permanece em dígitos.

### 4.3 Real
Do estado inteiro, ao ler `.` seguido de dígito, entra em estado real.

### 4.4 Expoente
Do estado real, ao ler `E` ou `e`, pode seguir para:
- dígito
- sinal `+` ou `-` seguido de dígito

### 4.5 Comentário
Ao ler `{`, entra em estado de comentário até encontrar `}`. Se EOF ocorrer antes, gera erro.

## 5. Testes

### Corretos
1. `tests/corretos/teste1.pas`
2. `tests/corretos/teste2.pas`
3. `tests/corretos/teste3.pas`

### Com erro
1. `tests/erros/teste4_caractere_invalido.pas`
2. `tests/erros/teste5_comentario_nao_fechado.pas`
3. `tests/erros/teste6_string_nao_fechada.pas`

## 6. Saídas
O arquivo `.lex` contém linhas no formato:
`<nome_token, lexema> linha coluna`

O arquivo `.ts` contém palavras reservadas e identificadores sem repetição.

O arquivo `.err` contém os erros léxicos encontrados.
