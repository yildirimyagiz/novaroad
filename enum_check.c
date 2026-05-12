#include <stdio.h>
#include "compiler/lexer.h"
int main() {
    printf("TOKEN_KEYWORD_PUB: %%d\n", TOKEN_KEYWORD_PUB);
    printf("TOKEN_KEYWORD_FN: %%d\n", TOKEN_KEYWORD_FN);
    printf("TOKEN_KEYWORD_NEW: %%d\n", TOKEN_KEYWORD_NEW);
    printf("TOKEN_KEYWORD_MATCH: %%d\n", TOKEN_KEYWORD_MATCH);
    return 0;
}
