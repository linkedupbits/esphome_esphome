#ifndef TDISPLAY_S3
#define TDISPLAY_S3

#include "Free_Fonts.h"
#include "TFT_eSPI.h"
#include "esphome.h"
#include "TFT_eWidget.h"

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/display/display_color_utils.h"

#define BUTTON_W 100
#define BUTTON_H 50
// Pause in milliseconds between screens, change to 0 to time font rendering
#define WAIT 500

namespace esphome {
namespace tft_espi {

static const char *const TAG = "TFT_eSPI";

class TDisplayS3 : public PollingComponent
{
  public:
    void setup() override {
        ESP_LOGD("custom", "TDisplayS3 startup");
        tft.begin();
        //tft.setRotation(1);
        tft.fillScreen(TFT_BLACK);
        // Wrap test at right and bottom of screen
        tft.setTextWrap(true, true);
        // Font and background colour, background colour is used for anti-alias blending
        tft.setTextColor(0x55fd, TFT_BLACK);
        tft.setCursor(0, 0);
        tft.setFreeFont(FM9);
        tft.println("\nBooting display ..");
        ESP_LOGD("tft_espi", "TDisplayS3 width %d height %d ", get_width_internal(), get_height_internal());
        //spr.setColorDepth(8); // Optionally set depth to 8 to halve RAM use
        //spr.createSprite(get_width_internal(), get_height_internal());
        // spr.fillSprite(TFT_TRANSPARENT);
    }

    void loop() override {

    }

/*
    //////////
    // DisplayBuffer methods
    //////////
    void fill(Color color) override {
        ESP_LOGD("tft_espi", "TDisplayS3 fill");
        spr.fillScreen(display::ColorUtil::color_to_565(color));
    }

    int get_width_internal() override {
        return tft.getViewportWidth();
    }

    int get_height_internal() override {
	return tft.getViewportHeight();
    }

    display::DisplayType get_display_type() override {
        return display::DisplayType::DISPLAY_TYPE_COLOR;
    }

    void draw_absolute_pixel_internal(int x, int y, Color color) override {
        spr.drawPixel(x, y, display::ColorUtil::color_to_565(color));
    }
*/
    /////////////
    // PollingComponent Methods
    /////////////
    void update() override {
        ESP_LOGV("tft_espi", "tft_espi update");
        this->do_update_();
        //spr.pushSprite(0, 0, TFT_TRANSPARENT);
    }

  private:
    TFT_eSPI tft = TFT_eSPI();

};

}  // namespace tdisplays3
}  // namespace esphome

#endif
