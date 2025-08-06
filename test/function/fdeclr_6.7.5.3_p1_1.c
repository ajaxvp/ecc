/* ISO: 6.7.5.3 (1); function declarator constraint: no function or array return type */

int (f(int x))[5];

int (f(int x))(void);
