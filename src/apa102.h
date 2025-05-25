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
    frame = 0xE0000000 |            // “111” + GBR
                    (br & 0x1F) << 24 |
                    uint32_t(b) << 16 |
                    uint32_t(g) << 8  |
                    r;
    _ledFrame[i] = __builtin_bswap32(frame);  // MSB ensin DMA:lle
  }

  //inline void setPixels(struct PovPixel *pixels, uint16_t len);
  inline void show()
  {
    if(apaenabled==false) {
      delay(1);
      return;
    }

    /* start-frame */
    t.length    = sizeof(startFrame) * 8;
    t.tx_buffer = startFrame;
    spi_device_transmit(_spi, &t);

    /* LED-data */
    t.length    = _num * 32;
    t.tx_buffer = _ledFrame.data();
    spi_device_transmit(_spi, &t);

    /* end-frame */
    t.length    = endLen * 8;
    t.tx_buffer = endFrame.data();
    spi_device_transmit(_spi, &t);
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
  std::vector<uint32_t> _ledFrame;   // valmiiksi pakattu 32-bit/pixel

  const uint8_t startFrame[4] = {0, 0, 0, 0};
  size_t endLen;
  std::vector<uint8_t> endFrame;

  spi_transaction_t t = {};
  uint32_t frame;
};