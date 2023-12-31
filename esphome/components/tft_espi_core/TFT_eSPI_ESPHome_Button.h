#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include <vector>
#include "TFT_eSPI.h"     // Hardware-specific library
#include "TFT_eWidget.h"  // Widget library
#include "tft_espi_esphome.h"

//#define BUTTON_W 100
//#define BUTTON_H 50

namespace esphome {
namespace tft_espi_widgets {
// see https://github.com/Bodmer/TFT_eWidget/blob/main/examples/Buttons/Button_demo/Button_demo.ino

class TFT_eSPI_ESPHome_Button : public binary_sensor::BinarySensorInitiallyOff, public Component {
 private:
  TFT_eSPI *tft;
  ButtonWidget *btnL;
  uint16_t x_;
  uint16_t y_;
  uint16_t width_;
  uint16_t height_;
  void PressAction();
  void ReleaseAction();
  void Init_Calibration();

 protected:
  uint32_t scanTime;

 public:
  TFT_eSPI_ESPHome_Button() {}
  void set_position(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
  void setup() override;
  void loop() override;
  void configure(esphome::tft_espi_core::TFT_eSPI_ESPHome *&owner, uint16_t x, uint16_t y, uint16_t width,
                 uint16_t height);
};
}  // namespace tft_espi_widgets
}  // namespace esphome
