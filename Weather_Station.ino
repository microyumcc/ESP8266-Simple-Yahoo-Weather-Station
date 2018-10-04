/*
 * Title: Simple Yahoo Weather Station.
 * Author: Microyum
 * Version: 1.0
 * Update: 2018/09/15
 * 
 * please make sure you have install Adafruit_GFX and MCUFRIEND_kbv libraries.
 * TFT_ILI9225_SPI
 * ArduinoJson https://github.com/bblanchon/ArduinoJson
 * Arduino Time Library https://github.com/PaulStoffregen/Time
 * you can download the libraries and copy into arduino's library folder.
 * also you can install using Sketch of arduino's IDE.
 * 
 */
#include "TFT_ILI9225_SPI.h"
#include <avr/pgmspace.h>
#include "weathericon.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

WiFiUDP Udp;
unsigned int localPort = 8888;
WiFiClientSecure httpsclient;

int prevDay = 0;
time_t prevDisplay = 0;

TFT_ILI9225_SPI tft = TFT_ILI9225_SPI(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);

currentWeatherStruct currentWeatherData;
forecastWeatherStruct forecastWeatherData[10];

void setup() { 
  Serial.begin(9600);
  tft.begin();
  tft.fillScreen(COLOR_BLACK);
  tft.drawLine(0, 110, 176, 110, COLOR_DARKGRAY);

  connectWiFi();

  Udp.begin(localPort);

  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  delay(500);
}

void loop() {
  long nowmillis = millis();
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) {
      prevDisplay = now();
      drawClock();
    }
  }

  if (nowmillis >= checkWeaterDueTime) {
    requestYahooWeather();
    drawCurrentWeather();
    drawForecastWeather();
    checkWeaterDueTime = nowmillis + WEATHER_UPDATE_INTERVAL*1000;
  }
}

/*-------WIFI---------*/
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("WiFi Connecting. ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PWD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

/*--------NTP---------*/

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

time_t getNtpTime() {
  IPAddress ntpIPAddress;
  WiFi.hostByName(ntpServerName, ntpIPAddress);
  sendNTPpacket(ntpIPAddress);
  uint32_t beginWait = millis();
  while( millis() - beginWait < 1500 ) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);
      unsigned long secsSince1900;
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  return 0;
}

void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

/*--------Clock---------*/
void drawClock() {
  String T, D, TOMORROW, AFTERTOMORROW;
  if (hour()<10) {
    T += ("0" + String(hour()) + ":");
  } else {
    T += String(hour()) + ":";
  }
  
  if (minute()<10) {
    T += ("0" + String(minute()));
  } else {
    T += String(minute());
  }
  
  tft.setFont(Number20x32, true);
  tft.drawText(35, 30, T, COLOR_WHITE, COLOR_BLACK);

  if(day() != prevDay) {
    prevDay = day();
    D += WEEK[weekday()-1];
    D += " ";
    if (day() < 10) {
      D += ("0" + String(day()));
    } else {
      D += String(day());
    }
    D += " ";
    D += MONTH[month()-1];
    D += " ";
    D += String(year());

    tft.setFont(Terminal8x12);
    tft.drawText(30, 10, D, COLOR_GRAY, COLOR_BLACK);
  }

  tft.setFont(Terminal6x8);
  if (isAM()) {
    tft.drawText(122, 80, "AM.", COLOR_SKYBLUE, COLOR_BLACK);
  } else {
    tft.drawText(122, 80, "PM.", COLOR_SKYBLUE, COLOR_BLACK);
  }

  if (weekday() == 7) {
    TOMORROW = WEEKUPPER[1];
    AFTERTOMORROW = WEEKUPPER[2];
  } else if (weekday() ==6) {
    TOMORROW = WEEKUPPER[7];
    AFTERTOMORROW = WEEKUPPER[1];
  } else {
    TOMORROW = WEEKUPPER[weekday()];
    AFTERTOMORROW = WEEKUPPER[weekday()+1];
  }
  tft.setFont(Terminal8x12);
  tft.drawText(15, 125, "TODAY", COLOR_WHITE, COLOR_BLACK);
  tft.drawText(77, 125, TOMORROW, COLOR_WHITE, COLOR_BLACK);
  tft.drawText(133, 125, AFTERTOMORROW, COLOR_WHITE, COLOR_BLACK);

  tft.setFont(Terminal6x8);
  tft.drawText(140-tft.getTextWidth(DISPLAY_CITY), 70, DISPLAY_CITY, COLOR_SKYBLUE);
  
}

/*--------Weather---------*/

void drawCurrentWeather() {
  tft.fillRectangle(0, 70, 75, 86, COLOR_BLACK);
  const String currentTemp = String(currentWeatherData.temp) + " " + TEMP_UNIT;
  uint16_t currentTempWidth = tft.getTextWidth(currentTemp);
  tft.setFont(Terminal11x16);
  tft.drawText(63-currentTempWidth, 70, currentTemp, COLOR_WHITE, COLOR_BLACK);
  tft.drawCircle(62, 70, 2, COLOR_WHITE);

  tft.drawRGBBitmap(10, 155, getWeatherIcon(currentWeatherData.code), 48, 48);
}

