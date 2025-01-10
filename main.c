#include <stdio.h>
#include "helpers/vector.h"
#include "compiler.h"
int main()
{
    // example of using vector
    // struct vector* vec = vector_create(sizeof(int));
    // int x = 50;
    // vector_push(vec, &x);
    // x = 20;
    // vector_push(vec, &x);
    // vector_pop(vec);
    // vector_set_peek_pointer(vec, 0);
    // int* ptr = vector_peek(vec);
    // while (ptr)
    // {
    //     printf("Value: %d\n", *ptr);
    //     ptr = vector_peek(vec);
    // }

    // printf("Hello World\n");

    int res = compile_file("./test.c", "./test", 0);
    if (res == COMPILER_FILE_COMPILED_OK)
    {
        printf("Compiled successfully\n");
    }
    else if (res == COMPILER_FAILED_WITH_ERRORS)
    {
        printf("Failed to compile\n");
    }
    else
    {
        printf("Unknown error\n");
    }

    return 0;
}