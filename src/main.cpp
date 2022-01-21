#include <Arduino.h>
#include "./esppl_functions.h"
#include "Device.h"

// After how many ms of not receiving a message from a device it will be marked as "unavailable"
// Default value is 15 Minutes (900000 ms)
#define DEVICE_TIMEOUT_MILLIS 900000

// How long to wait between two device status reports in ms
// Default value is 10 seconds (10000 ms)
#define DEVICE_STATUS_REPORT_INTERVAL_MILLIS 10000

// If notification messages should be sent over serial port when a device packet is detected (makes debugging easier)
// Default value is false
#define SEND_DEVICE_NOTIFICATIONS true

void callback(esppl_frame_info *info);
bool is_device_active(Device *device);
void print_device_status(void *pArg);
void setup_interrupts();

Device* devices[] = {
        new Device("Anna-Lena iPhone", (uint8_t[]) {0x82, 0xae, 0x30, 0x88, 0xd2, 0xc3}),
        new Device("Simon iPad", (uint8_t[]) {0x72, 0xF7, 0x0A, 0x4F, 0x5C, 0x37}),
        new Device("Simon Handy", (uint8_t[]) {0x36, 0x56, 0xdb, 0x3b, 0xc2, 0x14})
};

os_timer_t device_status_report_timer;
int interrupt_counter = 0;

void setup() {
    delay(500);
    pinMode(D5, OUTPUT); // sets the pins to output mode
    pinMode(D6, OUTPUT);
    pinMode(D7, OUTPUT);
    Serial.begin(74880);
    esppl_init(callback);
    esppl_sniffing_start();
    Serial.printf("Startup complete\n");
}

bool maccmp(const uint8_t *mac1, const uint8_t *mac2) {
    for (int i = 0; i < ESPPL_MAC_LEN; i++) {
        if (mac1[i] != mac2[i]) {
            return false;
        }
    }
    return true;
}

void callback(esppl_frame_info *info) {
    // check if any known mac is in the packet, either as sender (more likely) or recipient
    for(Device *device: devices) {
        if (maccmp(info->sourceaddr, device->getMacAddress()) || maccmp(info->receiveraddr, device->getMacAddress())) {
            if (SEND_DEVICE_NOTIFICATIONS) Serial.printf("Detected device \"%s\"\n", device->getName().c_str());
            device->setLastSeen(millis());
            return;
        }
    }
}

void setup_interrupts() {
    os_timer_setfn(&device_status_report_timer, print_device_status, &interrupt_counter);
    os_timer_arm(&device_status_report_timer, DEVICE_STATUS_REPORT_INTERVAL_MILLIS, true);
}

bool is_device_active(Device *device){
    if(device->getLastSeen() == 0) return false; // last_seen is still 0, so it has never been seen
    return millis() - device->getLastSeen() < DEVICE_TIMEOUT_MILLIS;
}

void print_device_status(void *pArg) {

    Serial.print("\n");

    Serial.printf("Current time: %lu\tTimeout: %i\n", millis(), DEVICE_TIMEOUT_MILLIS);
    for(Device *device: devices) {
        Serial.printf("Device name: \"%s\"\tStatus: %s\tLast seen: %lu\n", device->getName().c_str(), is_device_active(device) ? "active  " : "inactive", device->getLastSeen());
    }

    Serial.print("\n");
}

void loop() {
    for (int i = ESPPL_CHANNEL_MIN; i <= ESPPL_CHANNEL_MAX; i++) {
        esppl_set_channel(i);
        while (esppl_process_frames()) {
            // do nothing
        }
    }
}
