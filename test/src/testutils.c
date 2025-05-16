#include <stdio.h>
#include <string.h>

#include "../../src/ecc.h"

program_options_t opts = { .iflag = false };

program_options_t* get_program_options(void)
{
    return &opts;
}

syntax_component_t* quickparse(char* tlu_str)
{
    preprocessing_token_t* pp_tokens = lex_raw((unsigned char*) tlu_str, strlen(tlu_str), false);
    if (!pp_tokens)
        return NULL;
    
    preprocessing_settings_t settings;
    settings.filepath = ".";
    char pp_error[MAX_ERROR_LENGTH];
    settings.error = pp_error;
    if (!preprocess(&pp_tokens, &settings))
    {
        pp_token_delete_all(pp_tokens);
        return NULL;
    }
    
    charconvert(pp_tokens);
    strlitconcat(pp_tokens);

    tokenizing_settings_t tk_settings;
    tk_settings.filepath = ".";
    char tok_error[MAX_ERROR_LENGTH];
    tk_settings.error = tok_error;
    tk_settings.error[0] = '\0';

    token_t* tokens = tokenize(pp_tokens, &tk_settings);
    if (tk_settings.error[0])
    {
        pp_token_delete_all(pp_tokens);
        printf("%s", tk_settings.error);
        return NULL;
    }

    pp_token_delete_all(pp_tokens);

    syntax_component_t* tlu = parse(tokens);
    if (!tlu)
        return NULL;
    token_delete_all(tokens);
    return tlu;
}