#include <stdint.h>
#include "tusb.h"
#include "hid.h"
#include "ScanKeys.h"

static uint8_t LastKeyBits;

struct {
    uint8_t code;
    uint8_t bit;
} const KeyMap[] = {
    { HID_KEY_ENTER, Keys_Button1 },
    { HID_KEY_SPACE, Keys_Button0 },
    { HID_KEY_ARROW_RIGHT, Keys_Right },
    { HID_KEY_ARROW_LEFT, Keys_Left },
    { HID_KEY_ARROW_DOWN, Keys_Down },
    { HID_KEY_ARROW_UP, Keys_Up },
    { 0 }
};

void InitKeys()
{
    LastKeyBits = 0;
    tusb_init();
}

extern "C" {

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_BOOT);
    tuh_hid_receive_report(dev_addr, instance);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, const uint8_t *report, uint16_t len)
{   
    LastKeyBits = 0;
    auto pMap = KeyMap;
    for (int i = 2; i < 8; i++) {
        while (true) {
            auto code = pMap->code;
            if (code == 0) {
                break;
            }
            if (code == report[i]) {
                LastKeyBits |= pMap->bit;
            }
            ++pMap;
        }
    }
    tuh_hid_receive_report(dev_addr, instance);
}

}

void UpdateKeys()
{
    tuh_task();
}

uint8_t ScanKeys()
{
    return LastKeyBits;
}
