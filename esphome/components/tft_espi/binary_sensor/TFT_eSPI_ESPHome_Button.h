#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include <vector>
#include "TFT_eSPI.h"              // Hardware-specific library
#include "TFT_eWidget.h"           // Widget library
#include "../tft_espi.h"

//#define BUTTON_W 100
//#define BUTTON_H 50

namespace esphome {
namespace tft_espi_widgets {
// see https://github.com/Bodmer/TFT_eWidget/blob/main/examples/Buttons/Button_demo/Button_demo.ino

    class TFT_eSPI_ESPHome_Button : public binary_sensor::BinarySensorInitiallyOff , public Component {
        private:
            TFT_eSPI* tft;
            ButtonWidget* btnL; 
            uint16_t x_;
            uint16_t y_;
            uint16_t width_;
            uint16_t height_;
            void PressAction();
            void ReleaseAction();
        public:
        TFT_eSPI_ESPHome_Button(esphome::tft_espi::TFT_eSPI_ESPHome*& owner, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
            this->tft = owner->TFTptr();
            this->btnL = new ButtonWidget(tft);
            set_position(x, y,  width, height);
        }
        void set_position(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
            ESP_LOGD("TFT_eSPI_ESPHome_Button", "set_position");
            //btnL->initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_YELLOW, TFT_BLACK, "Button", 1);
            this->x_ = x;
            this->y_ = y;
            this->width_ = width;
            this->height_ = height;
        }
        void setup() override {
            ESP_LOGD("TFT_eSPI_ESPHome_Button", "setup");
            uint16_t x = x_; //(tft->width() - BUTTON_W) / 2;
            uint16_t y = y_; //tft->height() / 2 - BUTTON_H - 10;
            btnL->initButtonUL(x, y, width_, height_, TFT_WHITE, TFT_YELLOW, TFT_BLACK, "Button", 1);
            btnL->drawSmoothButton(false, 3, TFT_BLACK); // 3 is outline width, TFT_BLACK is the surrounding background colour for anti-aliasing
        }
        void loop() override;
    };
}
}
