#include "apa102.h"
#include <algorithm>   // fill
#include <vector>

/* -------- konstruktorissa: SPI-väylän ja laitteen luonti ---------- */
APA102::APA102(uint16_t n,
               spi_host_device_t host,
               uint8_t pinMOSI,
               uint8_t pinSCK,
               uint32_t hz)
    : _num(n), _ledFrame(n)
{
  spi_bus_config_t bus = {};
  bus.mosi_io_num     = pinMOSI;
  bus.miso_io_num     = -1;
  bus.sclk_io_num     = pinSCK;
  bus.max_transfer_sz = (n + 8) * 4;          // riittää start+data+end
 
  ESP_ERROR_CHECK(spi_bus_initialize(host, &bus, SPI_DMA_CH_AUTO));

  /* --> VANHA DESIGNATOR-ONGELMA POISTUU, kun asetetaan kentät erikseen */
  spi_device_interface_config_t dev = {};
  dev.clock_speed_hz = int(hz);               // 20 MHz tms.
  dev.mode           = 0;                     // CKI idle low, sample rising
  dev.spics_io_num   = -1;                    // ei CS-pinniä
  dev.queue_size     = 2;                     // kaks transaktiota jonossa

  /* END-frameksi ≥ N/2 '1'-bittiä → (N+15)/16 tavua 0xFF */
  endLen = (_num + 15) / 16;
  _ledFrame.resize(endLen, 0xff);

  ESP_ERROR_CHECK(spi_bus_add_device(host, &dev, &_spi));
}

/* -------------------- julkiset metodit ---------------------------- */
void APA102::begin()
{
  std::fill(_ledFrame.begin(), _ledFrame.end(), 0xE0000000); // kirkkaus=0
  show();                                   // nollaa nauhan
}






