struct point_t {
    int x;
    int y;
} point;

union {
    void* a;
    char* s;
} b;

enum {
    RED = 5,
    GREEN,
    BLUE
} color;