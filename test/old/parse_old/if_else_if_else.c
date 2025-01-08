// ik else if isn't real but why not test anyway
int main(void)
{
    int i = 3;
    if (i)
        ++i;
    else if (i == 8)
        i += 2;
    else
        --i;
}