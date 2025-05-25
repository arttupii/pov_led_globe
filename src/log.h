
#include <WiFi.h>
#ifndef _DBG_H_
#define _DBG_H_

WiFiUDP udp;

void dbg(const char *t) {
    udp.beginPacket("192.168.100.187", 12345);
    udp.println(t);
    udp.endPacket();
}
void dbg(const int i) {
    udp.beginPacket("192.168.100.187", 12345);
    udp.println(i);
     udp.endPacket();
}

void dbg(const char *t, const int val) {
    udp.beginPacket("192.168.100.187", 12345);
    udp.print(t);
    udp.println(val);
    udp.endPacket();
}
#endif