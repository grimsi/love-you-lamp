#include <Arduino.h>
#include "structs/Device.h++"
#include "services/mac_sniffer.h"
#include "services/led_service.h"
#include "services/device_service.h"
#include "services/report_service.h"

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
