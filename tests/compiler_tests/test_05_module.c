#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int module_function() {
    printf("Module function called\n");
    return 100;
}

int main(void) {
    return module_function();
}
