#ifndef DEVICE_H
#define DEVICE_H


class Device {
public:
    Device(const char *name, const uint8_t *mac_address){
        this->name = name;
        this->mac_address = mac_address;
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
    const uint8_t *getMacAddress() const {
        return mac_address;
    }

    void setLastSeen(ulong lastSeen) {
        last_seen = lastSeen;
    }

private:
    String name;
    const uint8_t *mac_address{};
    ulong last_seen {0};
};


#endif
