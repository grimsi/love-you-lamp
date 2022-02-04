#include <Arduino.h>
#include "./esppl_functions.h"
#include "Device.h++"

#define COMMON_ANODE 0
#define COMMON_CATHODE 1
// --- CONFIG STARTS HERE ---

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

// Configure the type of your LED
// Common anode means that your LED has a shared 5V (or 3.3V) and the other three pins connect to the ground
// Common anode means that your LED has a shared ground and each pin gets supplied with 5V (or 3.3V)
#define LED_TYPE COMMON_ANODE

Device* devices[] = {
        //new Device("Anna-Lena iPhone", new MACAddress(0x82, 0xae, 0x30, 0x88, 0xd2, 0xc3), new RGBColor(0, 0, 255)),
        new Device("Simon iPad", new MACAddress(0x72, 0xF7, 0x0A, 0x4F, 0x5C, 0x37), new RGBColor(0, 0, 255)),
        new Device("Simon Handy", new MACAddress(0x36, 0x56, 0xdb, 0x3b, 0xc2, 0x14), new RGBColor(0, 200, 0))
};

// --- CODE STARTS HERE ---

void process_packet(esppl_frame_info *info);
bool is_device_active(Device *device);
int get_active_device_count();
void write_status_led(void *pArg);
void setup_interrupts();
void arm_sniffer_interrupts();
void disarm_sniffer_interrupts();
void enable_heartbeat_effect();
void disable_heartbeat_effect();
RGBColor calculate_rgb_value();
void rgb(byte red_intensity, byte green_intensity, byte blue_intensity);
void heartbeat(void *pArg);
void rgb_heartbeat(int red, int green, int blue);
#ifdef DEBUG
void report_device_status(void *pArg);
#endif

byte heartbeat_graph_values[]={
        0,  2,  4,  8, 12, 20, 32, 44, 56, 68,
        80, 92,104,116,128,140,152,164,176,188,
        200,212,224,236,255,240,210,180,150,120,
        90, 60, 40, 25, 25, 40, 60, 90,120,150,
        180,210,240,255,230,205,180,155,130,105,
        80, 55, 30, 20, 16,  8,  4,  2,  0
};

os_timer_t device_status_update_timer, heartbeat_effect_timer;

#ifdef DEBUG
os_timer_t device_report_timer;
#endif

int registered_device_count = sizeof(devices) / sizeof(devices[0]);
bool is_heartbeat_timer_active = false;

void process_packet(esppl_frame_info *info) {
    auto sourceaddr = MACAddress(info->sourceaddr);
    auto receiveraddr = MACAddress(info->receiveraddr);

    // check if any known mac is in the packet, either as sender (more likely) or recipient
    for(Device *device: devices) {
        if (sourceaddr == *device->getMacAddress() || receiveraddr == *device->getMacAddress()) {

            #ifdef DEBUG
            Serial.printf("Detected device \"%s\"\n", device->getName().c_str());
            #endif

            device->setLastSeen(millis());
            return;
        }
    }
}

void setup_interrupts() {
    os_timer_setfn(&device_status_update_timer, write_status_led, nullptr);
    os_timer_setfn(&heartbeat_effect_timer, heartbeat, nullptr);

    #ifdef DEBUG
    os_timer_setfn(&device_report_timer, report_device_status, nullptr);
    #endif

    arm_sniffer_interrupts();
}

void arm_sniffer_interrupts() {
    os_timer_arm(&device_status_update_timer, DEVICE_STATUS_UPDATE_INTERVAL_MILLIS, true);

    #ifdef DEBUG
    os_timer_arm(&device_report_timer, DEVICE_STATUS_REPORT_INTERVAL_MILLIS, true);
    #endif
}

void disarm_sniffer_interrupts() {
    os_timer_disarm(&device_status_update_timer);

    #ifdef DEBUG
    os_timer_disarm(&device_report_timer);
    #endif
}

