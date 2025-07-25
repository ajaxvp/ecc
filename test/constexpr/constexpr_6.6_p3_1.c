/* ISO: 6.6 (3); invalid operators for constant expressions in evaluated sections */

int a;

// assignment operator
int b = a = 5;

// increment operators
int c = a += 1;
int d = a++;

// decrement operators
int e = a -= 1;
int f = a--;

// function call operator

static int g(void) {}

int h = g();

// comma operators
int i = (f, h);
