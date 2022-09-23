#include <GamecubeConsole.hpp>
#include <bsp/board.h>
#include <hardware/pio.h>
#include <pico/bootrom.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <tusb.h>
#include <xinput_host.h>

#define GC_DATA_PIN 2

void joybus_task();

extern volatile xinput_gamepad_t _xinput_report;

int main() {
    board_init();

    // Required for joybus-pio.
    set_sys_clock_khz(130'000, true);

    // Debug serial output.
    uart_init(uart0, 115200);

    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);

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

        gc_report_t gc_report = default_gc_report;

        gc_report.a = _xinput_report.wButtons & XINPUT_GAMEPAD_A;
        gc_report.b = _xinput_report.wButtons & XINPUT_GAMEPAD_B;
        gc_report.x = _xinput_report.wButtons & XINPUT_GAMEPAD_X;
        gc_report.y = _xinput_report.wButtons & XINPUT_GAMEPAD_Y;
        gc_report.start = _xinput_report.wButtons & XINPUT_GAMEPAD_START;
        gc_report.dpad_left = _xinput_report.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
        gc_report.dpad_right = _xinput_report.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
        gc_report.dpad_down = _xinput_report.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
        gc_report.dpad_up = _xinput_report.wButtons & XINPUT_GAMEPAD_DPAD_UP;
        gc_report.z = _xinput_report.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
        gc_report.r = _xinput_report.bRightTrigger > 140;
        gc_report.l = _xinput_report.bLeftTrigger > 140;
        gc_report.stick_x = (_xinput_report.sThumbLX + 32768) / 257;
        gc_report.stick_y = (_xinput_report.sThumbLY + 32768) / 257;
        gc_report.cstick_x = (_xinput_report.sThumbRX + 32768) / 257;
        gc_report.cstick_y = (_xinput_report.sThumbRY + 32768) / 257;
        gc_report.l_analog = _xinput_report.bLeftTrigger;
        gc_report.r_analog = _xinput_report.bRightTrigger;
        TU_LOG1(
            "%d, %d -> %d, %d\n",
            _xinput_report.sThumbLX,
            _xinput_report.sThumbLY,
            gc_report.stick_x,
            gc_report.stick_y
        );

        gc->SendReport(&gc_report);
    }
}