void enable_heartbeat_effect() {
    if(is_heartbeat_timer_active) return;

    heartbeat(nullptr);
    os_timer_arm(&heartbeat_effect_timer, HEARTBEAT_EFFECT_INTERVAL_MILLIS, true);
    is_heartbeat_timer_active = true;
}

void disable_heartbeat_effect() {
    if(!is_heartbeat_timer_active) return;

    os_timer_disarm(&heartbeat_effect_timer);
    is_heartbeat_timer_active = false;
}

bool is_device_active(Device *device){
    if(device->getLastSeen() == 0) return false; // last_seen is still 0, so it has never been seen
    return millis() - device->getLastSeen() < DEVICE_TIMEOUT_MILLIS;
}

int get_active_device_count() {
    int count = 0;

    for(Device *device : devices) {
        if(is_device_active(device)) count++;
    }

    return count;
}

void write_status_led(void *pArg) {
    if(get_active_device_count() == registered_device_count) {
        enable_heartbeat_effect();
    } else {
        disable_heartbeat_effect();
    }

    RGBColor color = calculate_rgb_value();
    rgb(color.red(), color.green(), color.blue());
}

#ifdef DEBUG
void report_device_status(void *pArg) {
    Serial.print("\n");
    Serial.printf("Current time: %lu\tTimeout: %i\n", millis(), DEVICE_TIMEOUT_MILLIS);
    for(Device *device : devices) {
        Serial.printf("Device name: \"%s\"\tStatus: %s\tLast seen: %lu\n", device->getName().c_str(), is_device_active(device) ? "active  " : "inactive", device->getLastSeen());
    }
    Serial.print("\n");
}
#endif

RGBColor calculate_rgb_value() {
    byte red = 0, green = 0,  blue = 0;

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
    if(LED_TYPE == COMMON_ANODE) {
        analogWrite(LED_RED_PIN, 255 - red_intensity);
        analogWrite(LED_GREEN_PIN, 255 - green_intensity);
        analogWrite(LED_BLUE_PIN, 255 - blue_intensity);
    } else {
        analogWrite(LED_RED_PIN, red_intensity);
        analogWrite(LED_GREEN_PIN, green_intensity);
        analogWrite(LED_BLUE_PIN, blue_intensity);
    }

}

void heartbeat(void *pArg) {
    disarm_sniffer_interrupts();

    RGBColor color = calculate_rgb_value();
    rgb_heartbeat(color.red(), color.green(), color.blue());

    rgb(color.red(), color.green(), color.blue());
    arm_sniffer_interrupts();
}

void rgb_heartbeat(int red, int green, int blue) {
    float red_intensity, green_intensity, blue_intensity;

    rgb(0,0,0);

    for(byte value: heartbeat_graph_values) {
        red_intensity = round(static_cast<float>(red) * (static_cast<float>(value) / 255));
        green_intensity = round(static_cast<float>(green) * (static_cast<float>(value) / 255));
        blue_intensity = round(static_cast<float>(blue) * (static_cast<float>(value) / 255));

        rgb(static_cast<byte>(red_intensity), static_cast<byte>(green_intensity), static_cast<byte>(blue_intensity));

        delayMicroseconds(25 * 1000); // use delayMicroseconds() since delay() will crash the ESP8266 (because it uses os_timer internally)
    }

    rgb(0,0,0);
}

void setup() {
    delay(500); // necessary, wait for WiFi module to start

    Serial.begin(74880);

    pinMode(LED_BLUE_PIN, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);

    rgb_heartbeat(255, 0, 0);
    rgb_heartbeat(0, 255, 0);
    rgb_heartbeat(0, 0, 255);
    rgb(0,0,0);

    esppl_init(process_packet);
    setup_interrupts();
    esppl_sniffing_start();

    Serial.printf("Startup complete\n");
}

void loop() {
    // scan all WiFi channels for packets
    for (int i = ESPPL_CHANNEL_MIN; i <= ESPPL_CHANNEL_MAX; i++) {
        esppl_set_channel(i);
        while (esppl_process_frames()) {
            // do nothing
        }
    }
}
