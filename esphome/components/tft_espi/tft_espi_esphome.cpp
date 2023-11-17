#include "tft_espi_esphome.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tft_espi {

    static const char *const TAG = "TFT_eSPI";

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

    void TFT_eSPI_ESPHome::PrintText() {
        //tft.setCursor(4, 50);
        //tft.println("From Lambda ...");
        tft.drawRect(0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_BLUE);
        tft.drawRect(1, 1, TFT_WIDTH-2, TFT_HEIGHT-2, TFT_BLUE);
    }


    void TFT_eSPI_ESPHome::update() override {
        ESP_LOGV("tft_espi", "tft_espi update");
        do_update_();
        //spr.pushSprite(0, 0, TFT_TRANSPARENT);
    }

    void TFT_eSPI_ESPHome::loop()  {

    }

    void TFT_eSPI_ESPHome::do_init_() {
        std::lock_guard<std::mutex> guard(lambda_mutex);
        if (this->initlambda_.has_value()) {
            (*this->initlambda_)(*this);
        }
    }

    void TFT_eSPI_ESPHome::do_update_() {
        std::lock_guard<std::mutex> guard(lambda_mutex);
        if (this->writer_.has_value()) {
            (*this->writer_)(*this);
        }
    }


/*
    //////////
    // DisplayBuffer methods
    //////////
    void TFT_eSPI_ESPHome::fill(Color color) override {
        ESP_LOGD("tft_espi", "TDisplayS3 fill");
        spr.fillScreen(display::ColorUtil::color_to_565(color));
    }

    int TFT_eSPI_ESPHome::get_width_internal() override {
        return tft.getViewportWidth();
    }

    int TFT_eSPI_ESPHome::get_height_internal() override {
	return tft.getViewportHeight();
    }

    display::DisplayType TFT_eSPI_ESPHome::get_display_type() override {
        return display::DisplayType::DISPLAY_TYPE_COLOR;
    }

    void TFT_eSPI_ESPHome::draw_absolute_pixel_internal(int x, int y, Color color) override {
        spr.drawPixel(x, y, display::ColorUtil::color_to_565(color));
    }
*/

}
}