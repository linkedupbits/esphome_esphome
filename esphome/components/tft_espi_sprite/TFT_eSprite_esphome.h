#ifndef TFT_eSprite_esphome
#define TFT_eSprite_esphome

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
//#include "esphome/components/binary_sensor/binary_sensor.h"

#include <vector>
#include "TFT_eSPI.h"              // Hardware-specific library
#include "../tft_espi/tft_espi_esphome.h"

//#define BUTTON_W 100
//#define BUTTON_H 50

namespace esphome {
namespace tft_espi_sprite {
// see https://github.com/Bodmer/TFT_eWidget/blob/main/examples/Buttons/Button_demo/Button_demo.ino

    class TFT_eSprite_ESPHome : public Component {
        private:
            TFT_eSPI* tft;

        public:
            TFT_eSprite_ESPHome(void** /*tft_espi::TFT_eSPI_ESPHome**/ owner) {
                this->tft = owner->TFTptr();
            }
            void setup() override;
            void loop() override;
    };
}
}

#endif