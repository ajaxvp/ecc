/* ISO: 6.6 (3); invalid operators for constant expressions in unevaluated sections */

int a;

// assignment operator
int b = sizeof(a = 5);

// increment operators
int c = sizeof(a += 1);
int d = sizeof(a++);

// decrement operators
int e = sizeof(a -= 1);
int f = sizeof(a--);

// function call operator

static int g(void) {}

int h = sizeof(g());

// comma operators
int i = sizeof(f, h);
