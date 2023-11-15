#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include <vector>
#include "TFT_eSPI.h"              // Hardware-specific library
#include "TFT_eWidget.h"           // Widget library
#include "../tft_espi.h"

#define BUTTON_W 100
#define BUTTON_H 50

namespace esphome {
namespace tft_espi_widgets {

    class TFT_eSPI_ESPHome_Button : public binary_sensor::BinarySensorInitiallyOff , public Component {
        private:
            TFT_eSPI* tft;
            ButtonWidget* btnL; 
           
        public:
        TFT_eSPI_ESPHome_Button(esphome::tft_espi::TFT_eSPI_ESPHome*& owner) {
            this->tft = owner->TFTptr();
            this->btnL = new ButtonWidget(tft);
        }

        void setup() override {
            ESP_LOGD("custom", "TDisplayS3 startup");
            uint16_t x = (tft->width() - BUTTON_W) / 2;
            uint16_t y = tft->height() / 2 - BUTTON_H - 10;
            btnL->initButtonUL(x, y, BUTTON_W, BUTTON_H, TFT_WHITE, TFT_RED, TFT_BLACK, "Button", 1);
            btnL->drawSmoothButton(false, 3, TFT_BLACK); // 3 is outline width, TFT_BLACK is the surrounding background colour for anti-aliasing
        }
    };
}
}
