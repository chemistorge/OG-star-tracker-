#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <string.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include "config.h"
#include "website_strings.h"
#include "axis.h"
#include "hardwaretimer.h"
#include "index_html.h"
#include "intervalometer.h"
// try to update every time there is breaking change
const int firmware_version = 6;

// Set your Wi-Fi credentials
const byte DNS_PORT = 53;
const char* ssid = "OG Star Tracker";  //change to your SSID
const char* password = "password123";  //change to your password, must be 8+ characters
//If you are using AP mode, you can access the website using the below URL
const String website_name = "www.tracker.com";
#define MIN_CUSTOM_SLEW_RATE 2

unsigned long blink_millis = 0;

WebServer server(80);
DNSServer dnsServer;

// Handle requests to the root URL ("/")
void handleRoot() {
  server.send(200, MIME_TYPE_HTML, html_content);
}

void handleOn() {
  int tracking_speed = server.arg(TRACKING_SPEED).toInt();
  trackingRateS rate;
  switch (tracking_speed) {
    case 0:  //sidereal rate
      rate = TRACKING_SIDEREAL;
      break;
    case 1:  //solar rate
      rate = TRACKING_SOLAR;
      break;
    case 2:  //lunar rate
      rate = TRACKING_LUNAR;
      break;
    default:
      rate = TRACKING_SIDEREAL;
      break;
  }
  int direction = server.arg(DIRECTION).toInt();
  ra_axis.startTracking(rate, direction);
  server.send(200, MIME_TYPE_TEXT, TRACKING_ON);
}

void handleOff() {
  ra_axis.stopTracking();
  server.send(200, MIME_TYPE_TEXT, TRACKING_OFF);
}

void handleSlewRequest() {
  if (!ra_axis.slewActive) {  //if slew is not active - needed for ipad (passes multiple touchon events)
    int slew_speed = server.arg(SPEED).toInt();
    int direction = server.arg(DIRECTION).toInt();
    //limit custom slew speed to 2-400
    slew_speed = slew_speed > MAX_CUSTOM_SLEW_RATE ? MAX_CUSTOM_SLEW_RATE : slew_speed < MIN_CUSTOM_SLEW_RATE ? MIN_CUSTOM_SLEW_RATE
                                                                                                              : slew_speed;
    ra_axis.startSlew((2 * ra_axis.trackingRate) / slew_speed, direction);
    server.send(200, MIME_TYPE_TEXT, SLEWING);
  }
}

void handleSlewOff() {
  if (ra_axis.slewActive) {  //if slew is active needed for ipad (passes multiple touchoff events)
    ra_axis.stopSlew();
  }
  server.send(200, MIME_TYPE_TEXT, "SLEW CANCELLED");
}

void handleSetCurrent() {
  if (!intervalometer.intervalometerActive) {
    int modeInt = server.arg(CAPTURE_MODE).toInt();
    Intervalometer::Mode captureMode;
    switch (modeInt) {
      case 0:  //Long Exp Still
        captureMode = Intervalometer::LONG_EXPOSURE_STILL;
        break;
      case 1:  //Long Exp Movie
        captureMode = Intervalometer::LONG_EXPOSURE_MOVIE;
        break;
      case 2:  //day time lapse
        captureMode = Intervalometer::DAY_TIME_LAPSE;
        break;
      case 3:
        captureMode = Intervalometer::DAY_TIME_LAPSE_PAN;
        break;
      default:
        captureMode = Intervalometer::LONG_EXPOSURE_STILL;
        break;
    }
    intervalometer.currentSettings.mode = captureMode;
    intervalometer.currentSettings.exposureTime = server.arg(EXPOSURE_TIME).toInt();
    intervalometer.currentSettings.exposures = server.arg(EXPOSURES).toInt();
    intervalometer.currentSettings.preDelay = server.arg(PREDELAY).toInt();
    intervalometer.currentSettings.delayTime = server.arg(DELAY).toInt();
    intervalometer.currentSettings.frames = server.arg(FRAMES).toInt();
    intervalometer.currentSettings.panAngle = server.arg(PAN_ANGLE).toFloat() / 100;
    intervalometer.currentSettings.panDirection = server.arg(PAN_DIRECTION).toInt();
    intervalometer.currentSettings.enableTracking = server.arg(ENABLE_TRACKING).toInt();
    intervalometer.currentSettings.dither = server.arg(DITHER_CHOICE).toInt();
    intervalometer.currentSettings.ditherFrequency = server.arg(DITHER_FREQUENCY).toInt();
    intervalometer.currentSettings.focalLength = server.arg(FOCAL_LENGTH).toInt();
    intervalometer.currentSettings.pixelSize = server.arg(PIXEL_SIZE).toFloat() / 100;
    String currentMode = server.arg(MODE);
    if (currentMode == "save") {
      int preset = server.arg(PRESET).toInt();
      intervalometer.saveSettingsToPreset(preset);
      server.send(200, MIME_TYPE_TEXT, "Saved Preset");
    } else if (currentMode == "start") {
      if ((intervalometer.currentSettings.mode == Intervalometer::LONG_EXPOSURE_MOVIE || intervalometer.currentSettings.mode == Intervalometer::LONG_EXPOSURE_STILL) && !ra_axis.trackingActive) {
        server.send(200, MIME_TYPE_TEXT, "TRACKING NOT ACTIVE!! Please start");
      } else {
        intervalometer.startCapture();
        server.send(200, MIME_TYPE_TEXT, CAPTURE_ON);
      }
    }
  } else {
    server.send(200, MIME_TYPE_TEXT, CAPTURE_ALREADY_ON);
  }
}


