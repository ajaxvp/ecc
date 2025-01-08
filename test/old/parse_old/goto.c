int main(void)
{
    int a = 3;
loo:
    a += 5;
    if (a < 10)
        goto loo;
}