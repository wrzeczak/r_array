#define RA_SILENT
#include "r_array.h"
// the above includes `RA_VECTOR2` and `RA_DOUBLE`

#include <stdlib.h>

char * vector_printer(void * value) {
    Vector2 v = *(Vector2 *) value;
    static char output_buffer[256];
    memset(output_buffer, 0, 256);

    sprintf(output_buffer, "<%.4f, %.4f>", v.x, v.y);

    return output_buffer;
}

int main(void) {
    r_array ra = ra_create(RA_INT);

    for(int i = 0; i < 10; i++) {
        ra_append(&ra, (i * i * i) - 13);
    }

    ra_printf(&ra, "%10d");
    ra_destroy(&ra);

    r_array rb = ra_create(RA_DOUBLE);

    for(int i = 0; i < 10; i++) {
        ra_append(&rb, 1.0f / (i + 1));
    }

    ra_printf(&rb, "%.4f");
    ra_destroy(&rb);

    r_array rc = ra_create(RA_VECTOR2);

    for(int i = 0; i < 10; i++) {
        ra_append(&rc, (Vector2) { (float) (rand() % 500), (float) (rand() % 500) });
    }

    r_array rc_slice = ra_slice(&rc, 2, -2);
    ra_printf_f(&rc_slice, &vector_printer);
    ra_destroy(&rc);

    return 0;
}