#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include <vector>
#include "TFT_eSPI.h"              // Hardware-specific library
#include "TFT_eWidget.h"           // Widget library
#include "../tft_espi.h"

namespace esphome {
namespace tft_espi_widgets {

    class TFT_eSPI_ESPHome_Button : public binary_sensor::BinarySensorInitiallyOff , public Component {
        private:
           ButtonWidget* btnL; 
        
        public:
        TFT_eSPI_ESPHome_Button(esphome::tft_espi::TFT_eSPI_ESPHome*& tft) {
            btnL = new ButtonWidget(&tft);
        }
    };
}
}
