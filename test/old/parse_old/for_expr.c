int doSomething(int a);

int main(void)
{
    int a = 5;
    for (a += 1; a >= -10; a -= 2)
        doSomething(a);
    return 0;
}