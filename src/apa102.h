#pragma once
#include <Arduino.h>
#include <driver/spi_master.h>
#include <vector>

 struct PovPixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t brigth;
 };
  
 
 static bool apaenabled = true;

class APA102 {
public:
  APA102(uint16_t n, spi_host_device_t host = VSPI_HOST,
         uint8_t pinMOSI = 23, uint8_t pinSCK = 18,
         uint32_t hz = 20000000);
  void begin();

  inline void setPixel(uint16_t i,
                      uint8_t r, uint8_t g, uint8_t b,
                      uint8_t br)
  {
      if (!_spiBuffer) return;

      // Laske offset ensimmäisen LED-pikselin alkuun:
      size_t pixelOffset = 4 + i * 4; // 4 = start-frame

      // APA102: 0b111xxxxx kirkkaus, sitten BGR
      _spiBuffer[pixelOffset + 0] = 0b11100000 | (br & 0x1F); // kirkkaus
      _spiBuffer[pixelOffset + 1] = b;
      _spiBuffer[pixelOffset + 2] = g;
      _spiBuffer[pixelOffset + 3] = r;
  }

  //inline void setPixels(struct PovPixel *pixels, uint16_t len);
  inline void show()
  {
      // Kirjoitetaan start-frame (4 nollaa)
      for (int i = 0; i < 4; ++i) _spiBuffer[i] = 0;

      // LED-data on jo paikallaan

      // Kirjoitetaan end-frame (täytetään loppu 0xFF:llä)
      size_t endStart = 4 + _num * 4;
      for (size_t i = 0; i < endLen; ++i) {
        _spiBuffer[endStart + i] = 0xFF;
      }

      spi_transaction_t t = {};
      t.length = _spiBufferLen * 8;  // bits
      t.tx_buffer = _spiBuffer;
      ESP_ERROR_CHECK(spi_device_transmit(_spi, &t));
  }

  static void disableApa102(){
    apaenabled = false;
  }

  static void enableApa102(){
    apaenabled = true;
  }

  inline void setPixels(struct PovPixel *pixels, uint16_t len){
    for(int i=0;i<len;i++){
      setPixel(i, pixels[i].r,pixels[i].g,pixels[i].b,pixels[i].brigth);
    }
  } 

private:
  uint16_t _num;
  spi_device_handle_t _spi;

  uint8_t *_spiBuffer = NULL;

  size_t endLen;
  size_t _spiBufferLen;  
};