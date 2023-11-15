#ifndef TFT_ESPI
#define TFT_ESPI
#include <mutex>

#include "Free_Fonts.h"
#include "TFT_eSPI.h"
#include "esphome.h"

#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tft_espi {

static const char *const TAG = "TFT_eSPI";

class TFT_eSPI_ESPHome;
using display_writer_t = std::function<void(TFT_eSPI_ESPHome &)>;

class TFT_eSPI_ESPHome : public PollingComponent
{
  public:
    TFT_eSPI_ESPHome() : PollingComponent(500) { }
    void setup() override {
        ESP_LOGD("custom", "TDisplayS3 startup");
        tft.begin();
        //tft.setRotation(1);
        tft.fillScreen(TFT_BLACK);
        // Wrap test at right and bottom of screen
        tft.setTextWrap(true, true);
        // Font and background colour, background colour is used for anti-alias blending
        tft.setTextColor(0x55fd, TFT_BLACK);
        tft.setCursor(4, 4);
        tft.setFreeFont(FM9);
        tft.println("\nBooting display ..");
        tft.drawRect(0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_BLUE);
        tft.drawRect(1, 1, TFT_WIDTH-2, TFT_HEIGHT-2, TFT_BLUE);
        ESP_LOGD("tft_espi", "TDisplayS3 width %d height %d ", TFT_WIDTH, TFT_HEIGHT);
        //spr.setColorDepth(8); // Optionally set depth to 8 to halve RAM use
        //spr.createSprite(get_width_internal(), get_height_internal());
        // spr.fillSprite(TFT_TRANSPARENT);
        do_init_();
    }

    void loop() override {

    }

    void set_init(display_writer_t &&init) { this->initlambda_ = init; }
    void set_writer(display_writer_t &&writer) { this->writer_ = writer; }
    /// @brief this method prints some text
    void PrintText() {
        //tft.setCursor(4, 50);
        tft.println("From Lambda ...");
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
        do_update_();
        //spr.pushSprite(0, 0, TFT_TRANSPARENT);
    }

    TFT_eSPI TFT () {return tft;}
    TFT_eSPI* TFTptr () {return &tft;}
  protected:
    void do_init_() {
        std::lock_guard<std::mutex> guard(lambda_mutex);
        if (this->initlambda_.has_value()) {
            (*this->initlambda_)(*this);
        }
    }
    void do_update_() {
        std::lock_guard<std::mutex> guard(lambda_mutex);
        if (this->writer_.has_value()) {
            (*this->writer_)(*this);
        }
    }
    
  private:
    std::mutex lambda_mutex;
    TFT_eSPI tft = TFT_eSPI();
    optional<display_writer_t> initlambda_{};
    optional<display_writer_t> writer_{};

};

}  // namespace tdisplays3
}  // namespace esphome

#endif
