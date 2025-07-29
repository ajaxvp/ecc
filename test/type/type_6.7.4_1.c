/* ISO: 6.7.4; function specifier constraints */

static int z;

inline int g(void)
{
    // ISO: 6.7.4 (3) - valid, const-qualified
    const static int y;
    // ISO: 6.7.4 (3) - invalid, static storage duration but modifiable
    static int x;

    // ISO: 6.7.4 (3) - invalid, internal linkage identifier referenced
    z;
}

// ISO: 6.7.4 (4) - invalid, inline may not be used in declaration of main
inline int main(void);
