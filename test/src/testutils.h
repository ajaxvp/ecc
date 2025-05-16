#ifndef TESTUTILS_H
#define TESTUTILS_H

#include "../../src/ecc.h"

program_options_t* get_program_options(void);
syntax_component_t* quickparse(char* tlu_str);

#endif