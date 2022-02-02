#ifndef INTERRUPT_SERVICE_H
#define INTERRUPT_SERVICE_H

#include <ESP8266WiFi.h>

class InterruptService {
private:
    InterruptService();

    os_timer_t device_status_update_timer, heartbeat_effect_timer;

    #ifdef DEBUG
    os_timer_t device_report_timer;
    #endif

public:
    static InterruptService& instance() {
        static InterruptService INSTANCE;
        return INSTANCE;
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
};


#endif
