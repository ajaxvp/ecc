int (*x)[3];
int *x[(int) 3];
// ARRAY { NEST { POINTER { IDENTIFIER } } }
// POINTER { ARRAY { IDENTIFIER } }

//int *x[3];
//int *(x[3]);

// *(x[3]) = *x[3]
// (*x)[3]

// const restrict pointer to array of 3 const function pointers
//int (*const (*const restrict ptr)[3])(int, int);
// FUNCTION { NEST { POINTER { ARRAY { NEST { POINTER { IDENTIFIER } } } } } }
// POINTER { ARRAY { POINTER { FUNCTION { IDENTIFIER } } } }

//int (*(*x[3][4]))(int, int);

//const int y = 3;