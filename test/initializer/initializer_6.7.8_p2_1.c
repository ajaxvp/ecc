/* ISO: 6.7.8 (2); out of bounds initializations */

int ys[1] = { 5, 6 };

int main(void)
{
    int xs[1] = { 2, 3 };

    (int[1]){ 3, 5 };
}
