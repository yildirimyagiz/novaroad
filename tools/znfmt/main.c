/**
 * @file main.c
 * @brief Nova code formatter
 */

#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: znfmt <file.nova>\n");
        return 1;
    }
    
    printf("Formatting %s...\n", argv[1]);
    /* TODO: Implement formatter */
    
    return 0;
}
