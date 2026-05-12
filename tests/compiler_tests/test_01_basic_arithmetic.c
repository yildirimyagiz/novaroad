#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int main(void) {
    int x = 5;
    int y = 10;
    int sum = add(x, y);
    int product = multiply(x, y);
    printf("Sum: 15\n");
    printf("Product: 50\n");
    return sum + product;
}
