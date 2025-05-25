#ifndef __POV_H_
#define __POV_H_
#define FASTLED_ALLOW_INTERRUPTS 0
//#define FASTLED_ALL_PINS_HARDWARE_SPI


#include <LittleFS.h>
#define FILESYSTEM LittleFS

#include <Arduino.h>
//#include <FastLED.h>
#include <esp_task_wdt.h>
#include "apa102.h"

#define NUM_LEDS_STRIP 60

#define DATA_PIN2 23
#define CLOCK_PIN2 18
#define DATA_PIN1 13
#define CLOCK_PIN1 14

#define HALL_PIN 4



#define LINES_OF_POV (180)

void povTask(void *pvParameters);
void IRAM_ATTR  hallInterruptHandler(void *c);

bool otaOngoing = false;

void notifyPovOtaOngoing() {
    otaOngoing = true;
}

#define DOUBLE_BUFFER

/*
struct PovPixel povPixels1[LINES_OF_POV][NUM_LEDS_STRIP];
#ifdef DOUBLE_BUFFER
struct PovPixel povPixels2[LINES_OF_POV][NUM_LEDS_STRIP];
#endif
*/

struct PovPixel **povPixels1;
#ifdef DOUBLE_BUFFER
struct PovPixel **povPixels2;
#endif

//using PovLine = struct PovPixel[NUM_LEDS_STRIP];

struct PovPixel **povPixels = povPixels1;

APA102 strip2(NUM_LEDS_STRIP, VSPI_HOST, DATA_PIN2, CLOCK_PIN2, 24000000);
APA102 strip1(NUM_LEDS_STRIP, HSPI_HOST, DATA_PIN1, CLOCK_PIN1, 24000000);

static SemaphoreHandle_t mtx;

void swapPov(){
    #ifdef DOUBLE_BUFFER
    //xSemaphoreTake(mtx, portMAX_DELAY);
    povPixels = (povPixels == povPixels1) ? povPixels2 : povPixels1;
    //xSemaphoreGive(mtx);
    #else
    
    #endif
}

struct PovPixel  ** getDraftBuffer(){
    #ifdef DOUBLE_BUFFER
    struct PovPixel **ret;

    ret = (povPixels == povPixels1) ? povPixels2 : povPixels1;
  
    return ret;
    #else
    return povPixels;
    #endif
}
struct PovPixel  ** getDisplayBuffer(){
    return povPixels;
}

void draftPixel(int x, int y, uint8_t r,uint8_t g,uint8_t b,uint8_t bright) {
    struct PovPixel  **s=getDraftBuffer();

    if(y>=NUM_LEDS_STRIP||y<0) return;
    if(x>=LINES_OF_POV||x<0) return;
    
    y = NUM_LEDS_STRIP - y - 1;

    s[x][y].r = r;
    s[x][y].g = g;
    s[x][y].b = b;
    s[x][y].brigth = bright; 
}

void paintBall(uint8_t r,uint8_t g,uint8_t b,uint8_t bright){
  struct PovPixel **s=getDraftBuffer();

  for(int i=0;i<LINES_OF_POV;i++){
    for(int x=0;x<NUM_LEDS_STRIP;x++){
            s[i][x].g=g;
            s[i][x].r=r;
            s[i][x].b=b;
            s[i][x].brigth=bright;
        }   
    }   
}

void draftEarth(){
    struct PovPixel  **s=getDraftBuffer();

    for(int i=0;i<LINES_OF_POV;i++){
        if(i%10){
            for(int x=0;x<NUM_LEDS_STRIP;x++){
                if(x%5==0) s[i][x].g=255;
                else s[i][x].g=0;

                s[i][x].r=0;
                s[i][x].b=255;
                s[i][x].brigth=20;
                
            }
            
        } else {
            for(int x=0;x<NUM_LEDS_STRIP;x++){
                s[i][x].r=255;
                if(x%5==0) s[i][x].g=255;
                else s[i][x].g=0;
                s[i][x].b=0;
                s[i][x].brigth=5;     
            }
        }
    }
}

