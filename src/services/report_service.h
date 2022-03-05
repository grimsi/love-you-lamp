#ifndef REPORT_SERVICE_H
#define REPORT_SERVICE_H


#ifdef DEBUG
class ReportService {
public:
    static void report_device_status(void *pArg) {
        Serial.print("\n");
        Serial.printf("Current time: %lu\tTimeout: %i\n", millis(), DEVICE_TIMEOUT_MILLIS);
        for(Device *device : devices) {
            Serial.printf("Device name: \"%s\"\tStatus: %s\tLast seen: %lu\n", device->getName().c_str(), DeviceService::is_device_active(device) ? "active  " : "inactive", device->getLastSeen());
        }
        Serial.print("\n");
    }
};
#endif


#endif
