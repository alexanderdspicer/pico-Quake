#ifndef PICOQUAKE_USB_H
#define PICOQUAKE_USB_H

#include <stdint.h>

//Thankfully the Rpi Pico (and its flavours) are little endian, so no conversions need be made

typedef enum {
    DEVICE =                    0x01,
    CONFIGURATION =             0x02,
    STRING =                    0x03,
    INTERFACE =                 0x04,
    ENDPOINT =                  0x05,
    DEVICE_QUALIFIER =          0x06,
    OTHER_SPEED_CONFIGURATION = 0x07,
    INTERFACE_POWER =           0x08,

    HID =                       0x21,
    HID_REPORT =                0x22
} descriptor_types_e

/*typedef struct {
    uint16_t    len;

    uint32_t    irp_id_1;
    uint32_t    irp_id_2;

    uint32_t    usbd_stat;
    uint8_t     irp_info; //1, Rx | 0, Tx
    uint8_t     urb_bus;
    uint16_t    device_address;
    uint8_t     endpoint;
    uint8_t     transfer_type;
    uint32_t    packet_len;
} usb_urb_t;*/

//bmRequestType Masks
#define _HOST_TO_DEVICE 0b00000000
#define _DEVICE_TO_HOST 0b10000000

#define _TYPE_STANDARD  0b00000000
#define _TYPE_CLASS     0b00100000
#define _TYPE_VENDOR    0b01000000
#define _TYPE_RESERVED  0b01100000

#define _RECIPIENT_DEVICE       0b00000000
#define _RECIPIENT_INTERFACE    0b00000001
#define _RECIPIENT_ENDPOINT     0b00000010
#define _RECIPIENT_OTHER        0b00000011

typedef enum {
    GET_STATUS =        0x00,
    CLEAR_FEATURE =     0x01,
    SET_FEATURE =       0x03,
    SET_ADDRESS =       0x05,
    GET_DESCRIPTOR =    0x06,
    SET_DESCRIPTOR =    0x07,
    GET_CONFIGURATION = 0x08,
    SET_CONFIGURATION = 0x09
} standard_device_requests_e;

typedef enum {
    GET_STATUS =        0x00,
    CLEAR_FEATURE =     0x01,
    SET_FEATURE =       0x03,
    SET_INTERFACE =     0x11,
    GET_INTERFACE =     0x0a
} standard_interface_requests_e;

typedef enum {
    GET_STATUS =        0x00,
    CLEAR_FEATURE =     0x01,
    SET_FEATURE =       0x03,
    SYNCH_FRAME =       0x12
} standard_endpoint_requests_e;

typedef struct {
    uint8_t     bmRequestType;
    uint8_t     bRequest;
    uint16_t    wValue;
    uint16_t    wIndex;
    uint16_t    wLength;
} usb_setup_t;

//===============================================

//bmAttributes Masks
#define _SELF_POWERED   0b01000000
#define _REMOTE_WAKE    0b00100000

typedef struct { //Fix Alignment
    uint8_t     bLength;
    uint8_t     bDescriptorType; // = 0x02
    uint16_t    wTotalLength;
    uint8_t     bNumInterfaces;
    uint8_t     bConfigurationValue;
    uint8_t     iConfiguration;
    uint8_t     bmAttributes;
    uint8_t     bMaxPower;
    void*       data;
} usb_config_descriptor;

//===============================================

typedef struct { //Fix Alignment
    uint8_t     bLength;
    uint8_t     bDescriptorType; // = 0x00
    uint16_t    bcdUSB;
    uint8_t     bDeviceClass;
    uint8_t     bDeviceSubClass;
    uint8_t     bDeviceProtocol;
    uint8_t     bMaxPacketSize0;
    uint16_t    idVendor;
    uint16_t    idProduct;
    uint16_t    bcdDevice;
    uint8_t     iManufacturer;
    uint8_t     iProduct;
    uint8_t     iSerialNumber;
    uint8_t     bNumConfigurations;
} usb_device_descriptor;

//===============================================

//As of September 22, 2023
//https://www.usb.org/defined-class-codes
typedef enum {
    DEVICE =                0x00,
    AUDIO =                 0x01,
    CDC =                   0x02,
    HID =                   0x03,
    PHYSICAL =              0x05,
    IMAGE =                 0x06,
    PRINTER =               0x07,
    MASS_STORAGE =          0x08,
    HUB =                   0x09,
    CDC_DATA =              0x0a,
    SMART_CARD =            0x0b,
    CONTENT_SECURITY =      0x0d,
    VIDEO =                 0x0e,
    PERSONAL_HEALTHCARE =   0x0f,
    AV_DEVICE =             0x10,
    BILLBOARD =             0x11,
    TYPE_C_BRIDGE =         0x12,
    BULK_DISPLAY =          0x13,
    MCTP =                  0x14,
    I3C =                   0x3c,
    DIAG_DEVICE =           0xdc,
    WIRELESS_CONTROLLER =   0xe0,
    MISC =                  0xef
} class_codes_e;

