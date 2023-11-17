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
namespace tft_espi_sprite {
// see https://github.com/Bodmer/TFT_eWidget/blob/main/examples/Buttons/Button_demo/Button_demo.ino

    class TFT_eSprite_ESPHome : public Component {

        public:
            void setup() override;
            void loop() override;
    }
}
}
