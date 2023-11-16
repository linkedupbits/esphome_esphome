#Attribution   
These components rely on https://gite.lirmm.fr/doccy/TFT_eSPI_Widgets   

``` yaml 
tft_espi:
  miso_pin: 12
  mosi_pin: 13
  width: 320
  height: 480
  cs_pin: 15
  clk_pin: 14
  dc_pin: 2
  init: |-
    auto colour_white = Color(255, 255, 255);
    //it.TFT().setRotation(1);
    it.PrintText();
    //it.TFT().setCursor(4, 140);
    it.TFT().println("init lambda text ...\n");
    it.TFT().println("more text ...\n");
    delay(20);

binary_sensor:
  - platform: tft_espi
    id: Wibble
```