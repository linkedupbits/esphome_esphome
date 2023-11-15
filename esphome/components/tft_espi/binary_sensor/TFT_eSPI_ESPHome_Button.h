#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include <vector>

#include "../tft_espi.h"

namespace esphome {
namespace tft_espi_widgets {

    class TFT_eSPI_ESPHome_Button : public binary_sensor::BinarySensorInitiallyOff , Component {
        public:
            TFT_eSPI_ESPHome_Button();
    };
}
}
