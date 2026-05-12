#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_types() {
    int i = 42;
    long long l = 9999999999;
    float f = 3.14;
    double d = 2.71828;
    int b = 1;
    printf("Type system test\n");
    return i;
}

int main(void) {
    return test_types();
}