void initPov() {
    povPixels1 = (PovPixel**) malloc(LINES_OF_POV * sizeof(PovPixel*));
    for (int i = 0; i < LINES_OF_POV; i++) {
        povPixels1[i] = (PovPixel*) malloc(NUM_LEDS_STRIP * sizeof(PovPixel));
    }

    #ifdef DOUBLE_BUFFER
    povPixels2 = (PovPixel**) malloc(LINES_OF_POV * sizeof(PovPixel*));
    for (int i = 0; i < LINES_OF_POV; i++) {
        povPixels2[i] = (PovPixel*) malloc(NUM_LEDS_STRIP * sizeof(PovPixel));
    }
    #endif

    mtx = xSemaphoreCreateMutex();

    if (!FILESYSTEM.begin(true)) {
        Serial.println("❌ LittleFS init failed!");
        dbg("LittleFS init failed!");
    }

    pinMode(HALL_PIN, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(HALL_PIN), hallInterruptHandler, nullptr,
                   ESP_INTR_FLAG_LEVEL4 | ESP_INTR_FLAG_IRAM | FALLING);

    //attachInterrupt(HALL_PIN, hallInterruptHandler, FALLING);   // pudotusreuna
    paintBall(0,0,0,0);
    swapPov();
    
    xTaskCreatePinnedToCore(
        povTask,            // funktio
        "povTask",          // selkeä nimi debuggausta varten
        4096,              // stack-koko sanoina (≈ 16 kB)
        nullptr,           // parametrit
        22,                 // prioriteetti (0-24, 1 on “low but not idle”)
        nullptr,           // jos tarvitset kahvan, talleta tähän
        1                  // ydin: 0 = PRO_CPU, 1 = APP_CPU
    );
  
}


u_int64_t time_pov_us=5000000;
bool syncPoint = false;
u_int64_t tm0 = esp_timer_get_time();



void IRAM_ATTR  hallInterruptHandler(void *c) {
    u_int64_t t = esp_timer_get_time();  
    u_int64_t tmp = t - tm0;
    
    if(tmp<10000) {
        return;
    }

    time_pov_us = tmp;
    tm0 = t;
    syncPoint = true;
}

u_int64_t update_time = 1370;

u_int64_t getAndCalculatePovRoundTime(u_int64_t t) {
    static u_int64_t tmp[10];
    static int k = 0;

    tmp[k] = t;
    k++;
    if(k>=10) k=0;

    u_int64_t ret=0;
    for(int i=0;i<10;i++) ret += tmp[i];

    return ret/10;
}


u_int64_t pov_line_tm = 0;
u_int64_t pov_rotation_tm = 0;

void povRotate(int i){
    struct PovPixel  ** draf = getDraftBuffer();
    struct PovPixel  ** dist = getDisplayBuffer();
    for(int i=1;i<LINES_OF_POV;i++){
        memcpy(draf[i-1], dist[i], NUM_LEDS_STRIP*sizeof(struct PovPixel));
    }
    memcpy(draf[LINES_OF_POV-1], dist[0], NUM_LEDS_STRIP*sizeof(struct PovPixel));
}
void deletePictureFromDisk(int index) {
    char name[10];

    sprintf(name,"/%pd.bin", index);
    FILESYSTEM.remove(name);           // LittleFS.remove(polku)
}


bool storeCurrentDisplayToDisk(int index){
    char name[10];

    sprintf(name,"/%pd.bin", index);

    APA102::disableApa102();

    File f = FILESYSTEM.open(name, FILE_WRITE);
    int size = 0;

    size_t len = LINES_OF_POV * NUM_LEDS_STRIP * sizeof(PovPixel);

    //uint8_t *buffer = (uint8_t*)malloc(len);

    for(int line = 0;line<LINES_OF_POV;line++){
        //memcpy(&buffer[line*LINES_OF_POV], povPixels[line], NUM_LEDS_STRIP*sizeof(struct PovPixel));
        f.write((uint8_t*)povPixels[line], NUM_LEDS_STRIP*sizeof(struct PovPixel));
    }

    f.flush();

    dbg("!!!!!!!!!!!!!!!!!!!!!!!Save disk index", index);
    dbg("Save disk size ", f.size());
    
    f.close();
    APA102::enableApa102();

    return true;
}

