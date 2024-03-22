// probably the simplest: a single type specifier
int;
// a little more complicated, a type with two words
unsigned int;
// mix around the order
int unsigned;
// let's introduce a storage class specifier
static unsigned int;
// mix it around
int static unsigned;
// now a four word arithmetic type qualifier with some random storage class specifier in the middle
long unsigned extern int long;
// crazy
long const long static volatile unsigned int;