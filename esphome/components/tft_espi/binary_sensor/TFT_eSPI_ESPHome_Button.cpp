#include "TFT_eSPI_ESPHome_Button.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tft_espi_widgets {

static const char *const TAG = "tft_espi_widgets";


void TFT_eSPI_ESPHome_Button::PressAction()
{
  if (btnL->justPressed()) {
    ESP_LOGI(TAG, "button just pressed");
    btnL->drawSmoothButton(true);
    btnL->drawSmoothButton(!btnL->getState(), 3, TFT_BLACK, btnL->getState() ? "OFF" : "ON");
  }
}

void TFT_eSPI_ESPHome_Button::ReleaseAction()
{
  if (btnL->justReleased()) {
    ESP_LOGI(TAG, "button just released");
    btnL->drawSmoothButton(false);
    btnL->setReleaseTime(millis());
  }
  else {
    
  }
}


void TFT_eSPI_ESPHome_Button::loop() {
  static uint32_t scanTime = millis();
  uint16_t t_x = 9999, t_y = 9999; // To store the touch coordinates

  // Scan keys every 50ms at most
  if (millis() - scanTime >= 50) {
    // Pressed will be set true if there is a valid touch on the screen
    bool pressed = tft->getTouch(&t_x, &t_y);
    scanTime = millis();
    
    if (pressed) {
      ESP_LOGI(TAG, "button just pressed %d %d", t_x, t_y);
      if (btnL->contains(t_x, t_y)) {
        btnL->press(true);
        btnL->pressAction();
      }
    }
    else {
      btnL->press(false);
      btnL->releaseAction();
    }
    
  }
}
/*
void TFT_eSPI_ESPHome_Button::setup() {
  ESP_LOGCONFIG(TAG, "Setting up tft_espi_widgets... ");
  delay(2);
}
void TFT_eSPI_ESPHome_Button::dump_config() {
  ESP_LOGCONFIG(TAG, "TFT_eSPI_ESPHome_Button:");

}
*/
}  // namespace tft_espi_widgets
}  // namespace esphome
