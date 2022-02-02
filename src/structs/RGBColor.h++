#ifndef RGBCOLOR_H
#define RGBCOLOR_H

#include <Arduino.h>

class RGBColor {
public:
    RGBColor(byte red, byte green, byte blue) {
        this->_rgb_colors[0] = red;
        this->_rgb_colors[1] = green;
        this->_rgb_colors[2] = blue;
    }

    byte red() {
        return this->_rgb_colors[0];
    }

    byte green() {
        return this->_rgb_colors[1];
    }

    byte blue() {
        return this->_rgb_colors[2];
    }

private:
    byte _rgb_colors[3]{};
};


#endif
