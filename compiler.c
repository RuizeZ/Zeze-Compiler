#include "compiler.h"

struct lex_precess_functions compiler_lex_functions = {
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char,
};

int compile_file(const char *filename, const char *out_filename, int flags)
{
    struct compile_process *process = compile_process_create(filename, out_filename, flags);
    if (!process)
    {
        return COMPILER_FAILED_WITH_ERRORS;
    }

    // perform lexical analysis
    struct lex_process *lex_process = lex_process_create(process, &compiler_lex_functions, NULL);
    // perform parsing

    // preform code generation

    return 0;
}