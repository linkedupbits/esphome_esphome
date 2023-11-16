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
    this->publish_state(true);
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
  this->publish_state(false);
}

void TFT_eSPI_ESPHome_Button::Init_Calibration() {
  tft->setTouch(calibrationData);
  //tft->calibrateTouch(calibrationData, TFT_WHITE, TFT_RED, 15);
  /*
  // check if calibration file exists
  if (SPIFFS->exists(CALIBRATION_FILE)) {
    File f = SPIFFS->open(CALIBRATION_FILE, "r");
    if (f) {
      if (f.readBytes((char *)calibrationData, 14) == 14)
        calDataOK = 1;
      f.close();
    }
  }
  if (calDataOK) {
    // calibration data valid
    tft->setTouch(calibrationData);
  } else {
    // data not valid. recalibrate
    tft->calibrateTouch(calibrationData, TFT_WHITE, TFT_RED, 15);
    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calibrationData, 14);
      f.close();
    }
  }
  */
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
      std::__cxx11::string message = "";
        message = to_string(calibrationData[0])
                  + " " + to_string(calibrationData[1])
                  + " " + to_string(calibrationData[2])
                  + " " + to_string(calibrationData[3])
                  + " " + to_string(calibrationData[4]) ; //<< " " << calibrationData[2] << " " << calibrationData[3]<< " " << calibrationData[4];
        ESP_LOGI("Calibration %s", message.c_str());
              
      ESP_LOGI(TAG, "button just pressed %d %d", t_x, t_y);
      if (btnL->contains(t_x, t_y)) {
        ESP_LOGI(TAG, "Contains");
        btnL->press(true);
        PressAction();
      }
      else {
        ESP_LOGI(TAG, "Not contains");
      }
    }
    else {
      if (btnL->isPressed()) {
        btnL->press(false);
        ReleaseAction();
      }
    }
    
  }
}

void void TFT_eSPI_ESPHome_Button::configure(esphome::tft_espi::TFT_eSPI_ESPHome*& owner, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
      this->tft = owner->TFTptr();
      this->btnL = new ButtonWidget(tft);
      set_position(x, y,  width, height);
}

void TFT_eSPI_ESPHome_Button::setup() {
    ESP_LOGD(TAG, "setup");
    if (this->tft) {
      Init_Calibration();
      this->publish_initial_state(false);
      uint16_t x = x_; //(tft->width() - BUTTON_W) / 2;
      uint16_t y = y_; //tft->height() / 2 - BUTTON_H - 10;
      btnL->initButtonUL(x, y, width_, height_, TFT_WHITE, TFT_YELLOW, TFT_BLACK, "Button", 1);
      btnL->drawSmoothButton(false, 3, TFT_BLACK); // 3 is outline width, TFT_BLACK is the surrounding background colour for anti-aliasing
    } else {
      ESP_LOGW(TAG, "setup > this->tft is null");
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
