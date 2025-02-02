#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"
#include <string.h>
#include <assert.h>

#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }
struct token *read_next_token();
static struct lex_process *lex_process;
static struct token tmp_token;

static char peekc()
{
    return lex_process->function->peek_char(lex_process);
}

static void pushc(char c)
{
    lex_process->function->push_char(lex_process, c);
}

static struct pos lex_file_position()
{
    return lex_process->pos;
}

static char nextc()
{
    char c = lex_process->function->next_char(lex_process);
    lex_process->pos.col++;
    if (c == '\n')
    {
        lex_process->pos.line++;
        lex_process->pos.col = 1;
    }
    return c;
}

struct token *token_create(struct token *_token)
{
    memcpy(&tmp_token, _token, sizeof(struct token));
    tmp_token.pos = lex_file_position();
    return &tmp_token;
}

static struct token *lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}

static struct token *handle_whitespace()
{
    struct token *last_token = lexer_last_token();
    if (last_token)
    {
        last_token->whitespace = true;
    }
    nextc();
    return read_next_token();
}

const char *read_number_str()
{
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));
    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long read_number()
{
    const char *s = read_number_str();
    return atoll(s);
}

struct token *token_make_number_for_value(unsigned long number)
{
    return token_create(&(struct token){
        .type = TOKEN_TYPE_NUMBER, .llnum = number});
}

struct token *token_make_number(struct compile_process *process, char c)
{
    return token_make_number_for_value(read_number());
}

static bool op_treated_as_one(char op)
{
    return op == '(' || op == '[' || op == ',' || op == '.' || op == '*' || op == '?';
}

static bool is_single_operator(char op)
{
    return op == '+' || op == '-' || op == '/' || op == '*' || op == '%' || op == '^' || op == '&' || op == '|' || op == '~' || op == '!' || op == '<' || op == '>' || op == '=' || op == '(' || op == '[' || op == ',' || op == '.' || op == '?';
}

const char *read_op()
{
    bool single_operator = true;
    char op = nextc();
    struct buffer *buffer = buffer_create();
    buffer_write(buffer, op);
    if (!op_treated_as_one(op))
    {
        op = peekc();
        if (is_single_operator(op))
        {
            buffer_write(buffer, op);
            nextc();
            single_operator = false;
        }
    }
    buffer_write(buffer, 0x00);
    char *ptr = buffer_ptr(buffer);
    if (!single_operator)
    {
    }
}

static struct token *token_make_operator_or_string()
{
    return NULL;
}

static struct token *token_make_string(char start_delim, char end_delim)
{
    struct buffer *buf = buffer_create();
    assert(nextc() == start_delim);
    char c = nextc();
    for (; c != end_delim && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            // we need to handle an escape character
            continue;
        }
        buffer_write(buf, c);
    }
    buffer_write(buf, 0x00);
    return token_create(&(struct token){
        .type = TOKEN_TYPE_STRING, .sval = buffer_ptr(buf)});
}

struct token *read_next_token()
{
    struct token *token = NULL;
    char c = peekc();
    switch (c)
    {
    NUMERIC_CASES:
        token = token_make_number(lex_process->compiler, c);
        break;
    OPERATOR_CASE_EXCLUDING_DIVISION:
        token = token_make_operator_or_string();
        break;
    case '"':
        token = token_make_string('"', '"');
        break;
    case ' ':
    case '\t':
        token = handle_whitespace();
        break;
    case EOF:
        break;

    default:
        compile_error(lex_process->compiler, "Unexpected token\n");
        break;
    }
    return token;
}

int lex(struct lex_process *process)
{
    process->currtent_expression_count = 0;
    process->parenthsess_buffer = NULL;
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;

    struct token *token = read_next_token();
    while (token)
    {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }
    return LEXICAL_ANALYSIS_ALL_OK;
}