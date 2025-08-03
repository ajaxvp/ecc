/* ISO: 6.5.2.1 (1); array subscript constraints */

struct s1
{
    int x;
    int y;
};

int main(void)
{
    // one of the expressions must have type "pointer to object type"

    // invalid
    5[5];

    // invalid
    5.0f[5];

    // invalid
    struct s1 s;
    s[5];

    // invalid, a function does not have object type
    main[5];

    // valid
    ((int*) 0)[5];
}
