/*

   install esp32 extension:
  https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/


  OLED
   desc: https://www.instructables.com/ESP32-With-Integrated-OLED-WEMOSLolin-Getting-Star/   
   LIB https://github.com/ThingPulse/esp8266-oled-ssd1306

  google: wemos lolin esp32 oled

  board: ESP32 DEV Module
*/


/* OLED Kram*/
//  https://github.com/ThingPulse/esp8266-oled-ssd1306
//  "name": "ESP8266 and ESP32 OLED driver for SSD1306 displays",
//  "version": "4.6.1",
//  "keywords": "ssd1306, oled, display, i2c",
#include "SSD1306.h"  // alias for `#include "SSD1306Wire.h"'
SSD1306 display(0x3c, 5, 4);


#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "wlan_credicals.h"

struct {
  String hostName;
  String swDate;
} my;

void setup() {
  Serial.begin(115200);  // opens USB serial port


  // ##########################################
  // Display Stuff
  Serial.println("## initialize Display");

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);  //10,16,24
  display.setColor(WHITE);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  // ##########################################
  // init
  WiFi.begin(WLANSSID1, WLANPASS1);
  ota2_init();
  webServer2_init();
  serialComm_init();

  // ##########################################
  // host id und Version
  String hostId = String(__FILE__);
  int ix = hostId.lastIndexOf("/") + 1;
  hostId = hostId.substring(ix);
  ix = hostId.lastIndexOf(".ino");
  hostId = hostId.substring(0, ix);
  my.hostName = hostId;
  my.swDate = String(__DATE__);

  // ##########################################
  Serial.println("finished: void setup()");
}

void ota2_init() {

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else  // U_SPIFFS
        type = "filesystem";
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
    });

  ArduinoOTA.begin();
}

// ##########################################
// start loop
void loop() {
  delay(5);  //in msec

  // ##########################################
  webServer2_run();
  ArduinoOTA.handle();
  serialComm_run();

  // ##########################################
  // oled
  {
    static unsigned long t1 = 0;
    if (millis() - t1 > 2 * 1000) {
      t1 = millis();

      display.clear();
      display.setFont(ArialMT_Plain_10);  //10,16,24

      display.drawString(1, 0, my.hostName);
      display.drawString(1, 10, my.swDate);
      display.drawString(1, 20, WiFi.localIP().toString());

      display.display();
    }
  }
}