bool loadPictureFromDisk(int index){
    char name[10];

    sprintf(name,"/%pd.bin", index);

    if(!FILESYSTEM.exists(name)) return false;

    dbg("Read Picture1");
    File f = FILESYSTEM.open(name, FILE_READ);

        dbg("Read disk size ", f.size());

    struct PovPixel  ** draf = getDraftBuffer();

    for(int line = 0;line<LINES_OF_POV;line++){
        for(int leds=0;leds<NUM_LEDS_STRIP;leds++)
            f.read((uint8_t *)&draf[line][leds], sizeof(struct PovPixel));
    }

    f.close();
    dbg("Read Picture2");
    return true;
}


SemaphoreHandle_t startSemaphore1;
SemaphoreHandle_t startSemaphore2;
SemaphoreHandle_t doneSemaphore;
void showTask1(void*) {
    while(1){
        xSemaphoreTake(startSemaphore1, portMAX_DELAY);
        strip1.show();
        xSemaphoreGive(doneSemaphore);  // Ilmoita että valmis
    }
    vTaskDelete(NULL);
}

void showTask2(void*) {
    while(1){
        xSemaphoreTake(startSemaphore2, portMAX_DELAY);
        strip2.show();
        xSemaphoreGive(doneSemaphore);  // Ilmoita että valmis
    }
    vTaskDelete(NULL);
}
void updateStripsSimultaneously() {
    xSemaphoreGive(startSemaphore1);
    xSemaphoreGive(startSemaphore2);
    // Odota kunnes molemmat ovat valmiit
    xSemaphoreTake(doneSemaphore, portMAX_DELAY);
    xSemaphoreTake(doneSemaphore, portMAX_DELAY);
}

void povTask(void *pvParameters){       
    strip1.begin();
    strip2.begin();
    doneSemaphore = xSemaphoreCreateCounting(2, 0); // odottaa kahta "valmista"
    startSemaphore1 = xSemaphoreCreateCounting(1, 0); 
    startSemaphore2 = xSemaphoreCreateCounting(1, 0); 

    xTaskCreatePinnedToCore(showTask1, "APA102Show1", 4096, NULL, 22, NULL, 1);
    xTaskCreatePinnedToCore(showTask2, "APA102Show2", 4096, NULL, 22, NULL, 1);


    static uint8_t baseHue = 0;

    for (uint16_t i = 0; i < NUM_LEDS_STRIP; ++i) {
        uint8_t r, g, b;
        strip1.setPixel(i, 0, 0, 0, 20);     // maks. kirkkaus
        strip2.setPixel(i, 0, 0, 0, 20);     // maks. kirkkaus
    }
    strip1.show();                         
    strip2.show();
    
    u_int64_t pov_us;
 
    u_int64_t time_pov_us_tmp=70000;
    int time_pov_us_cnt=0;

    while(1){
        esp_task_wdt_reset();
        if(syncPoint){
            pov_us = getAndCalculatePovRoundTime(time_pov_us);
            pov_rotation_tm = pov_us;

            if(pov_us>1000000) pov_us=1000000;
            
            syncPoint = false;
     
            u_int64_t pov_line_us = pov_us/LINES_OF_POV;
            
            u_int64_t tt0 = esp_timer_get_time() + pov_line_us;

            xSemaphoreTake(mtx, portMAX_DELAY);
            u_int64_t tm_line;
            int i2;
            for(int i=0;i<LINES_OF_POV &&syncPoint==false;i++){
                tm_line = micros();
                strip1.setPixels(povPixels[i], NUM_LEDS_STRIP);
                i2 = i+LINES_OF_POV/2;
                if(i2>=LINES_OF_POV) i2=i2-LINES_OF_POV;
                strip2.setPixels(povPixels[i2], NUM_LEDS_STRIP);
                
                //strip1.show();                         
                //strip2.show();
                updateStripsSimultaneously();
                pov_line_tm = micros()-tm_line;

                while(esp_timer_get_time()<tt0 && syncPoint==false);
                tt0 += pov_us/LINES_OF_POV;
            }
            xSemaphoreGive(mtx);
        }
        
        while(otaOngoing){
            esp_task_wdt_reset();
            paintBall(0,0,0,0);
            swapPov();  
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
        
}


#endif