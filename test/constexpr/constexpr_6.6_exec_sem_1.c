/* ISO: 6.6; semantics test - operator support */

#include "../test.h"

/*

what's allowed in integer constant expressions:
unary +, unary -, +, -, *, /, %, ~, &, |, ^, <<, >>, !,
&&, ||, ==, !=, <, >, <=, >=, [], unary *, unary &, ->, .,
(type) special case, ?:, non-VLA sizeof,
integer constants, enumeration constants, character constants,
float constants immediately converted to integer constants

arithmetic constant expressions is the above + regular floating constants and casts can convert between arithmetic types

[], unary *, unary &, ->, and . just won't happen in integer and arithmetic constant expressions, as far as i know

*/

enum e1
{
    E1_V1 = 0
};

struct s1
{
    int s1m1;
    int s1m2;
};

// integer constant expressions
int v1 = 5;
int v2 = E1_V1;
int v3 = 'a';
int v4 = (int) 5.0f;
int v5 = +12;
int v6 = -15;
int v7 = 1 + 2;
int v8 = 8 - 3;
int v9 = 2 * 8;
int v10 = 16 / 4;
int v11 = 12 % 3;
int v12 = ~1;
int v13 = 5 & 7;
int v14 = 4 | 3;
int v15 = 15 ^ 9;
int v16 = 1 << 3;
int v17 = 8 >> 3;
int v18 = !1;
int v19 = 5 && 3;
int v20 = 0 || 7;
int v21 = 3 == 3;
int v22 = 6 != 7;
int v23 = 7 < 9;
int v24 = 11 > 15;
int v25 = 19 <= 70;
int v26 = 70 >= 30;
int v27 = 1 ? 3 : 5;
int v28 = sizeof(int);
int v29 = sizeof((void*) 0x0);

// arithmetic constant expressions
float v30 = 5.0f;
double v31 = (double) 8.0f;

// address constant expressions

// a null pointer
int* v32 = (int*) 0;

// an integer constant cast to a pointer type
int* v33 = (int*) 0xFF;

int v34[2] = { 1, 2 };

// basic lvalue address constant
int* v35 = v34;

// subscript address constant
int* v36 = &v34[1];

struct s1 v37 = { 3, 4 };

// member access address constant
int* v38 = &v37.s1m2;

// fun times
int* v39 = &*&*&*&*&v37.s1m2;

// compound literal as lvalue for address constant
struct s1* v40 = &(struct s1){ 5, 7 };

// string literal as lvalue for address constant
char (*v41)[] = &"Hello, world!";

// static initializer constant expressions: a null pointer constant
void* v42 = 0;
void* v43 = (void*) 0;

// static initializer constant expression: plus or minus an integer constant expression
int* v44 = (int*) 0 + (4 * 3);
int* v45 = (4 * 3) + (int*) 0;
int* v46 = (int*) 0x40 - (4 * 3);

int main(void)
{
    ASSERT_EQUALS(v1, 5);
    ASSERT_EQUALS(v2, E1_V1);
    ASSERT_EQUALS(v3, 'a');
    ASSERT_EQUALS(v4, 5);
    ASSERT_EQUALS(v5, 12);
    ASSERT_EQUALS(v6, -15);
    ASSERT_EQUALS(v7, 3);
    ASSERT_EQUALS(v8, 5);
    ASSERT_EQUALS(v9, 16);
    ASSERT_EQUALS(v10, 4);
    ASSERT_EQUALS(v11, 0);
    ASSERT_EQUALS(v12, -2);
    ASSERT_EQUALS(v13, 5);
    ASSERT_EQUALS(v14, 7);
    ASSERT_EQUALS(v15, 6);
    ASSERT_EQUALS(v16, 8);
    ASSERT_EQUALS(v17, 1);
    ASSERT_EQUALS(v18, 0);
    ASSERT_EQUALS(v19, 1);
    ASSERT_EQUALS(v20, 1);
    ASSERT_EQUALS(v21, 1);
    ASSERT_EQUALS(v22, 1);
    ASSERT_EQUALS(v23, 1);
    ASSERT_EQUALS(v24, 0);
    ASSERT_EQUALS(v25, 1);
    ASSERT_EQUALS(v26, 1);
    ASSERT_EQUALS(v27, 3);
    ASSERT_EQUALS(v28, 4);
    ASSERT_EQUALS(v29, 8);
    ASSERT_EQUALS(v30, 5.0f);
    ASSERT_EQUALS(v31, 8.0);
    ASSERT_EQUALS(v32, (int*) 0);
    ASSERT_EQUALS(v33, (int*) 0xFF);
    ASSERT_EQUALS(v35, v34);
    ASSERT_EQUALS(*v36, 2);
    ASSERT_EQUALS(*v38, 4);
    ASSERT_EQUALS(*v39, 4);
    ASSERT_EQUALS(v40->s1m1, 5);
    ASSERT_EQUALS(v40->s1m2, 7);
    ASSERT_EQUALS((*v41)[0], 'H');
    ASSERT_EQUALS(v42, (void*) 0);
    ASSERT_EQUALS(v43, (void*) 0);
    ASSERT_EQUALS(v44, (int*) 0x30);
    ASSERT_EQUALS(v45, (int*) 0x30);
    ASSERT_EQUALS(v46, (int*) 0x10);
}