void drawForecastWeather() {
  tft.fillRectangle(0, 140, 176, 152, COLOR_BLACK);
  
  tft.setFont(Terminal8x12);
  tft.drawText(31-tft.getTextWidth(String(forecastWeatherData[0].tempMin)), 140, String(forecastWeatherData[0].tempMin), COLOR_WHITE);
  tft.drawText(85-tft.getTextWidth(String(forecastWeatherData[1].tempMin)), 140, String(forecastWeatherData[1].tempMin), COLOR_WHITE);
  tft.drawText(141-tft.getTextWidth(String(forecastWeatherData[2].tempMin)), 140, String(forecastWeatherData[2].tempMin), COLOR_WHITE);
  tft.drawText(38, 140, String(forecastWeatherData[0].tempMax), COLOR_WHITE);
  tft.drawText(92, 140, String(forecastWeatherData[1].tempMax), COLOR_WHITE);
  tft.drawText(148, 140, String(forecastWeatherData[2].tempMax), COLOR_WHITE);

  tft.drawLine(34, 140, 34, 152, COLOR_WHITE);
  tft.drawLine(88, 140, 88, 152, COLOR_WHITE);
  tft.drawLine(144, 140, 144, 152, COLOR_WHITE);
  
  tft.drawRGBBitmap(65, 155, getWeatherIcon(forecastWeatherData[1].code), 48, 48);
  tft.drawRGBBitmap(120, 155, getWeatherIcon(forecastWeatherData[2].code), 48, 48);
}

void requestYahooWeather() {
  String title = "";
  String headers = "";
  String body = "";
  long now;
  bool finishedHeaders = false;
  bool currentLineIsBlank = true;
  bool gotResponse = false;
  
  char host[] = "query.yahooapis.com";
  
  if (httpsclient.connect(host, 443)) {
    Serial.println("Host connected");
    
    String URL = "/v1/public/yql?q=select%20*%20from%20weather.forecast%20where%20woeid%20in%20(select%20woeid%20from%20geo.places(1)%20where%20text%3D%22"+WEATHER_CITY+"%22)%20and%20u%3D%22"+TEMP_UNIT+"%22&format=json&env=store%3A%2F%2Fdatatables.org%2Falltableswithkeys";
    
    Serial.print("requesting URL: ");
    Serial.println(URL);

    httpsclient.println("GET " + URL + " HTTP/1.1");
    httpsclient.print("Host: "); httpsclient.println(host);
    httpsclient.println("User-Agent: arduino/1.0");
    httpsclient.println("");

    while (httpsclient.connected()) {
      String line = httpsclient.readStringUntil('\n');
      if (finishedHeaders) {
        if (line.startsWith("{\"query\":{\"count\"")) {
          body = body + line;
          break;
        }
      } else {
        if (line == "\r") {
          finishedHeaders = true;
        } else {
          headers = headers + line;
        }
      }
    }
      
    gotResponse = true;
    
    if (gotResponse) {
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(body);
      if (root.success()) {
        if (root.containsKey("query")) {
          String content = root["query"].as<String>();
          JsonObject& results = root["query"]["results"]["channel"];
          currentWeatherData.country = results["location"]["country"];
          currentWeatherData.city =  results["location"]["city"];
          currentWeatherData.lon = results["item"]["long"];
          currentWeatherData.lat = results["item"]["lat"];
          currentWeatherData.code = results["item"]["condition"]["code"];
          currentWeatherData.text = results["item"]["condition"]["text"];
          currentWeatherData.temp = results["item"]["condition"]["temp"];
          currentWeatherData.pressure = results["atmosphere"]["pressure"];
          currentWeatherData.humidity = results["atmosphere"]["humidity"];
          currentWeatherData.visibility = results["atmosphere"]["visibility"];
          currentWeatherData.windSpeed = results["wind"]["speed"];
          currentWeatherData.windDeg = results["wind"]["direction"];
          currentWeatherData.sunrise = results["astronomy"]["sunrise"];
          currentWeatherData.sunset = results["astronomy"]["sunset"];
          for (int i=0; i<10; i++) {
            forecastWeatherData[i].code = results["item"]["forecast"][i]["code"];
            forecastWeatherData[i].date = results["item"]["forecast"][i]["date"];
            forecastWeatherData[i].day = results["item"]["forecast"][i]["day"];
            forecastWeatherData[i].tempMax = results["item"]["forecast"][i]["high"];
            forecastWeatherData[i].tempMin = results["item"]["forecast"][i]["low"];
            forecastWeatherData[i].text = results["item"]["forecast"][i]["text"];
          };
        }
      } else {
        Serial.println("failed to parse JSON");
      }
    }
      
  }
}


