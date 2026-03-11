#define RA_SILENT
#include "r_array.h"

#include <stdlib.h>

int main(void) {
    r_array strings = ra_create(RA_STR);

    ra_append(&strings, "A really long string of text that goes over the limit of the internal dynamic memory of this library.");

    return 0;
}