void handleGetPresetExposureSettings() {
  int preset = server.arg(PRESET).toInt();
  intervalometer.readSettingsFromPreset(preset);
  JsonDocument settings;
  String json;
  settings[MODE] = intervalometer.currentSettings.mode;
  settings[EXPOSURES] = intervalometer.currentSettings.exposures;
  settings[DELAY] = intervalometer.currentSettings.delayTime;
  settings[PREDELAY] = intervalometer.currentSettings.preDelay;
  settings[EXPOSURE_TIME] = intervalometer.currentSettings.exposureTime;
  settings[PAN_ANGLE] = intervalometer.currentSettings.panAngle * 100;
  settings[PAN_DIRECTION] = intervalometer.currentSettings.panDirection;
  settings[DITHER_CHOICE] = intervalometer.currentSettings.dither;
  settings[DITHER_FREQUENCY] = intervalometer.currentSettings.ditherFrequency;
  settings[ENABLE_TRACKING] = intervalometer.currentSettings.enableTracking;
  settings[FRAMES] = intervalometer.currentSettings.frames;
  settings[PIXEL_SIZE] = intervalometer.currentSettings.pixelSize * 100;
  settings[FOCAL_LENGTH] = intervalometer.currentSettings.focalLength;
  serializeJson(settings, json);
  // Serial.println(json);
  // server.send(200, "application/json", json);
}

void handleAbortCapture() {
  if (intervalometer.intervalometerActive) {
    intervalometer.abortCapture();
    server.send(200, MIME_TYPE_TEXT, CAPTURE_OFF);
  } else {
    server.send(200, MIME_TYPE_TEXT, CAPTURE_ALREADY_OFF);
  }
}

void handleStatusRequest() {
  if (intervalometer.intervalometerActive) {
    switch (intervalometer.currentState) {
      case Intervalometer::PRE_DELAY:
        server.send(200, MIME_TYPE_TEXT, "Capture: Predelay");
        break;
      case Intervalometer::CAPTURE:
        server.send(200, MIME_TYPE_TEXT, "Capture: Exposing");
        break;
      case Intervalometer::DITHER:
        server.send(200, MIME_TYPE_TEXT, "Capture: Dither");
        break;
      case Intervalometer::PAN:
        server.send(200, MIME_TYPE_TEXT, "Capture: Panning");
        break;
      case Intervalometer::DELAY:
        server.send(200, MIME_TYPE_TEXT, "Capture: Delay");
        break;
      case Intervalometer::REWIND:
        server.send(200, MIME_TYPE_TEXT, "Capture: Rewind");
        break;
      case Intervalometer::INACTIVE:
      default:
        break;
    }
  } else if (ra_axis.slewActive) {
    server.send(200, MIME_TYPE_TEXT, SLEWING);
  } else if (ra_axis.trackingActive) {
    server.send(200, MIME_TYPE_TEXT, TRACKING_ON);
  }

  if (ra_axis.slewActive) {
    slewTimeOut.setCountValue(0);  //reset timeout counter
  }


  server.send(204, MIME_TYPE_TEXT, "dummy");
}

void handleVersion() {
  server.send(200, MIME_TYPE_TEXT, (String)firmware_version);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);  //SIZE = 5 x presets = 5 x 32 bytes = 160 bytes
  intervalometer.readPresetsFromEEPROM();
#ifdef AP
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(ssid, password);
  delay(500);
  Serial.println("Creating Wifi Network");

  //ANDROID 10 WORKAROUND==================================================
  //set new WiFi configurations
  WiFi.disconnect();
  Serial.println("reconfig WiFi...");
  /*Stop wifi to change config parameters*/
  esp_wifi_stop();
  esp_wifi_deinit();
  /*Disabling AMPDU RX is necessary for Android 10 support*/
  wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();  //We use the default config ...
  my_config.ampdu_rx_enable = 0;                              //... and modify only what we want.
  Serial.println("WiFi: Disabled AMPDU...");
  esp_wifi_init(&my_config);  //set the new config = "Disable AMPDU"
  esp_wifi_start();           //Restart WiFi
  delay(500);
  //ANDROID 10 WORKAROUND==================================================
#else
  WiFi.mode(WIFI_MODE_STA);  // Set ESP32 in station mode
  WiFi.begin(ssid, password);
  Serial.println("Connecting to Network in STA mode");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
#endif
  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, website_name, WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/on", HTTP_GET, handleOn);
  server.on("/off", HTTP_GET, handleOff);
  server.on("/startslew", HTTP_GET, handleSlewRequest);
  server.on("/stopslew", HTTP_GET, handleSlewOff);
  server.on("/setCurrent", HTTP_GET, handleSetCurrent);
  server.on("/readPreset", HTTP_GET, handleGetPresetExposureSettings);
  server.on("/abort", HTTP_GET, handleAbortCapture);
  server.on("/status", HTTP_GET, handleStatusRequest);
  server.on("/version", HTTP_GET, handleVersion);

  // Start the server
  server.begin();

#ifdef AP
  Serial.println(WiFi.softAPIP());
#else
  Serial.println(WiFi.localIP());
#endif
  pinMode(INTERV_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(AXIS1_STEP, OUTPUT);
  pinMode(AXIS1_DIR, OUTPUT);
  pinMode(EN12_n, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  digitalWrite(AXIS1_STEP, LOW);
  digitalWrite(EN12_n, LOW);
  //handleExposureSettings();
}

void loop() {
  if (ra_axis.slewActive) {
    //blink status led if mount is in slew mode
    if (millis() - blink_millis >= 150) {
      digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
      blink_millis = millis();
    }
  } else {
    //turn on status led if sidereal tracking is ON
    digitalWrite(STATUS_LED, ra_axis.trackingActive);
  }

  if (intervalometer.intervalometerActive) {
    intervalometer.run();
  }

  server.handleClient();
  dnsServer.processNextRequest();
}