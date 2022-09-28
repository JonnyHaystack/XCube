#include "filters.hpp"

#include <GamecubeConsole.hpp>
#include <bsp/board.h>
#include <hardware/pio.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <tusb.h>
#include <xinput_host.h>

#define REV1 10
#define REV2 20
#define REV2_2 22

#define REVISION REV2
#define WONKY false

#if REVISION == REV1
#define GC_DATA_PIN 0
#elif REVISION == REV2 && !WONKY
#define GC_DATA_PIN 0
#define GC_3V3_PIN 1
#elif REVISION == REV2 && WONKY
#define GC_DATA_PIN 1
#define GC_3V3_PIN 0
#elif REVISION == REV2_2
#define GC_DATA_PIN 28
#define GC_3V3_PIN 27
#endif

void joybus_task();

volatile gc_report_t _gc_report = default_gc_report;

int main() {
    board_init();

    // Required for joybus-pio.
    set_sys_clock_khz(130'000, true);

    // Debug serial output.
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);

    // Turn on LED.
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

#if REVISION >= REV2
    // Reboot into bootsel mode if GC 3.3V not detected.
    gpio_init(GC_3V3_PIN);
    gpio_set_dir(GC_3V3_PIN, GPIO_IN);
    gpio_pull_down(GC_3V3_PIN);

    sleep_ms(200);

    if (!gpio_get(GC_3V3_PIN))
        reset_usb_boot(0, 0);
#endif

    multicore_launch_core1(joybus_task);

    tusb_init();

    while (1) {
        tuh_task();
    }

    return 1;
}

void joybus_task() {
    GamecubeConsole *gc = new GamecubeConsole(GC_DATA_PIN, pio0);

    while (1) {
        gc->WaitForPoll();

        // Take snapshot of GC report.
        gc_report_t gc_report = default_gc_report;

        gc_report.a = _gc_report.a;
        gc_report.b = _gc_report.b;
        gc_report.x = _gc_report.x;
        gc_report.y = _gc_report.y;
        gc_report.start = _gc_report.start;
        gc_report.dpad_left = _gc_report.dpad_left;
        gc_report.dpad_right = _gc_report.dpad_right;
        gc_report.dpad_down = _gc_report.dpad_down;
        gc_report.dpad_up = _gc_report.dpad_up;
        gc_report.z = _gc_report.z;
        gc_report.r = _gc_report.r;
        gc_report.l = _gc_report.l;
        gc_report.stick_x = _gc_report.stick_x;
        gc_report.stick_y = _gc_report.stick_y;
        gc_report.cstick_x = _gc_report.cstick_x;
        gc_report.cstick_y = _gc_report.cstick_y;
        gc_report.l_analog = _gc_report.l_analog;
        gc_report.r_analog = _gc_report.r_analog;

        TU_LOG1("GCC: (%d, %d)\n", gc_report._stick_x, gc_report._stick_y);
        TU_LOG1("GCC after deadzone and scaling: (%d, %d)\n", gc_report.stick_x, gc_report.stick_y);

        gc->SendReport(&gc_report);
    }
}
