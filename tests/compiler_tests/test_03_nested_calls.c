#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int inner(int x) {
    return x * 2;
}

int middle(int x) {
    return inner(x) + 10;
}

int outer(int x) {
    return middle(x) * 3;
}

int main(void) {
    int result = outer(5);
    printf("Nested calls result: 90\n");
    return result;
}
