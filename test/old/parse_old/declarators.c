// very simple declarator 'a' tacked onto this declaration
int a;
// here's a nested declarator:
int (a);
// here's a pointer declarator:
int *a;
// here's an array declarator:
int a[];
// here's a function declarator:
int sum(int, int);
// here's one with parameters with abstract declarators:
int do_stuff(int*, unsigned long*);
// nested array declarator:
int b[][];
// function pointer:
int (*func)(int);
// const pointer to a const int (pointer qualifiers):
const int *const a;
// multiple declarators
int a, b;
// multiple function declarators
int add(int, int), sub(int, int);
// 0-arg function
int side_effect(void);
// K&R empty
int anything();
// K&R identifier list
int mul(a, b);
// variadic
int add_all(int len, ...);
// pointer to an array of length 3 (ARRAY { NEST { POINTER { IDENTIFIER } } })
int (*c)[3];
// an array of 3 int pointers (POINTER { ARRAY { IDENTIFIER } })
int *c[3];
// array of 5 function pointers
int (*d[5])(int, int);
int* w(int);