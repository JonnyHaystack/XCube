#include <GamecubeConsole.hpp>
#include <pico/stdlib.h>
#include <tusb.h>
#include <xinput_host.h>

volatile xinput_gamepad_t _xinput_report = {};

void tuh_xinput_report_received_cb(
    uint8_t dev_addr,
    uint8_t instance,
    const uint8_t *report,
    uint16_t len
) {
    xinputh_interface_t *xid_itf = (xinputh_interface_t *)report;
    xinput_gamepad_t *xinput_report = &xid_itf->pad;
    const char *type_str;
    switch (xid_itf->type) {
        case 1:
            type_str = "Xbox One";
            break;
        case 2:
            type_str = "Xbox 360 Wireless";
            break;
        case 3:
            type_str = "Xbox 360 Wired";
            break;
        case 4:
            type_str = "Xbox OG";
            break;
        default:
            type_str = "Unknown";
    }

    if (xid_itf->connected && xid_itf->new_pad_data) {
        TU_LOG1(
            "[%02x, %02x], Type: %s, Buttons %04x, LT: %02x RT: %02x, LX: %d, LY: %d, RX: %d, RY: %d\n",
            dev_addr,
            instance,
            type_str,
            xinput_report->wButtons,
            xinput_report->bLeftTrigger,
            xinput_report->bRightTrigger,
            xinput_report->sThumbLX,
            xinput_report->sThumbLY,
            xinput_report->sThumbRX,
            xinput_report->sThumbRY
        );

        _xinput_report.wButtons = xinput_report->wButtons;
        _xinput_report.bLeftTrigger = xinput_report->bLeftTrigger;
        _xinput_report.bRightTrigger = xinput_report->bRightTrigger;
        _xinput_report.sThumbLX = xinput_report->sThumbLX;
        _xinput_report.sThumbLY = xinput_report->sThumbLY;
        _xinput_report.sThumbRX = xinput_report->sThumbRX;
        _xinput_report.sThumbRY = xinput_report->sThumbRY;
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
