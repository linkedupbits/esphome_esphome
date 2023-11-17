#include "tft_espi_esphome.h"

namespace esphome {
namespace tft_espi {

    void TFT_eSPI_ESPHome::setup()  {
        ESP_LOGD("custom", "TDisplayS3 startup");
        tft.begin();
        //tft.setRotation(1);
        tft.fillScreen(TFT_BLACK);
        // Wrap test at right and bottom of screen
        tft.setTextWrap(true, true);
        // Font and background colour, background colour is used for anti-alias blending
        tft.setTextColor(0x55fd, TFT_BLACK);
        //tft.setCursor(4, 4);
        tft.setFreeFont(FM9);
        //tft.println("\nBooting display ..");
        ESP_LOGD("tft_espi", "TDisplayS3 width %d height %d ", TFT_WIDTH, TFT_HEIGHT);
        //spr.setColorDepth(8); // Optionally set depth to 8 to halve RAM use
        //spr.createSprite(get_width_internal(), get_height_internal());
        // spr.fillSprite(TFT_TRANSPARENT);
        do_init_();
    }

    void TFT_eSPI_ESPHome::loop()  {

    }

}
}