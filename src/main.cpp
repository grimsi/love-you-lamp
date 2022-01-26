#include <Arduino.h>
#include "./esppl_functions.h"
#include "Device.h++"

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


void process_packet(esppl_frame_info *info);
bool is_device_active(Device *device);
void update_device_status(void *pArg);
void report_device_status(void *pArg);
void setup_interrupts();
RGBColor calculate_rgb_value();
void rgb(byte red_intensity, byte green_intensity, byte blue_intensity);

Device* devices[] = {
        new Device("Anna-Lena iPhone", new MACAddress(0x82, 0xae, 0x30, 0x88, 0xd2, 0xc3), new RGBColor(0, 0, 122)),
        new Device("Simon iPad", new MACAddress(0x72, 0xF7, 0x0A, 0x4F, 0x5C, 0x37), new RGBColor(122, 0, 0)),
        new Device("Simon Handy", new MACAddress(0x36, 0x56, 0xdb, 0x3b, 0xc2, 0x14), new RGBColor(0, 122, 0))
};

os_timer_t device_status_update_timer, device_report_timer;

void process_packet(esppl_frame_info *info) {
    auto sourceaddr = MACAddress(info->sourceaddr);
    auto receiveraddr = MACAddress(info->receiveraddr);

    // check if any known mac is in the packet, either as sender (more likely) or recipient
    for(Device *device: devices) {
        if (sourceaddr == *device->getMacAddress() || receiveraddr == *device->getMacAddress()) {
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
    RGBColor color = calculate_rgb_value();
    rgb(color.red(), color.green(), color.blue());
}

void report_device_status(void *pArg) {
    Serial.print("\n");
    Serial.printf("Current time: %lu\tTimeout: %i\n", millis(), DEVICE_TIMEOUT_MILLIS);
    for(Device *device : devices) {
        Serial.printf("Device name: \"%s\"\tStatus: %s\tLast seen: %lu\n", device->getName().c_str(), is_device_active(device) ? "active  " : "inactive", device->getLastSeen());
    }
    Serial.print("\n");
}

RGBColor calculate_rgb_value() {
    byte red, green, blue = 0;

    for(Device *device : devices) {
        if(is_device_active(device)) {
            red += device->getLedColor()->red();
            green += device->getLedColor()->green();
            blue += device->getLedColor()->blue();
        }
    }

    return {red, green, blue};
}

void rgb(byte red_intensity, byte green_intensity, byte blue_intensity) {
    analogWrite(LED_RED_PIN, red_intensity);
    analogWrite(LED_GREEN_PIN, green_intensity);
    analogWrite(LED_BLUE_PIN, blue_intensity);
}

void setup() {
    delay(500); // necessary, wait for WiFi module to start
    Serial.begin(74880);

    pinMode(LED_BLUE_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);

    digitalWrite(LED_RED_PIN, 0);
    digitalWrite(LED_GREEN_PIN, 0);
    digitalWrite(LED_BLUE_PIN, 0);

    esppl_init(process_packet);
    setup_interrupts();
    esppl_sniffing_start();
    Serial.printf("Startup complete\n");
}

void loop() {
    for (int i = ESPPL_CHANNEL_MIN; i <= ESPPL_CHANNEL_MAX; i++) {
        esppl_set_channel(i);
        while (esppl_process_frames()) {
            // do nothing
        }
    }
}
