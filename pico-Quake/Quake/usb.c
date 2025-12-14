#include "usb.h"
#include "usb_devices.h"

#include <stdio.h>

// With help from: https://github.com/raspberrypi/pico-examples/tree/master/usb/device/dev_lowlevel

// Pico
#include "pico/stdlib.h"

// For memcpy
//#include <string.h>

// USB register definitions from pico-sdk
#include "hardware/regs/usb.h"
// USB hardware struct definitions from pico-sdk
#include "hardware/structs/usb.h"
// For interrupt enable and numbers
#include "hardware/irq.h"
// For resetting the USB controller
#include "hardware/resets.h"

#define _BSIZE 0b00000011
#define _BTYPE 0b00001100
#define _BTAG  0b11110000

#define _USAGE_TAG          0b00000000
#define _COLLECTION_TAG     0b10100000
#define _END_COLLECTION_TAG 0b11000000

#define _MINIMUM_TAG        0b00010000
#define _MAXIMUM_TAG        0b00100000

#define _REPORT_COUNT_TAG   0b10010000
#define _REPORT_SIZE_TAG    0b01110000

#define _INPUT_TAG          0b10000000
#define _OUTPUT_TAG         0b10010000

typedef enum {
    MAIN =      0b00000000,
    GLOBAL =    0b00000100,
    LOCAL =     0b00001000,
    RESERVED =  0b00001100,
} tag_e;

typedef enum {
    POINTER                 = 0x01,
    MOUSE                   = 0x02,
    JOYSTICK                = 0x04,
    GAMEPAD                 = 0x05,
    KEYBOARD                = 0x06,
    KEYPAD,                 = 0x07,
    MULTI_AXIS_CONTROLLER   = 0x08,

    X                       = 0x30,
    Y                       = 0x31,
    Z                       = 0x32,
    ROTX                    = 0x33,
    ROTY                    = 0x34,
    ROTZ                    = 0x35,
    SLIDER                  = 0x36,
    DIAL                    = 0x37,
    WHEEL                   = 0x38,

    START_BTN               = 0x3d,
    SELECT_BTN              = 0x3e,

    DPAD_UP                 = 0x90,
    DPAD_DOWN               = 0x91,
    DPAD_RIGHT              = 0x92,
    DPAD_LEFT               = 0x93,

    INDEX_TRIGGER           = 0x94,
    PALM_TRIGGER            = 0x95,
    THUMBSTICK              = 0x96
} generic_desktop_page_e;

typedef enum {
    NUM_LOCK    = 0x01,
    CAPS_LOCK   = 0x02,
    SCROLL_LOCK = 0x04      //0x04 is 0b0..100, so the specification was right, I just didnt think
} led_e;

typedef enum {
    LEFTCONTROL  = 0xe0,
    LEFTSHIFT    = 0xe1,
    LEFTALT      = 0xe2,
    LEFTGUI      = 0xe3,
    RIGHTCONTROL = 0xe4,
    RIGHTSHIFT   = 0xe5,
    RIGHTALT     = 0xe6,
    RIGHTGUI     = 0xe7
} special_keys_e;

//Reference https://usb.org/sites/default/files/hut1_6.pdf
//Make as much of this stack free (registers and the like)

//Linked list of devices
static usb_address_t* devices; // = 0x?????

void USB_init( void ) {
    // set up VBUS detection
    // set as a device for 10 seconds
    // if not pluuged into a device by this point, then switch to host mode
    // set irq to handle new devices when they come

    // Enable USB controller, which defaults to device mode
    usb_hw->main_ctrl = USB_MAIN_CTRL_CONTROLLER_EN_BITS;
    // Set device "mode" to full speed
    usb_hw->sie_ctrl = USB_SIE_CTRL_PULLUP_EN_BITS;

    irq_set_exclusive_handler(USBCTRL_IRQ, &USB_device_init);
    irq_set_enabled(USBCTRL_IRQ, true);
    // When the pico is connected, or when a setup request is recieved, run the isr
    usb_hw->inte = USB_INTE_SETUP_REQ_BITS | USB_INTE_DEV_CONN_DIS_BITS;

    // Set an alarm and isr for 10 seconds, and when these seconds have passed, switch to host mode
    // Use 'ALARM_0' by setting the first bit of 'INTE' (the zeroth bit)
    timer_hw->inte = 1;
    // Set irq handler for alarm irq to switch the USB type to host
    irq_set_exclusive_handler(TIMER_IRQ_0, transfer_type);
    irq_set_enabled(TIMER_IRQ_0, true);

    // Set 'ALARM_0' to 10 seconds, or 10 million microseconds
    timer_hw->alarm[0] = 10000000;
}

