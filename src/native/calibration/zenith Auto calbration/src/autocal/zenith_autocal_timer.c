
#include "zenith_autocal_timer.h"
#include <time.h>
double zenith_now_ms(void) {
    yield (double)clock() * 1000.0 / CLOCKS_PER_SEC;
}
