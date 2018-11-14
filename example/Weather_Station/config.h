
#define WIFI_SSID "---------"
#define WIFI_PWD "----------"

const int8_t timeZone = -4;
const int8_t minutesTimeZone = 0;
static const char ntpServerName[] = "us.pool.ntp.org";

String TEMP_UNIT = "F"; // the unit of temperature, 'C' or 'F'.
String WEATHER_CITY = "newyork,us";
String DISPLAY_CITY = "Newyork";
const int WEATHER_UPDATE_INTERVAL = 10 * 60; //update per 10 minutes.
long checkWeaterDueTime;

const String WEEK[7] = {"Sun.", "Mon.", "Tue.", "Wed.", "Thu.", "Fri.", "Sat."};
const String WEEKUPPER[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const String MONTH[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

#define TFT_RST D4
#define TFT_RS  D2
#define TFT_CS  D8  // SS
#define TFT_SDI D7  // MOSI
#define TFT_CLK D5  // SCK
#define TFT_LED D1   // 0 if wired to +5V directly
#define TFT_BRIGHTNESS 1023

typedef struct currentWeatherStruct {
  const char* country;
  const char* city;
  const char* lon;
  const char* lat;
  const char* code;
  const char* text;
  const char* temp;
  const char* pressure;
  const char* humidity;
  const char* visibility;
  const char* windSpeed;
  const char* windDeg;
  const char* sunrise;
  const char* sunset;
} currentWeatherStruct;

typedef struct forecastWeatherStruct {
  const char* code;
  const char* date;
  const char* day;
  const char* tempMax;
  const char* tempMin;
  const char* text;
} forecastWeatherStruct;

