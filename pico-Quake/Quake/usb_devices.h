#ifndef PICOQUAKE_USB_DEVICES_H
#define PICOQUAKE_USB_DEVICES_H

typedef enum {
    GENERIC_DESKTOP_CONTROLS     = 0x01,
    GAME_CONTROLS_PAGE           = 0x05,
    GENERIC_DEVICE_CONTROLS_PAGE = 0x06,
    KEYBOARD_KEYPAD_PAGE         = 0x07,
    LED_PAGE                     = 0x08,
    BUTTON_PAGE                  = 0x09
} usage_page_e;

typedef enum {
    PHYSICAL    = 0x00,
    APPLICATION = 0x01,
    LOGICAL     = 0x02
} collection_type_e;

#endif