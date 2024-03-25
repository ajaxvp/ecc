int main(void)
{
    int a = 5;
    int b = a+++ ++a;
    // ^ should be: a++ + ++a
    int c = a || b ^ b;
    int d = a ? b : c;
    unsigned long long c = (unsigned long long) (a + b);
}