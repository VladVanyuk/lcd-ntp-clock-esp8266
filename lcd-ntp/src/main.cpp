#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <AsyncFsWebServer.h>
#include <LiquidCrystal_Base.h>
#include <time.h>

#include <user_interface.h>

#include "serialDebug.h"
#include "states.h"
#include "configNames.h"

// Timezone definition to get properly time from NTP server
// #define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3" // todo change to const char ptr

struct tm ntpTime;

uint8_t state = STATE_START;
uint32_t now = 0;
AsyncFsWebServer server(80, LittleFS, "esphost");
LiquidCrystal_I2C lcd(0x26,16,2);  // set the LCD address to 0x26 for a 16 chars and 2 line display


void initPinsApp()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
}

// change type func
void initI2CLCD()
{
  // TODO implement I2C LCD init
  Wire.begin();
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);

  lcd.print("LCD I2C Init");
}
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



////////////////////  Load application options from filesystem  ////////////////////
bool loadOptions() {
  if (LittleFS.exists(server.getConfiFileName())) 
  {
    server.getOptionValue(SSID_NAME, WIFI_BOX_SSID);
    server.getOptionValue(PASSWORD_NAME, WIFI_BOX_PASSWORD);
    server.getOptionValue(NTP_NAME, NTP_SERVER);

#if (DEBUG == 1)
    DEBUG_PRINT("\nThis are the current values stored: \n");
    Serial.printf("SSID: %s\n", WIFI_BOX_SSID.c_str());
    Serial.printf("Password: %s\n\n", WIFI_BOX_PASSWORD.c_str());
    Serial.printf("NTP Server: %s\n\n", NTP_SERVER.c_str());
#endif
    return true;
  }
  else
  {
    DEBUG_PRINT(F("Config file not exist"));
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
      state = STATE_INIT_I2C_LCD;
      break;
    
    case STATE_INIT_I2C_LCD:
      initI2CLCD();
      state = STATE_INIT_FS;
      lcd.setCursor(0, 1);
      lcd.write('*');
      break;

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
        state = STATE_INIT_CONFIG_FILE;
      }
      lcd.write('*');
      break;


    case STATE_INIT_CONFIG_FILE:
      DEBUG_PRINT("Init Config File");
      if(loadOptions())
      {
        state = STATE_INIT_WEB_SERVER;
      }
      else
      {
        DEBUG_PRINT("Failed to load config file");
        //state = STATE_ERROR;
        server.addOptionBox("My Options");
        server.addOption(SSID_NAME, WIFI_BOX_SSID.c_str());
        server.addOption(PASSWORD_NAME, WIFI_BOX_PASSWORD.c_str());
        server.addOption(NTP_NAME, NTP_SERVER.c_str());
        state = STATE_INIT_WEB_SERVER;
      }
      lcd.write('*');
      break;
     
     
     
     // bool wifi_station_set_config(struct station_config *config); 
    case STATE_INIT_WEB_SERVER:
      DEBUG_PRINT("Init Web FS Server");
      // Try to connect to WiFi (will start AP if not connected after timeout)
      // struct station_config stationConf;
      // //malloc (sizeof(struct station_config));
      // memset(&stationConf, 0, sizeof(stationConf));
      
      // strncpy((char*)stationConf.ssid, WIFI_BOX_SSID.c_str(), sizeof(stationConf.ssid) - 1);
      // strncpy((char*)stationConf.password, WIFI_BOX_PASSWORD.c_str(), sizeof(stationConf.password) - 1);

      // wifi_station_set_config(&stationConf);

      WiFi.begin(WIFI_BOX_SSID, WIFI_BOX_PASSWORD);
     // DEBUG_PRINT("\nConnecting to %s", WIFI_BOX_SSID.c_str());

      if (!server.startWiFi(10000))
      {
        DEBUG_PRINT("\nWiFi not connected! Starting AP mode...");
        server.startCaptivePortal("ESP_FS_WEB", "123456789", "/setup");
      }

      // Enable ACE FS file web editor and add FS info callback fucntion
      server.enableFsCodeEditor();

        server.addOption(SSID_NAME, WIFI_BOX_SSID.c_str());
        server.addOption(PASSWORD_NAME, WIFI_BOX_PASSWORD.c_str());
        server.addOption(NTP_NAME, NTP_SERVER.c_str());

      // Start server
      server.init();

      // Serial.print(F("Async ESP Web Server started on IP Address: "));
      DEBUG_PRINT(server.getServerIP());

// Set NTP servers
      configTime(NTP_SERVER.c_str(), "time.google.com", "time.windows.com", "pool.ntp.org");

      // Wait for NTP sync (with timeout)
      getLocalTime(&ntpTime, 5000);

      state = STATE_READ_FS_CONFIG;
      lcd.write('*');
      break;

    case STATE_READ_FS_CONFIG:
      DEBUG_PRINT("Reading FS Config file");
    
     
      state = STATE_READY;
      lcd.write('*');
      break;

    case STATE_READY:
      DEBUG_PRINT("State is 2");
      initialized = true;
      lcd.clear();
      lcd.setCursor(0, 0);
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
  now = millis();
  server.updateDNS();

  static uint32_t lcdUpdateTimer = millis();

  // if wifi connected -> get time from NTP server every 4 secs -> update CSV log file + update LCD
  if (WiFi.isConnected() && (millis() - lcdUpdateTimer > 4000))
  {
    lcdUpdateTimer = millis();
    if (getLocalTime(&ntpTime, 1000))
    {
      char timeStringBuff[25];
      strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &ntpTime);
      DEBUG_PRINT(timeStringBuff);

      // Update LCD display
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(timeStringBuff);

      // Append to log file
      File logFile = LittleFS.open("/csv/ntp_log.csv", "a");
      if (logFile)
      {
        logFile.printf("%s\n", timeStringBuff);
        logFile.close();
      }
    }
    else
    {
      DEBUG_PRINT("Failed to obtain NTP time");
    }
  }

}
