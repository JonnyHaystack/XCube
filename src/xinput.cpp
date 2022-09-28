#include "filters.hpp"

#include <GamecubeConsole.hpp>
#include <pico/stdlib.h>
#include <tusb.h>
#include <xinput_host.h>

extern volatile gc_report_t _gc_report;

constexpr uint8_t _deadzone = PERCENT_TO_STICK_VAL(6);
constexpr uint8_t _radius = PERCENT_TO_STICK_VAL(80);

void tuh_xinput_report_received_cb(
    uint8_t dev_addr,
    uint8_t instance,
    const uint8_t *report,
    uint16_t len
) {
    xinputh_interface_t *xid_itf = (xinputh_interface_t *)report;
    xinput_gamepad_t *xinput_report = &xid_itf->pad;
    const char *type_str[] = {
        "Unknown", "Xbox One", "Xbox 360 Wireless", "Xbox 360 Wired", "Xbox OG",
    };
    const size_t num_types = sizeof(type_str) / sizeof(type_str[0]);

    if (xid_itf->connected && xid_itf->new_pad_data) {
        TU_LOG1(
            "[%02x, %02x], Type: %s, Buttons %04x, LT: %02x RT: %02x, LX: %d, LY: %d, RX: %d, RY: %d\n",
            dev_addr,
            instance,
            xid_itf->type >= num_types ? type_str[0] : type_str[xid_itf->type],
            xinput_report->wButtons,
            xinput_report->bLeftTrigger,
            xinput_report->bRightTrigger,
            xinput_report->sThumbLX,
            xinput_report->sThumbLY,
            xinput_report->sThumbRX,
            xinput_report->sThumbRY
        );

        // Translate XInput report to GameCube input report.
        _gc_report.a = xinput_report->wButtons & XINPUT_GAMEPAD_A;
        _gc_report.b = xinput_report->wButtons & XINPUT_GAMEPAD_B;
        _gc_report.x = xinput_report->wButtons & XINPUT_GAMEPAD_X;
        _gc_report.y = xinput_report->wButtons & XINPUT_GAMEPAD_Y;
        _gc_report.start = xinput_report->wButtons & XINPUT_GAMEPAD_START;
        _gc_report.dpad_left = xinput_report->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
        _gc_report.dpad_right = xinput_report->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
        _gc_report.dpad_down = xinput_report->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
        _gc_report.dpad_up = xinput_report->wButtons & XINPUT_GAMEPAD_DPAD_UP;
        _gc_report.z = xinput_report->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
        _gc_report.r = xinput_report->bRightTrigger > 140;
        _gc_report.l = xinput_report->bLeftTrigger > 140;
        _gc_report.stick_x = apply_radius(
            apply_deadzone((xinput_report->sThumbLX + 32768) / 257, _deadzone, true),
            _radius
        );
        _gc_report.stick_y = apply_radius(
            apply_deadzone((xinput_report->sThumbLY + 32768) / 257, _deadzone, true),
            _radius
        );
        _gc_report.cstick_x = apply_radius(
            apply_deadzone((xinput_report->sThumbRX + 32768) / 257, _deadzone, true),
            _radius
        );
        _gc_report.cstick_y = apply_radius(
            apply_deadzone((xinput_report->sThumbRY + 32768) / 257, _deadzone, true),
            _radius
        );
        _gc_report.l_analog = xinput_report->bLeftTrigger;
        _gc_report.r_analog = xinput_report->bRightTrigger;
    }
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_mount_cb(
    uint8_t dev_addr,
    uint8_t instance,
    const xinputh_interface_t *xinput_itf
) {
    TU_LOG1("XINPUT MOUNTED %02x %d\n", dev_addr, instance);
    // If this is a Xbox 360 Wireless controller we need to wait for a connection packet
    // on the in pipe before setting LEDs etc. So just start getting data until a controller is
    // connected.
    if (xinput_itf->type == XBOX360_WIRELESS && xinput_itf->connected == false) {
        tuh_xinput_receive_report(dev_addr, instance);
        return;
    }
    tuh_xinput_set_led(dev_addr, instance, 0, true);
    tuh_xinput_set_led(dev_addr, instance, 1, true);
    tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance) {
    TU_LOG1("XINPUT UNMOUNTED %02x %d\n", dev_addr, instance);
}
