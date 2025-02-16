#include "compiler.h"
bool tocken_if_keyword(struct token *token, const char *value)
{
    return token->type == TOKEN_TYPE_KEYWORD && S_EQ(token->sval, value);
}