/**
 * @file main.c
 * @brief Nova debugger
 */

#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: zndbg <program>\n");
        return 1;
    }
    
    printf("Nova Debugger v0.1.0\n");
    printf("Debugging: %s\n", argv[1]);
    
    /* TODO: Implement debugger */
    
    return 0;
}
