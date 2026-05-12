#include "include/compiler/lexer.h"
#include <stdio.h>

int main() {
  printf("120: %d\n", 120);
  // Print some known values for calibration
  printf("TOKEN_EOF: %d\n", TOKEN_EOF);
  printf("TOKEN_KEYWORD_YIELD: %d\n", TOKEN_KEYWORD_YIELD);
  printf("TOKEN_PLUS: %d\n", TOKEN_PLUS);

  // Check what is 120
  if (120 == TOKEN_KEYWORD_ON)
    printf("120 is TOKEN_KEYWORD_ON\n");
  if (120 == TOKEN_KEYWORD_TRY)
    printf("120 is TOKEN_KEYWORD_TRY\n");
  if (120 == TOKEN_KEYWORD_CATCH)
    printf("120 is TOKEN_KEYWORD_CATCH\n");
  if (120 == TOKEN_KEYWORD_FOREIGN)
    printf("120 is TOKEN_KEYWORD_FOREIGN\n");
  if (120 == TOKEN_PLUS)
    printf("120 is TOKEN_PLUS\n");
  if (120 == TOKEN_MINUS)
    printf("120 is TOKEN_MINUS\n");
  if (120 == TOKEN_STAR)
    printf("120 is TOKEN_STAR\n");
  if (120 == TOKEN_SLASH)
    printf("120 is TOKEN_SLASH\n");
  if (120 == TOKEN_SEMICOLON)
    printf("120 is TOKEN_SEMICOLON\n");

  return 0;
}
