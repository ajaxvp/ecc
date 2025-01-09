/* [6.7_1_2_a.c] declarators for declarations */

int a;                  // basic identifier declarator
int *b;                 // declarator w/ pointer
int (*c);               // nested declarator
int d[];                // simple array declarator
int e(int x, int y);    // simple function declarator
int f();                // K&R untyped n-parameter function
int g(x, y);            // K&R untyped 2-parameter function
int h(int x, ...);      // variadic function