#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>

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

bool op_valid(const char *op)
{
    return S_EQ(op, "+") || S_EQ(op, "-") || S_EQ(op, "*") || S_EQ(op, "/") || S_EQ(op, "!") || S_EQ(op, "^") || S_EQ(op, "+=") || S_EQ(op, "-=") || S_EQ(op, "*=") || S_EQ(op, "/=") || S_EQ(op, ">>") || S_EQ(op, "<<") || S_EQ(op, ">=") || S_EQ(op, "<=") || S_EQ(op, ">") || S_EQ(op, "<") || S_EQ(op, "||") || S_EQ(op, "&&") || S_EQ(op, "|") || S_EQ(op, "&") || S_EQ(op, "++") || S_EQ(op, "--") || S_EQ(op, "=") || S_EQ(op, "!=") || S_EQ(op, "==") || S_EQ(op, "->") || S_EQ(op, "(") || S_EQ(op, "[") || S_EQ(op, ",") || S_EQ(op, ".") || S_EQ(op, "...") || S_EQ(op, "?") || S_EQ(op, "%");
}

void read_op_flush_back_keep_first(struct buffer *buffer)
{
    // +*, push * back to the stack, only leave + in the buffer
    const char *data = buffer_ptr(buffer);
    int len = buffer->len;
    for (int i = len - 1; i > 0; i--)
    {
        if (data[i] == 0x00)
        {
            continue;
        }
        pushc(data[i]);
    }
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
        if (!op_valid(ptr))
        {
            read_op_flush_back_keep_first(buffer);
            ptr[1] = 0x00;
        }
    }
    else if (!op_valid(ptr))
    {
        compiler_error(lex_process->compiler, "Invalid operator: %s\n", ptr);
    }
    return ptr;
}

bool lex_is_in_expression()
{
    return lex_process->currtent_expression_count > 0;
}

static void lex_new_expression()
{
    lex_process->currtent_expression_count++;
    // ( () )
    if (lex_process->currtent_expression_count == 1)
    {
        lex_process->parenthsess_buffer = buffer_create();
    }
}
// close the expression
static void lex_finish_expression()
{
    // ) -> symble
    lex_process->currtent_expression_count--;
    // there is no ( to match )
    if (lex_process->currtent_expression_count < 0)
    {
        compiler_error(lex_process->compiler, "Unexpected ')'\n");
    }
}

bool is_keyword(const char *str)
{
    return S_EQ(str, "unsigned") ||
           S_EQ(str, "signed") ||
           S_EQ(str, "char") ||
           S_EQ(str, "short") ||
           S_EQ(str, "int") ||
           S_EQ(str, "long") ||
           S_EQ(str, "float") ||
           S_EQ(str, "double") ||
           S_EQ(str, "void") ||
           S_EQ(str, "struct") ||
           S_EQ(str, "enum") ||
           S_EQ(str, "union") ||
           S_EQ(str, "const") ||
           S_EQ(str, "static") ||
           S_EQ(str, "extern") ||
           S_EQ(str, "auto") ||
           S_EQ(str, "register") ||
           S_EQ(str, "volatile") ||
           S_EQ(str, "inline") ||
           S_EQ(str, "restrict") ||
           S_EQ(str, "sizeof") ||
           S_EQ(str, "typedef") ||
           S_EQ(str, "return") ||
           S_EQ(str, "if") ||
           S_EQ(str, "else") ||
           S_EQ(str, "switch") ||
           S_EQ(str, "case") ||
           S_EQ(str, "default") ||
           S_EQ(str, "while") ||
           S_EQ(str, "do") ||
           S_EQ(str, "for") ||
           S_EQ(str, "break") ||
           S_EQ(str, "continue") ||
           S_EQ(str, "goto") ||
           S_EQ(str, "void");
}

static struct token *token_make_operator_or_string()
{
    char op = peekc();
    // #include <abc.h> -> # include abc.h
    if (op == '<')
    {
        struct token *last_token = lexer_last_token();
        if (tocken_if_keyword(last_token, "include"))
        {
            return token_make_string('<', '>');
        }
    }
    struct token *token = token_create(&(struct token){
        .type = TOKEN_TYPE_OPERATOR, .sval = read_op()});
    if (op == '(')
    {
        lex_new_expression();
    }
    return token;
}

struct token *token_make_one_line_comment()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, c, c != '\n' && c != EOF);
    // hello world
    return token_create(&(struct token){
        .type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *token_make_multiline_comment()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    while (1)
    {
        LEX_GETC_IF(buffer, c, c != '*' && c != EOF);
        if (c == EOF)
        {
            compiler_error(lex_process->compiler, "Unexpected EOF\n");
        }
        else if (c == '*')
        {
            // skip the *
            nextc();
            if (peekc() == '/')
            {
                /**
                 *  end
                 */
                nextc();
                break;
            }
        }
    }

    // hello world
    return token_create(&(struct token){
        .type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *handle_comment()
{
    char c = peekc();
    if (c == '/')
    {
        nextc();
        if (peekc() == '/')
        {
            //
            nextc();
            return token_make_one_line_comment();
        }
        else if (peekc() == '*')
        {
            nextc();
            return token_make_multiline_comment();
        }
        pushc('/');
        return token_make_operator_or_string();
    }

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

static struct token *token_make_symbol()
{
    char c = nextc();
    if (c == ')')
    {
        lex_finish_expression();
    }

    struct token *token = token_create(&(struct token){
        .type = TOKEN_TYPE_SYMBOL, .cval = c});
    return token;
}

static struct token *token_make_identifier_or_keyword()
{
    struct buffer *buf = buffer_create();
    char c = 0;
    LEX_GETC_IF(buf, c, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');

    // null terminator
    buffer_write(buf, 0x00);

    // check if this is a keyword
    if (is_keyword(buffer_ptr(buf)))
    {
        return token_create(&(struct token){
            .type = TOKEN_TYPE_KEYWORD, .sval = buffer_ptr(buf)});
    }

    return token_create(&(struct token){
        .type = TOKEN_TYPE_IDENTIFIER, .sval = buffer_ptr(buf)});
}

struct token *read_special_token()
{
    char c = peekc();
    if (isalpha(c) || c == '_')
    {
        return token_make_identifier_or_keyword();
    }
    return NULL;
}

struct token *token_make_newline()
{
    nextc();
    return token_create(&(struct token){
        .type = TOKEN_TYPE_NEWLINE});
}

struct token *read_next_token()
{
    struct token *token = NULL;
    char c = peekc();
    token = handle_comment();
    if (token)
    {
        return token;
    }

    switch (c)
    {
    NUMERIC_CASES:
        token = token_make_number(lex_process->compiler, c);
        break;
    OPERATOR_CASE_EXCLUDING_DIVISION:
        token = token_make_operator_or_string();
        break;
    SYMBOL_CASE:
        token = token_make_symbol();
        break;
    case '"':
        token = token_make_string('"', '"');
        break;
    case ' ':
    case '\t':
        token = handle_whitespace();
        break;
    case '\n':
        token = token_make_newline();
        break;
    case EOF:
        break;

    default:
        token = read_special_token();
        if (!token)
        {
            compiler_error(lex_process->compiler, "Unexpected token\n");
        }

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