/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021, Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <GamecubeConsole.hpp>
#include <bsp/board.h>
#include <tusb.h>

extern volatile gc_report_t _gc_report;

typedef enum {
    DS4_HAT_UP,
    DS4_HAT_UP_RIGHT,
    DS4_HAT_RIGHT,
    DS4_HAT_DOWN_RIGHT,
    DS4_HAT_DOWN,
    DS4_HAT_DOWN_LEFT,
    DS4_HAT_LEFT,
    DS4_HAT_UP_LEFT,
    DS4_HAT_CENTERED,
} ds4_hat_t;

// Sony DS4 report layout detail https://www.psdevwiki.com/ps4/DS4-USB
typedef struct TU_ATTR_PACKED {
    uint8_t x;
    uint8_t y;
    uint8_t z;
    uint8_t rz;

    ds4_hat_t dpad : 4;
    bool square : 1;
    bool cross : 1;
    bool circle : 1;
    bool triangle : 1;

    bool l1 : 1;
    bool r1 : 1;
    bool l2 : 1;
    bool r2 : 1;
    bool share : 1;
    bool option : 1;
    bool l3 : 1;
    bool r3 : 1;

    bool ps : 1; // playstation button
    bool tpad : 1; // track pad click
    uint8_t counter : 6; // +1 each report

    uint8_t l2_trigger;
    uint8_t r2_trigger;

    // uint16_t timestamp;
    // uint8_t  battery;
    //
    // int16_t gyro[3];  // x, y, z;
    // int16_t accel[3]; // x, y, z

    // there is still lots more info
} sony_ds4_report_t;

// Check if device is Sony DualShock 4
inline static bool is_sony_ds4(uint8_t dev_addr) {
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    return (
        (vid == 0x054c && (pid == 0x09cc || pid == 0x05c4)) // Sony DualShock4
        || (vid == 0x0f0d && pid == 0x005e) // Hori FC4
        || (vid == 0x0f0d && pid == 0x00ee) // Hori PS4 Mini (PS4-099U)
        || (vid == 0x1f4f && pid == 0x1002) // ASW GG xrd controller
    );
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

void tuh_hid_mount_cb(
    uint8_t dev_addr,
    uint8_t instance,
    const uint8_t *desc_report,
    uint16_t desc_len
) {
    (void)desc_report;
    (void)desc_len;
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    TU_LOG1("HID device address = %d, instance = %d is mounted\r\n", dev_addr, instance);
    TU_LOG1("VID = %04x, PID = %04x\r\n", vid, pid);

    // Sony DualShock 4 [CUH-ZCT2x]
    if (is_sony_ds4(dev_addr)) {
        // request to receive report
        // tuh_hid_report_received_cb() will be invoked when report is available
        if (!tuh_hid_receive_report(dev_addr, instance)) {
            TU_LOG1("Error: cannot request to receive report\r\n");
        }
    }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    TU_LOG1("HID device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
}

void process_sony_ds4(const uint8_t *report, uint16_t len) {
    const char *dpad_str[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW", "none" };

    const uint8_t report_id = report[0];
    report++;
    len--;

    // all buttons state is stored in ID 1
    if (report_id == 1) {
        sony_ds4_report_t ds4_report;
        memcpy(&ds4_report, report, sizeof(ds4_report));

        TU_LOG1(
            "(x, y, z, rz) = (%u, %u, %u, %u)\n",
            ds4_report.x,
            ds4_report.y,
            ds4_report.z,
            ds4_report.rz
        );
        TU_LOG1("DPad = %s\n", dpad_str[ds4_report.dpad]);

        // Translate DS4 report to GameCube input report.
        _gc_report.a = ds4_report.cross;
        _gc_report.b = ds4_report.circle;
        _gc_report.x = ds4_report.square;
        _gc_report.y = ds4_report.triangle;
        _gc_report.start = ds4_report.option;
        _gc_report.dpad_left =
            DS4_HAT_DOWN_LEFT <= ds4_report.dpad && ds4_report.dpad <= DS4_HAT_UP_RIGHT;
        _gc_report.dpad_right =
            DS4_HAT_UP_RIGHT <= ds4_report.dpad && ds4_report.dpad <= DS4_HAT_DOWN_RIGHT;
        _gc_report.dpad_down =
            DS4_HAT_DOWN_RIGHT <= ds4_report.dpad && ds4_report.dpad <= DS4_HAT_DOWN_LEFT;
        _gc_report.dpad_up = ds4_report.dpad == DS4_HAT_UP || ds4_report.dpad == DS4_HAT_UP_RIGHT ||
                             ds4_report.dpad == DS4_HAT_UP_LEFT;
        _gc_report.z = ds4_report.r1;
        _gc_report.r = ds4_report.r2_trigger > 140;
        _gc_report.l = ds4_report.l2_trigger > 140;
        _gc_report.stick_x = ds4_report.x;
        _gc_report.stick_y = 255 - ds4_report.y;
        _gc_report.cstick_x = ds4_report.z;
        _gc_report.cstick_y = 255 - ds4_report.rz;
        _gc_report.l_analog = ds4_report.l2_trigger;
        _gc_report.r_analog = ds4_report.r2_trigger;
    }
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(
    uint8_t dev_addr,
    uint8_t instance,
    const uint8_t *report,
    uint16_t len
) {
    if (is_sony_ds4(dev_addr)) {
        process_sony_ds4(report, len);
    }

    // continue to request to receive report
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        TU_LOG1("Error: cannot request to receive report\r\n");
    }
}