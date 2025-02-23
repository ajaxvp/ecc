#include <stdio.h>
#include <string.h>

#include "../../src/ecc.h"

token_t* testutils_tokenize(char* tlu)
{
    FILE* file = fopen("temp.c", "w+");
    fwrite(tlu, sizeof(char), strlen(tlu), file);
    fseek(file, 0, SEEK_SET);
    token_t* tokens = lex(file);
    fclose(file);
    return tokens;
}