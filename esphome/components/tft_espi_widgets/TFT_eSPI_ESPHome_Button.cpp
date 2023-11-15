#include "TFT_eSPI_ESPHome_Button.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tft_espi_widgets {

static const char *const TAG = "tft_espi_widgets";

void TFT_eSPI_ESPHome_Button::setup() {
  ESP_LOGCONFIG(TAG, "Setting up tft_espi_widgets... ");
  delay(2);
}
void TFT_eSPI_ESPHome_Button::dump_config() {
  ESP_LOGCONFIG(TAG, "TFT_eSPI_ESPHome_Button:");

}

}  // namespace ttp229_bsf
}  // namespace esphome
