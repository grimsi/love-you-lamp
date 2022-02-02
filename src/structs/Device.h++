#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include "RGBColor.h++"
#include "MACAddress.h++"

class Device {
public:
    Device(const char *name, MACAddress *mac_address, RGBColor *led_color){
        this->name = name;
        this->mac_address = mac_address;
        this->led_color = led_color;
    }

    [[nodiscard]]
    const String &getName() const {
        return name;
    }

    [[nodiscard]]
    ulong getLastSeen() const {
        return last_seen;
    }

    [[nodiscard]]
    MACAddress *getMacAddress() const {
        return mac_address;
    }

    [[nodiscard]]
    RGBColor *getLedColor() const {
        return led_color;
    }

    void setLastSeen(ulong lastSeen) {
        last_seen = lastSeen;
    }

private:
    String name;
    MACAddress *mac_address;
    RGBColor *led_color;
    ulong last_seen{0};
};


#endif
