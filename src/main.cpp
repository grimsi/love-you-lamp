#include <Arduino.h>
#include "./esppl_functions.h"
#include "Device.h"

// After how many ms of not receiving a message from a device it will be marked as "unavailable"
// Default value is 15 Minutes (900000 ms)
#define DEVICE_TIMEOUT_MILLIS 900000

// How long to wait between two device status updates in ms
// Default value is 1 second (1000 ms)
#define DEVICE_STATUS_UPDATE_INTERVAL_MILLIS 1000

// How long to wait between two device status reports in ms
// Default value is 1 second (1000 ms)
#define DEVICE_STATUS_REPORT_INTERVAL_MILLIS 10000

// Pins where the LED is connected to
#define LED_RED_PIN D1
#define LED_GREEN_PIN D2
#define LED_BLUE_PIN D3


void callback(esppl_frame_info *info);
bool is_device_active(Device *device);
void update_device_status(void *pArg);
void report_device_status(void *pArg);
void setup_interrupts();
void red(int intensity);
void green(int intensity);
void blue(int intensity);
void rgb(int red, int green, int blue);

Device* devices[] = {
        new Device("Anna-Lena iPhone", (uint8_t[]) {0x82, 0xae, 0x30, 0x88, 0xd2, 0xc3}),
        new Device("Simon iPad", (uint8_t[]) {0x72, 0xF7, 0x0A, 0x4F, 0x5C, 0x37}),
        new Device("Simon Handy", (uint8_t[]) {0x36, 0x56, 0xdb, 0x3b, 0xc2, 0x14})
};

int led_pins[3] = { LED_RED_PIN, LED_GREEN_PIN, LED_BLUE_PIN };

os_timer_t device_status_update_timer, device_report_timer;

void setup() {
    delay(500); // necessary, wait for WiFi module to start
    Serial.begin(74880);

    pinMode(LED_BLUE_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);

    digitalWrite(LED_RED_PIN, 0);
    digitalWrite(LED_GREEN_PIN, 0);
    digitalWrite(LED_BLUE_PIN, 0);

    esppl_init(callback);
    setup_interrupts();
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
            Serial.printf("Detected device \"%s\"\n", device->getName().c_str());
            device->setLastSeen(millis());
            return;
        }
    }
}

void setup_interrupts() {
    os_timer_setfn(&device_status_update_timer, update_device_status, nullptr);
    os_timer_setfn(&device_report_timer, report_device_status, nullptr);

    os_timer_arm(&device_status_update_timer, DEVICE_STATUS_UPDATE_INTERVAL_MILLIS, true);
    os_timer_arm(&device_report_timer, DEVICE_STATUS_REPORT_INTERVAL_MILLIS, true);
}

bool is_device_active(Device *device){
    if(device->getLastSeen() == 0) return false; // last_seen is still 0, so it has never been seen
    return millis() - device->getLastSeen() < DEVICE_TIMEOUT_MILLIS;
}

void update_device_status(void *pArg) {
    for(int i = 0; i < 3; i++) {
        if(is_device_active(devices[i])) {
            digitalWrite(led_pins[i], 1);
        } else {
            digitalWrite(led_pins[i], 0);
        }
    }
}

void report_device_status(void *pArg) {
    Serial.print("\n");
    Serial.printf("Current time: %lu\tTimeout: %i\n", millis(), DEVICE_TIMEOUT_MILLIS);
    for(Device *device : devices) {
        Serial.printf("Device name: \"%s\"\tStatus: %s\tLast seen: %lu\n", device->getName().c_str(), is_device_active(device) ? "active  " : "inactive", device->getLastSeen());
    }
    Serial.print("\n");
}

void red(int intensity) {
    analogWrite(LED_RED_PIN, intensity);
}

void green(int intensity) {
    analogWrite(LED_GREEN_PIN, intensity);
}

void blue(int intensity) {
    analogWrite(LED_BLUE_PIN, intensity);
}

void rgb(int red_intensity, int green_intensity, int blue_intensity) {
    red(red_intensity);
    green(green_intensity);
    blue(blue_intensity);
}

void loop() {
    for (int i = ESPPL_CHANNEL_MIN; i <= ESPPL_CHANNEL_MAX; i++) {
        esppl_set_channel(i);
        while (esppl_process_frames()) {
            // do nothing
        }
    }
}