typedef struct { //Alignment?
    uint8_t     bLength;
    uint8_t     bDescriptorType; // = 0x04
    uint8_t     bInterfactypedef enumber;
    uint8_t     bAlternateSetting;
    uint8_t     bNumEndpoints;
    uint8_t     bInterfaceClass;
    uint8_t     bInterfaceSubClass;
    uint8_t     bInterfaceProtocol;
    uint8_t     iInterface;
} interface_descriptor_t;

//===============================================

typedef enum {
    NOT_SUPPORTED =     0x00,
    ARABIC =            0x01,
    BELGIAN =           0x02,
    CANADIAN_BILINGUAL= 0x03,
    CANADIAN_FRENCH =   0x04
    CZECHIA =           0x05,
    DANISH =            0x06,
    FINNISH =           0x07,
    FRENCH =            0x08,
    GERMAN =            0x09,
    GREEK =             0x0a,
    HEBREW =            0x0b,
    HUNGARY =           0x0c,
    INTERNATIONAL =     0x0d,
    ITALIAN =           0x0e,
    KATAKANA =          0x0f,
    KOREAN =            0x10,
    LATIN_AMERICA =     0x11,
    NETHERLANDS =       0x12,
    NORWEGIAN =         0x13,
    PERSIAN =           0x14,
    POLAND =            0x15,
    PORTUGUESE =        0x16,
    RUSSIAN =           0x17,
    SLOVAKIA =          0x18,
    SPANISH =           0x19,
    SWEDISH =           0x1a,
    SWISS_FRENCH =      0x1b,
    SWISS_GERMAN =      0x1c,
    SWITZERLAND =       0x1d,
    TAIWAN =            0x1e,
    TURKISH_Q =         0x1f,
    UK =                0x20,
    US =                0x21,
    YUGOSLAVIA =        0x22, //????, check date on this entry
    TURKISH_F =         0x23
} country_codes_e;

typedef struct { //Fix alignment
    uint8_t             bLength;
    uint8_t             bDescriptorType; // = 0x21
    uint16_t            bcdHID;
    country_codes_e     bCountryCode;
    uint8_t             bNumDescriptors;
    uint8_t             bDescriptorType;
    uint16_t            wDescriptorLength;
} hid_descriptor_t;

//===============================================

typedef struct {
    uint8_t     endpoint  : 4;
    uint8_t     reserved  : 3;
    uint8_t     direction : 1; //0, Tx | 1, Rx
} endpoint_address_t;

//bmAttributes Masks
#define _CONTROL_TRANSFER_T     0b00000000
#define _ISOCHRONOUS_TRANSFER_T 0b00000001
#define _BULK_TRANSFER_T        0b00000010
#define _INTERUPT_TRANSFER_T    0b00000011

//ONLY IF ISOSYNCHRONOUS TRANSFER*******
#define _SYNCH_TYPE_NONE        0b00000000
#define _SYNCH_TYPE_ASYNCH      0b00000100
#define _SYNCH_TYPE_ADAPTIVE    0b00001000
#define _SYNCH_TYPE_SYNCH       0b00001100

#define _USAGE_TYPE_DATA_END                    0b00000000
#define _USAGE_TYPE_FEEDBACK_END                0b00010000
#define _USAGE_TYPE_EXPLICIT_FEEDBACK_DATA_END  0b00100000
#define _USAGE_TYPE_RESERVED                    0b00110000
//**************************************

typedef struct {
    uint8_t     bLength;
    uint8_t     bDescriptorType; // = 0x05
    uint8_t     bEndpointAddress;
    uint8_t     bmAttributes;
    uint16_t    wMaxPacketSize;
    uint8_t     bInterval;
} endpoint_descriptor_t;

//===============================================

typedef struct hid_block_s {
    uint8_t         Usage_Page[4]; //God forbid anything use more than 4 Usage pages at a time
    uint32_t        Usage_Maximum;
    uint32_t        Usage_Minimum;
    int32_t         Logical_Maximum;
    int32_t         Logical_Minimum;
    hid_block_s*    next;
    uint8_t         Report_Size;
    uint8_t         Report_Count;
} hid_block_t;

typedef struct usb_address_s {
    uint8_t     usage_page;
    uint8_t     usage;
    uint8_t     bus;
    uint8_t     device_address;
    uint8_t     endpoint;
    //usb_address_s* prev;
} usb_address_t;

#define LEFT_CONTROL    0b00000001
#define LEFT_SHIFT      0b00000010
#define LEFT_ALT        0b00000100
#define LEFT_GUI        0b00001000

#define RIGHT_CONTROL   0b00010000
#define RIGHT_SHIFT     0b00100000
#define RIGHT_ALT       0b01000000
#define RIGHT_GUI       0b10000000

#define ANY_CONTROL     LEFT_CONTROL | RIGHT_CONTROL
#define ANY_SHIFT       LEFT_SHIFT | RIGHT_SHIFT
#define ANY_ALT         LEFT_ALT | RIGHT_ALT
#define ANY_GUI         LEFT_GUI | RIGHT_GUI

void USB_init(); //SET ISOSYNCHRONOUS TRANSFER MODE (and check if its support is required in HID)

void* USB_configure(usb_address_t* address);

hid_block_t* USB_hid_parse_report(uint8_t* data, uint32_t length);

#endif