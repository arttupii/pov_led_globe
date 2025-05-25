#define ARDUINO_RUNNING_CORE 0
#define ARDUINO_EVENT_RUNNING_CORE 0

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "log.h"
#include "pov.h"
#include "AsyncUDP.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
AsyncUDP audp;

const char* ssid = "Maaaaaa";
const char* password = "aaaaaaKoo";

void otaTask(void*);
void tasks(void*);

bool rotate = true;
bool demo = true;

void setup() {

    Serial.begin(115200);

    setCpuFrequencyMhz(240);      // 80, 160 tai 240 (jos tukee)

 
    WiFi.begin(ssid, password);
    esp_wifi_set_protocol(WIFI_IF_STA,
                      WIFI_PROTOCOL_11B |
                      WIFI_PROTOCOL_11G |
                      WIFI_PROTOCOL_11N);   // 802.11n 72 Mbit

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    WiFi.setSleep(false);

      // OTA-määritykset
     // Tärkeä: Aseta hostnamen WiFi:lle, ei ArduinoOTA:lle
  WiFi.setHostname("pov");
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  ArduinoOTA
  .onStart([]() { Serial.println("OTA Start"); 
    paintBall(0,0,0,0);
    swapPov();
    rotate = false;
    demo = false;
  })
  .onEnd([]() { Serial.println("OTA End"); })
  .onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]\n", error);

  });

  ArduinoOTA.begin();
     
  initPov();

  xTaskCreatePinnedToCore(
        otaTask,            // funktio
        "ota",          // selkeä nimi debuggausta varten
        4096,              // stack-koko sanoina (≈ 16 kB)
        nullptr,           // parametrit
        1,                 // prioriteetti (0-24, 1 on “low but not idle”)
        nullptr,           // jos tarvitset kahvan, talleta tähän
        0                 // ydin: 0 = PRO_CPU, 1 = APP_CPU
    );
   
   xTaskCreatePinnedToCore(
        tasks,            // funktio
        "tasks",          // selkeä nimi debuggausta varten
        4096,              // stack-koko sanoina (≈ 16 kB)
        nullptr,           // parametrit
        10,                 // prioriteetti (0-24, 1 on “low but not idle”)
        nullptr,           // jos tarvitset kahvan, talleta tähän
        0                 // ydin: 0 = PRO_CPU, 1 = APP_CPU
    );

  draftEarth();
  swapPov();
  dbg("Started POV");

  audp.listen(22222);

  audp.onPacket([](AsyncUDPPacket udp_data) {
      int len = udp_data.length();

      const uint8_t* packet = udp_data.data();   

      rotate = false;
      demo = false;
      //dbg("Received Udb len ", len);
      //dbg("Received Udb lport ", udp_data.localPort());
      //dbg("Received Udb rport ", udp_data.remotePort());
      
      
      if (packet[0] == 0xFF) {
   

        if(len>1) {
          if(packet[1]==2) //tallenna
          {
            delay(1000);
            storeCurrentDisplayToDisk(packet[2]);
            demo = false;
            rotate = false;
          }
          if(packet[1]==3) //poista
          {
            deletePictureFromDisk(packet[2]);
            demo = true;
            rotate = true;
          }

        } else {
           swapPov();
        }
        //dbg("SWAP");
      } else {
        int x = 0;
        int y = packet[0];
       
        for (int i = 1; i < len; i += 4) {
          draftPixel(x, y, packet[i], packet[i+1], packet[i+2], packet[i+3]);
          
          x++;
          if(x>=LINES_OF_POV) {
            x=0;
            y++;
            i++;
              esp_task_wdt_reset();
              yield();                       // pitää myös Wi-Fi:n hengissä
          }
        }
      }
      udp_data.print("ACK");
  });

}

void otaTask(void*) {
  while(1){

        ArduinoOTA.handle();
      delay(1000); 
  }
}

void show_task(){
  static u_int64_t tm=millis()+5000;
  static int index = 0;

  if(demo==false) return;

  while(millis()>=tm){
    if(loadPictureFromDisk(index)) {
      swapPov();
      rotate = true;
      index++;
    } else {
      index = 0;
      if(index!=0) continue;
    }
    
    tm=millis()+10000;
  }
}
void rotate_task(){
  static u_int64_t tm=millis()+50;

  if(rotate==false) return;

  if(millis()>=tm){
    povRotate(1);
    swapPov();
    tm=millis()+50;
  }
}
void dbg_task(){
  static u_int64_t tm=millis()+50;

  if(millis()>=tm){
    tm=millis()+1000;
    
    dbg("Rotate ", rotate);
    dbg("Demo ", demo);
    dbg("pov_line_tm ", pov_line_tm);
    dbg("pov_rotation_tm ", pov_rotation_tm);

     // dbg("max_x ", pov_rotation_tm/pov_line_tm);
     // dbg("rotation_r/min", (1000000/pov_rotation_tm)*60);
    //  dbg("rotation_r/s", (1000000/pov_rotation_tm));
    
    dbg("Free heap ", ESP.getFreeHeap());
    dbg("highWater", uxTaskGetStackHighWaterMark(NULL)); 
/*
    dbg("SPIFFS.totalBytes() ", SPIFFS.totalBytes()); 
    dbg("SPIFFS.usedBytes() ", SPIFFS.usedBytes()); 
    dbg("SPIFFS free ", SPIFFS.totalBytes() - SPIFFS.usedBytes()); 

  */  
  }
}

void tasks(void*){
  while(1) {
    delay(5);
    show_task();
    rotate_task();
    dbg_task();
  }
}

void loop(void) {
   delay(1000);
}
