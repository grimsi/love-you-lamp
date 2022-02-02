#ifndef CONFIG_H
#define CONFIG_H


#include "structs/Device.h++"
#include "structs/MACAddress.h++"
#include "structs/RGBColor.h++"

// After how many ms of not receiving a message from a device it will be marked as "unavailable"
// Default value is 15 Minutes (900000 ms)
#define DEVICE_TIMEOUT_MILLIS 900000

// How long to wait between two device status updates in ms
// Default value is 1 second (1000 ms)
#define DEVICE_STATUS_UPDATE_INTERVAL_MILLIS 1000

// How long to wait between two device status reports in ms
// Default value is 1 second (1000 ms)
#define DEVICE_STATUS_REPORT_INTERVAL_MILLIS 10000

// How long to wait between two heartbeats if the effect is active
// Default value is 30 seconds (10000 ms)
#define HEARTBEAT_EFFECT_INTERVAL_MILLIS 30000

// Pins where the LED is connected to
#define LED_RED_PIN D1
#define LED_GREEN_PIN D2
#define LED_BLUE_PIN D3

// Define all devices that should be tracked here
// A device consists of - a name (choose your own name for each device, it's only used for debug logging)
//                      - a mac address (that's the important part)
//                      - a color (the LED will light up in this color when the device is detected)
Device* devices[] = {
        //new Device("Anna-Lena iPhone", new MACAddress(0x82, 0xae, 0x30, 0x88, 0xd2, 0xc3), new RGBColor(0, 0, 255)),
        new Device("Simon iPad", new MACAddress(0x72, 0xF7, 0x0A, 0x4F, 0x5C, 0x37), new RGBColor(0, 0, 255)),
        new Device("Simon Handy", new MACAddress(0x36, 0x56, 0xdb, 0x3b, 0xc2, 0x14), new RGBColor(0, 200, 0))
};


#endif