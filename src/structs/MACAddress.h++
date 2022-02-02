#ifndef MACADDRESS_H
#define MACADDRESS_H

class MACAddress {
public:
    MACAddress(byte octet_0, byte octet_1, byte octet_2, byte octet_3, byte octet_4, byte octet_5)
    {
        this->_octets[0] = octet_0;
        this->_octets[1] = octet_1;
        this->_octets[2] = octet_2;
        this->_octets[3] = octet_3;
        this->_octets[4] = octet_4;
        this->_octets[5] = octet_5;
    }

    explicit MACAddress(const byte octets[])
    {
        std::memcpy(_octets, octets, 6);
    }

    byte operator[](int index) {
        return(_octets[index]);
    }

    bool operator==(const MACAddress& other) const
    {
        return(std::memcmp(_octets, other._octets, 6) == 0);
    }


private:
    byte _octets[6]{};
};



#endif
