#ifndef DEVICE_SERVICE_H
#define DEVICE_SERVICE_H

#include <Arduino.h>
#include "../structs/Device.h++"

class DeviceService {
private:
    constexpr static int registered_device_count = sizeof(devices) / sizeof(devices[0]);

public:
    static bool is_device_active(Device *device){
        if(device->getLastSeen() == 0) return false; // last_seen is still 0, so it has never been seen
        return millis() - device->getLastSeen() < DEVICE_TIMEOUT_MILLIS;
    }

    static int get_active_device_count() {
        int count = 0;

        for(Device *device : devices) {
            if(is_device_active(device)) count++;
        }

        return count;
    }

    static bool are_all_registered_devices_active() {
        return get_active_device_count() >= registered_device_count;
    }
};

#endif
