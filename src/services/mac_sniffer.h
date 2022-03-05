/**
    MIT License

    Copyright (c) 2017 Ricardo Oliveira

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 */

#ifndef MAC_SNIFFER_H
#define MAC_SNIFFER_H

#include <ESP8266WiFi.h>
#include "../structs/esppl_struct.h++"

extern "C" {
#include "user_interface.h"
[[maybe_unused]] typedef void (*freedom_outside_cb_t)(uint8 status);
}

uint8_t esppl_channel = ESPPL_CHANNEL_DEFAULT;
uint8_t esppl_default_mac[ESPPL_MAC_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
ESPPL_CB_T user_cb;
int frame_waitlist = 0;
bool esppl_sniffing_enabled = false;

void esppl_buf_to_info(uint8_t *frame, signed rssi, uint16_t len);

/*
 * Promiscuous RX process_packet function
 */
void esppl_rx_cb(uint8_t *buf, uint16_t len) {
    if (len == sizeof(struct sniffer_buf2)) {
        auto *sniffer = (struct sniffer_buf2 *) buf;
        esppl_buf_to_info(sniffer->buf, sniffer->rx_ctrl.rssi, len);
    } else if (len == sizeof(struct RxControl)) {
        // Currently unused
        // auto *sniffer = (struct RxControl *) buf;
    } else {
        auto *sniffer = (struct sniffer_buf *) buf;
        esppl_buf_to_info(sniffer->buf, sniffer->rx_ctrl.rssi, len);
    }
}

/* 
 * buf to esppl_frame_info structure converter 
 */
void esppl_buf_to_info(uint8_t *frame, signed rssi, uint16_t len) {
    struct esppl_frame_info info{};
    uint8_t dstofrom;
    uint8_t pos;
    uint8_t parsecount;

    if (esppl_sniffing_enabled) {
        frame_waitlist++;
        // - "Resets" some fields
        memcpy(info.receiveraddr, esppl_default_mac, ESPPL_MAC_LEN);
        memcpy(info.destinationaddr, esppl_default_mac, ESPPL_MAC_LEN);
        memcpy(info.transmitteraddr, esppl_default_mac, ESPPL_MAC_LEN);
        memcpy(info.sourceaddr, esppl_default_mac, ESPPL_MAC_LEN);
        memcpy(info.bssid, esppl_default_mac, ESPPL_MAC_LEN);
        info.ssid_length = 0;
        info.isvalid = true;
        info.channel = 0;
        info.seq_num = 0;
        // - Populates the fields
        memcpy(info.raw, frame, len - 1);
        info.raw_length = len;

        info.frametype = (frame[0] & B00001100) >> 2;
        info.framesubtype = (frame[0] & B11110000) >> 4;
        info.rssi = rssi;

        dstofrom = (frame[1] & 3);

        switch (info.frametype) {
            case ESPPL_CONTROL: // - Control Frames
                switch (info.framesubtype) {
                    case ESPPL_CONTROL_RTS: // - RTS
                        memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN); //?
                        memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN); //?
                        break;
                    case ESPPL_CONTROL_CTS: // - CTS
                    case ESPPL_CONTROL_ACK: // - ACK
                        memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN); //?
                        break;
                    case ESPPL_CONTROL_PS_POLL: // - PS-POLL
                        memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN); //?
                        memcpy(info.bssid, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN); //?
                        break;
                    case ESPPL_CONTROL_CF_END: // - CF-END
                    case ESPPL_CONTROL_CF_END_CF_ACK: // - CF-END+CF-ACK
                        memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.bssid, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        break;
                    case ESPPL_CONTROL_BLOCK_ACK_REQUEST: // - BlockAckReq
                    case ESPPL_CONTROL_BLOCK_ACK: // - BlockAck
                        memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        break;
                    case ESPPL_CONTROL_CONTROL_WRAPPER: // - Control Wrapper
                        //TODO
                        break;
                }
                break;
            case ESPPL_DATA: // - Data Frames
                info.seq_num = frame[23] * 0xFF + (frame[22] & 0xF0);
                switch (dstofrom) { // - ToDS FromDS
                    case ESPPL_DS_NO: // - ToDS=0 FromDS=0
                        memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.bssid, frame + 4 + ESPPL_MAC_LEN * 2, ESPPL_MAC_LEN);
                        break;
                    case ESPPL_DS_TO: // - ToDS=1 FromDS=0
                        memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.bssid, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.destinationaddr, frame + 4 + ESPPL_MAC_LEN * 2, ESPPL_MAC_LEN); //MSDU
                        break;
                    case ESPPL_DS_FROM: // - ToDS=0 FromDS=1
                        memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.bssid, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN * 2, ESPPL_MAC_LEN);
                        break;
                    case ESPPL_DS_TOFROM: // - ToDS=1 FromDS=1
                        memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
                        memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                        memcpy(info.destinationaddr, frame + 4 + ESPPL_MAC_LEN * 2, ESPPL_MAC_LEN);
                        memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN * 2 + 2, ESPPL_MAC_LEN);
                        break;
                    default:
                        break;
                }
                break;
            case ESPPL_MANAGEMENT: // - Management Frames
                switch (info.framesubtype) {
                    case ESPPL_MANAGEMENT_PROBE_RESPONSE:
                    case ESPPL_MANAGEMENT_BEACON:
                        // - Parses management frame body
                        info.seq_num = frame[23] * 0xFF + (frame[22] & 0xF0);
                        pos = ESPPL_MANAGEMENT_MAC_HEADER_SIZE;
                        parsecount = 0;
                        while (pos < len && parsecount < 4) {
                            switch (frame[pos]) {
                                case 0: // - SSID
                                    info.ssid_length = frame[pos + 1];
                                    if (info.ssid_length > 32 || info.ssid_length < 0) {
                                        info.ssid_length = 0;
                                    }
                                    memcpy(info.ssid, frame + pos + 2, info.ssid_length);
                                    break;
                                case 3: // - Channel
                                    info.channel = (int) frame[pos + 2];
                                    break;
                                default:
                                    break;
                            }
                            parsecount++; // - Avoid bad parsing or infinite loop
                            pos += frame[pos + 1] + 2;
                        }
                    default:
                        break;
                }
                memcpy(info.receiveraddr, frame + 4, ESPPL_MAC_LEN);
                memcpy(info.destinationaddr, frame + 4, ESPPL_MAC_LEN);
                memcpy(info.transmitteraddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                memcpy(info.sourceaddr, frame + 4 + ESPPL_MAC_LEN, ESPPL_MAC_LEN);
                memcpy(info.bssid, frame + 4 + ESPPL_MAC_LEN * 2, ESPPL_MAC_LEN);
                break;
            default:
                info.isvalid = false; //TODO Proper checksum validation
        }

        // - User process_packet function
        if (info.isvalid) {
            user_cb(&info);
        }
        frame_waitlist--;
    }
}

/* 
 * Change channel 
 */
void esppl_set_channel(int channel) {
    wifi_set_channel(channel);
    esppl_channel = channel;
}

/* 
 * Allows some time to process the frames 
 */
bool esppl_process_frames() {
    delay(10);
    return frame_waitlist != 0;
}

/*
 * Initialization function
 */
void esppl_init(ESPPL_CB_T cb) {
    user_cb = cb;
    frame_waitlist = 0;
    wifi_station_disconnect();
    wifi_set_opmode(STATION_MODE);
    wifi_set_channel(esppl_channel);
    wifi_promiscuous_enable(false);
    wifi_set_promiscuous_rx_cb(esppl_rx_cb);
    wifi_promiscuous_enable(true);
    esppl_sniffing_enabled = false;
}

/*
 * Start sniffing
 */
void esppl_sniffing_start() {
    esppl_sniffing_enabled = true;
}

/*
 * Stop sniffing
 */
void esppl_sniffing_stop() {
    esppl_sniffing_enabled = false;
}

#endif