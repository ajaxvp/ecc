#include <stdlib.h>

#include "cc.h"

typedef struct preprocessing_state
{
    // some type of symbol table?
} preprocessing_state_t;

#define init_preprocess preprocessor_token_t* pp_token = *pp_tokens;

token_t* preprocess_preprocessing_file(preprocessor_token_t** pp_tokens)
{

}

token_t* preprocess(preprocessor_token_t* pp_tokens)
{
    return preprocess_preprocessing_file(&pp_tokens);
}