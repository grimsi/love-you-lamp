#ifndef LED_SERVICE_H
#define LED_SERVICE_H

#include "structs/RGBColor.h++"
#include "config.cpp"
#include "device_service.h"

class LedService {
private:
    LedService();

    constexpr static byte heartbeat_graph_values[] = {
            0,  2,  4,  8, 12, 20, 32, 44, 56, 68,
            80, 92,104,116,128,140,152,164,176,188,
            200,212,224,236,255,240,210,180,150,120,
            90, 60, 40, 25, 25, 40, 60, 90,120,150,
            180,210,240,255,230,205,180,155,130,105,
            80, 55, 30, 20, 16,  8,  4,  2,  0
    };

    bool is_heartbeat_timer_active = false;

public :
    static LedService& instance() {
        static LedService INSTANCE;
        return INSTANCE;
    }

    static void write_status_led(void *pArg) {
        if(DeviceService::are_all_registered_devices_active()) {
            enable_heartbeat_effect();
        } else {
            disable_heartbeat_effect();
        }

        RGBColor color = calculate_rgb_value();
        rgb(color.red(), color.green(), color.blue());
    }

    static RGBColor calculate_rgb_value() {
        byte red = 0, green = 0, blue = 0;

        for (Device *device: devices) {
            if (DeviceService::is_device_active(device)) {
                red += device->getLedColor()->red();
                green += device->getLedColor()->green();
                blue += device->getLedColor()->blue();
            }
        }

        return {red, green, blue};
    }

    static void rgb(byte red_intensity, byte green_intensity, byte blue_intensity) {
        analogWrite(LED_RED_PIN, red_intensity);
        analogWrite(LED_GREEN_PIN, green_intensity);
        analogWrite(LED_BLUE_PIN, blue_intensity);
    }

    static void rgb_heartbeat(int red, int green, int blue) {
        float red_intensity, green_intensity, blue_intensity;

        rgb(0, 0, 0);

        for (byte value: heartbeat_graph_values) {
            red_intensity = round(static_cast<float>(red) * (static_cast<float>(value) / 255));
            green_intensity = round(static_cast<float>(green) * (static_cast<float>(value) / 255));
            blue_intensity = round(static_cast<float>(blue) * (static_cast<float>(value) / 255));

            rgb(static_cast<byte>(red_intensity), static_cast<byte>(green_intensity),
                static_cast<byte>(blue_intensity));

            delayMicroseconds(25 *
                              1000); // use delayMicroseconds() since delay() will crash the ESP8266 (because it uses os_timer internally)
        }

        rgb(0, 0, 0);
    }

    void heartbeat(void *pArg) {
        interruptService.disarm_sniffer_interrupts();

        RGBColor color = calculate_rgb_value();
        rgb_heartbeat(color.red(), color.green(), color.blue());
        rgb(color.red(), color.green(), color.blue());

        interruptService.arm_sniffer_interrupts();
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

};

#endif
