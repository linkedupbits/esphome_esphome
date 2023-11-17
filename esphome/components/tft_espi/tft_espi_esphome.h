#ifndef tft_espi_esphome
#define tft_espi_esphome

#include <mutex>

#include "Free_Fonts.h"
#include "TFT_eSPI.h"
#include "esphome.h"

#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tft_espi {

class TFT_eSPI_ESPHome;
using display_writer_t = std::function<void(TFT_eSPI_ESPHome &)>;

class TFT_eSPI_ESPHome : public PollingComponent
{
  public:
    TFT_eSPI_ESPHome() : PollingComponent(500) { }
    void setup() override;
    void loop() override;

    void set_init(display_writer_t &&init) { this->initlambda_ = init; }
    void set_writer(display_writer_t &&writer) { this->writer_ = writer; }
    /// @brief this method prints some text
    void PrintText();

    /////////////
    // PollingComponent Methods
    /////////////
    void update() override;

    TFT_eSPI TFT () {return tft;}
    TFT_eSPI* TFTptr () {return &tft;}
  protected:
    void do_init_();
    void do_update_();
    
  private:
    std::mutex lambda_mutex;
    TFT_eSPI tft = TFT_eSPI();
    optional<display_writer_t> initlambda_{};
    optional<display_writer_t> writer_{};

};

}  // namespace tft_espi
}  // namespace esphome

#endif
