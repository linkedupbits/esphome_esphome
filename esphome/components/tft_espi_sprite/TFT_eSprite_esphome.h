#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include <vector>
#include "TFT_eSPI.h"              // Hardware-specific library
#include "TFT_eWidget.h"           // Widget library
#include "esphome/components/tft_espi/tft_espi.h"

//#define BUTTON_W 100
//#define BUTTON_H 50

// Forward Declaration class A
class esphome::tft_espi::TFT_eSPI_ESPHome;

namespace esphome {
namespace tft_espi {
    
    class TFT_eSPI_ESPHome;
}
namespace tft_espi_sprite {
// see https://github.com/Bodmer/TFT_eWidget/blob/main/examples/Buttons/Button_demo/Button_demo.ino

    class TFT_eSprite_ESPHome : public Component {
        private:
            TFT_eSPI* tft;

        public:
            TFT_eSprite_ESPHome(esphome::tft_espi::TFT_eSPI_ESPHome* owner) {
                this->tft = owner->TFTptr();
            }
            void setup() override;
            void loop() override;
    };
}
}
