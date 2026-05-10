#include <stdio.h>
#include "pico/stdlib.h"
#include "tusb.h"

int main(void) {
    stdio_init_all();
    tusb_init();

    while (1) {
        tud_task();
    }
    return 0;
}
