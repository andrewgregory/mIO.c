#include <time.h>

#include "tap.c/tap.c"
#include "mio.c"

#define ASSERT(x) if(!(x)) { tap_bail(#x " failed"); exit(1); }

#define COUNT 10000

int main(void) {
    FILE *stream;
    char buf[COUNT];
    long i = 0;

    ASSERT(stream = fopen("/dev/zero", "r"));

    tap_plan(COUNT);
    while(i++ < COUNT) {
        tap_ok(mio_fgets(buf, COUNT, stream) != NULL, "mio_getc %ld", i);
    }

    fclose(stream);
    return tap_finish();
}