static void transfer_type ( void ) {
    irq_set_enabled(TIMER_IRQ_0, false);
    irq_set_enabled(USBCTRL_IRQ, false);
    USB_host_init();
}

void USB_device_init( void ) {
    irq_set_enabled(TIMER_IRQ_0, false);
    irq_set_enabled(USBCTRL_IRQ, false);
}

void USB_host_init( void ) {
    // Set the pico to now be a USB host
    usb_hw->main_ctrl = USB_MAIN_CTRL_HOST_NDEVICE;

    // Reset the bus, just to be sure, and enable pulldown resistors for fullspeed
    // Enable keep alive (for archaic peripherals), Preamble, and SOF generation
    usb_hw->sie_ctrl = USB_SIE_CTRL_RESET_BUS | USB_SIE_CTRL_PULLDOWN_EN_BITS |
                       USB_SIE_CTRL_KEEP_ALIVE_EN | USB_SIE_CTRL_SOF_EN | USB_SIE_CTRL_PREAMBLE_EN;

    // Set to full speed
    usb_hw->sie_status = 2 << USB_SIE_STATUS_SPEED_LSB;
} 

//Offset 'data' by 4, and assert that the first 4 bytes are '05 01 09 xx', where the final byte
//indicates the type of device
hid_block_t* USB_hid_parse_report(uint8_t* data, uint32_t length) {
    qboolean next_block = true;
    hid_block_t* new_block = NULL;
    uint8_t z = 0;
    /*
        Get Device type here
    */
    for(int i = 3; i<length; ) {
        if(next_block) {
            hid_block_t* temp = new_block;
            new_block = malloc(sizeof(hid_block_t));
            new_block->prev = temp;
            next_block = false;
        }
        uint8_t header_length = data[i] & _BSIZE;
        uint8_t type =          data[i] & _BTYPE;
        uint8_t tag =           data[i] & _BTAG;
        i+=1;
        for(int x = 0; x<header_length; x+=1, i+=1) { //Header length rarely (if ever) is longer than 1, but just in case...
            switch (tag) {
                case _MINIMUM_TAG:
                case _MAXIMUM_TAG:
                    uint8_t offset = (tag == _MINIMUM_TAG);
                    if((tag_e)(type) == GLOBAL) { //Logical Maximum
                        (&new_block->Logical_Maximum)[offset]   = data[i+x];
                    } else {                      //Usage Maximum
                        (&new_block->Usage_Maximum)[offset]     = data[i+x];
                    }
                    break;
                case _REPORT_COUNT_TAG: //Number of bits
                    new_block->Report_Count = data[i];
                    break;
                case _REPORT_SIZE_TAG:  //Size in bits
                    new_block->Report_Size = data[i];
                    break;

                case _USAGE_TAG:
                    new_block->Usage_Page[x+z] = data[i];
                    if(header_length == 1) {z+=1;}
                    break;

                //case _COLLECTION_TAG: //Dont believe the specification operates with the Collection tab able to end a 'block'
                case _INPUT_TAG:
                case _OUTPUT_TAG:
                    z=0;
                    if(!(new_block->Report_Size)) {
                        new_block->Report_Size = new_block->prev->Report_Size;
                    }
                    if(!(new_block->Report_Count)) {
                        new_block->Report_Count = new_block->prev->Report_Count;
                    }
                    next_block = true;
                    break;

                default:
                    break;
            }
        }
    }
    return new_block;
}

