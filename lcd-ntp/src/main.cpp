#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <AsyncFsWebServer.h>
#include <LiquidCrystal_Base.h>

#include "serialDebug.h"
#include "states.h"

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3" // todo change to const char ptr
#include <time.h>

struct tm ntpTime;

uint8_t state = STATE_START;
uint32_t now = 0;
AsyncFsWebServer server(80, LittleFS, "esphost");
// LiquidCrystal_I2C lcd(0x26,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void initPinsApp()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
}

// // change type func
// void initI2CLCD()
// {
//   // TODO implement I2C LCD init
//   lcd.init();                      // initialize the lcd 
//   lcd.backlight();
//   lcd.clear();
//   lcd.setCursor(0, 0);
// }
////////////////////////////////  Filesystem  /////////////////////////////////////////
bool startFilesystem()
{
  if (LittleFS.begin())
  {
    server.printFileList(LittleFS, "/", 2);
    return true;
  }
  else
  {
    DEBUG_PRINT("ERROR on mounting filesystem. It will be formmatted!");
    // LittleFS.format();
    // ESP.restart();
  }
  return false;
}

void setup()
{

  bool initialized = false;
  while (!initialized)
  {
    switch (state)
    {
    case STATE_START:
      state = STATE_INIT_PINS;
      break;

    case STATE_INIT_PINS:
      initPinsApp();
      state = STATE_SERIAL_INIT;
      break;

    case STATE_SERIAL_INIT:
      initSerial();
      state = STATE_INIT_FS; //STATE_INIT_I2C_LCD;
      break;
    
    // case STATE_INIT_I2C_LCD:
    //   initI2CLCD();
    //   state = STATE_INIT_FS;
    //   break;

    case STATE_INIT_FS:
      DEBUG_PRINT("State is 1");
      if (!startFilesystem())
      {
        DEBUG_PRINT("LittleFS Mount Failed");
        LittleFS.format();
        state = STATE_ERROR;
      }
      else
      {
        DEBUG_PRINT("LittleFS Mount OK");
        state = STATE_INIT_WEB_SERVER;
      }
      break;

    case STATE_INIT_WEB_SERVER:
      DEBUG_PRINT("Init Web FS Server");
      // Try to connect to WiFi (will start AP if not connected after timeout)
      if (!server.startWiFi(10000))
      {
        DEBUG_PRINT("\nWiFi not connected! Starting AP mode...");
        server.startCaptivePortal("ESP_FS_WEB", "123456789", "/setup");
      }

      // Enable ACE FS file web editor and add FS info callback fucntion
      server.enableFsCodeEditor();

      // Start server
      server.init();

      // Serial.print(F("Async ESP Web Server started on IP Address: "));
      DEBUG_PRINT(server.getServerIP());

// Set NTP servers
      configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");

      // Wait for NTP sync (with timeout)
      getLocalTime(&ntpTime, 5000);

      state = STATE_READ_FS_CONFIG;
      break;

    case STATE_READ_FS_CONFIG:
      DEBUG_PRINT("Reading FS Config file");
      // File configFile = server.getConfigFile("r");
      // if (!configFile)
      // {
        // LittleFS.open("/config/config.json", "w").close(); // create empty config file
      // }s
      //configFile.close();

      state = STATE_READY;
      break;

    case STATE_READY:
      DEBUG_PRINT("State is 2");
      initialized = true;
      break;

    case STATE_ERROR:
      DEBUG_PRINT("ERROR");
      delay(100000);
      ESP.restart();
      break;
    default:
      DEBUG_PRINT("Unknown State");
      break;
    }
  }

  DEBUG_PRINT("INIT DONE");
  now = millis();
}

void loop()
{
  server.updateDNS();

  // if wifi connected -> get time from NTP server every 4 secs -> update CSV log file + update LCD
  
}
