int main(void)
{
    int i = 3;
    switch (i)
    {
        case 3:
            ++i;
            break;
        case 1:
            --i;
            break;
        default:
            i += 3;
            break;
    }
}