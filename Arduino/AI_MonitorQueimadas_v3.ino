#include <WiFi.h>
#include <Json.h>
#include <Delay.h>
#include <Esp32InternalRTC.h>
#include <NeoPixelChanger.h>
#include <FS.h>
#include <FFat.h>
#include <Http.h>

#define FILE_PATH "/WeatherData.json"

String ssid = "D-Link";
String password =  "01234567";
int nextThrigger = -1;
Delay oneSecond(INTERVAL_1S);
Esp32InternalRTC rtc;
NeoPixelChanger led;
Http http;

typedef struct {
  String city;
  String coordinates;
  int daysWithoutRain;
  double maximumPrecipitation;
} cities;

cities coordinates[30] = {
  {"Salitre", "-7.285748,-40.457514", 0, 0}, {"Campo Sales", "-7.075103,-40.372339", 0, 0}, {"Araripe", "-7.211309,-40.138323", 0, 0},
  {"Potengi", "-7.091934,-40.027603", 0, 0}, {"Assare", "-6.870889,-39.871030", 0, 0}, {"Antonia do Norte", "-6.775348,-39.988188", 0, 0},
  {"Tarrafas", "-6.684036,-39.758108", 0, 0}, {"Altaneira", "-6.998939,-39.738878", 0, 0}, {"Nova Olinda", "-7.092372,-39.678686", 0, 0},
  {"Santana do Cariri", "-7.185914,-39.737159", 0, 0}, {"Farias Brito", "-6.928198,-39.571438", 0, 0}, {"Crato", "-7.231216,-39.410477", 0, 0},
  {"Juazeiro do Norte", "-7.228166,-39.312093", 0, 0}, {"Barbalha", "-7.288738,-39.299320", 0, 0}, {"Jardim", "-7.586031,-39.279563", 0, 0},
  {"Porteiras", "-7.534501,-39.116856", 0, 0}, {"Penaforte", "-7.830278,-39.072340", 0, 0}, {"Jati", "-7.688990,-39.005227", 0, 0},
  {"Brejo Santo", "-7.488500,-38.987459", 0, 0}, {"Abaiara", "-7.349389,-39.033383", 0, 0}, {"Milagres", "-7.310940,-38.938627", 0, 0},
  {"Mauriti", "-7.382958,-38.771900", 0, 0}, {"Barro", "-7.174146,-38.779534", 0, 0}, {"Caririacu", "-7.042127,-39.285435", 0, 0},
  {"Granjeiro", "-6.887292,-39.220469", 0, 0}, {"Aurora", "-6.943031,-38.969761", 0, 0}, {"Lavras da Mangabeira", "-6.752719,-38.965939", 0, 0},
  {"Ipaumirim", "-6.789527,-38.718022", 0, 0}, {"Baixio", "-6.730631,-38.716898", 0, 0}, {"Umari", "-6.644247,-38.699599", 0, 0}
};

void freezes() {
  while (1) {
    neopixelWrite(RGB_BUILTIN, 255, 0, 0);
    delay(250);
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    delay(250);
  }
}

void setFileContent(String path, String content) {
  File file = FFat.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Write Error: Failed to open file!");
    freezes();
    return;
  }
  file.print(content.c_str());
  file.close();
}

String getFileContent(String path) {
  String s;
  if (!FFat.exists(path)) return s;
  File file = FFat.open(path.c_str());
  if (!file) {
    Serial.println("Read Error: Failed to open file!");
    freezes();
    return s;
  }

  while (file.available()) s += (char) file.read();
  file.close();
  return s;
}

void deleteFile(String path) {
  FFat.remove(path.c_str());
}

void saveData() {
  JsonArray data = JsonArray();
  for (int i = 0; i < 30; i++) {
    JsonObject jsonObject = JsonObject();
    jsonObject.put("city", coordinates[i].city);
    jsonObject.put("days_without_rain", coordinates[i].daysWithoutRain);
    jsonObject.put("maximum_precipitation", coordinates[i].maximumPrecipitation);
    data.put(jsonObject);
  }
  setFileContent(FILE_PATH, data.toString());
  if (FFat.exists(FILE_PATH)) Serial.println("File written.");
  else Serial.println("File not written!");
}

void readData() {
  String content = getFileContent(FILE_PATH);
  if (!content.length()) return;
  JsonArray jsonArray = JsonArray(content);
  for (int i = 0; i < 30; i++) {
    JsonObject jsonObject = jsonArray.getJsonObject(i);
    coordinates[i].city = jsonObject.getString("city");
    coordinates[i].daysWithoutRain = jsonObject.getInt("days_without_rain");
    coordinates[i].maximumPrecipitation = jsonObject.getDouble("maximum_precipitation");
  }
  Serial.println("Load data complete.");
}

void printData() {
  String content = getFileContent(FILE_PATH);
  if (!content.length()) {
    Serial.println("Data file not exists.");
    return;
  }
  JsonArray jsonArray = JsonArray(content);
  for (int i = 0; i < 30; i++) {
    JsonObject jsonObject = jsonArray.getJsonObject(i);
    Serial.println(jsonObject.toString());
  }
}

void updateWeather() {
  JsonArray jsonArray;
  for (int i = 0; i < 30; i++) {
    Serial.print(i + 1);
    Serial.print(" - Requesting weather for ");
    Serial.print(coordinates[i].city);
    Serial.print("...");
    String url = "https://api.weatherapi.com/v1/current.json?key=a8856d705d3b4b17b25151225240605&q=" + coordinates[i].coordinates + "&aqi=yes";
    String content = http.get(url);
    if (!content.length()) {
      Serial.println(" Fatal error on data dowbload.");
      return;
    }

    JsonObject json(content);
    if (!json.contains("location")) {
      Serial.println(" Invalid server response->" + content);
      return;
    }
    Serial.println(" Sucessful.");

    JsonObject current = json.getJsonObject("current");
    double precipitation = current.getDouble("precip_mm");
    if (precipitation >= 1.0) {
      coordinates[i].daysWithoutRain = 0;
      if (precipitation > coordinates[i].maximumPrecipitation) coordinates[i].maximumPrecipitation = precipitation;
    }

    JsonObject jsonItem;
    jsonItem.put("city", coordinates[i].city);
    jsonItem.put("timestamp", current.getLong("last_updated_epoch"));
    jsonItem.put("date_time", current.getString("last_updated"));
    jsonItem.put("temperature", current.getDouble("temp_c"));
    jsonItem.put("humidity", current.getInt("humidity"));
    jsonItem.put("precipitation", current.getDouble("precip_mm"));
    jsonItem.put("cloud", current.getInt("cloud"));
    jsonItem.put("carbon_monoxide", current.getJsonObject("air_quality").getDouble("co"));
    jsonItem.put("days_without_rain", coordinates[i].daysWithoutRain);
    jsonArray.put(jsonItem);

    delay(1000);
  }
  if (!jsonArray.length()) {
    Serial.print(" Error: Nothing to send!");
    return;
  }
  Serial.print(" Sending to backend...");
  http.post("https://secretamensagem.000webhostapp.com/monitor_queimadas_cariri/weather/weather.php", jsonArray.toString());
  if (!http.getResponseCode()) {
    Serial.println(" Fatal fail on send to back-end.");
    return;
  }
  Serial.println(" Successful.");
}

void incrementDays() {
  for (int i = 0; i < 30; i++) {
    if (coordinates[i].maximumPrecipitation == 0) coordinates[i].daysWithoutRain++;
    coordinates[i].maximumPrecipitation = 0;
  }
}

void updateForecast() {
  for (int i = 0; i < 30; i++) {
    Serial.print(i + 1);
    Serial.print(" - Requesting forecast for ");
    Serial.print(coordinates[i].city);
    Serial.print("...");
    String url = "https://api.weatherapi.com/v1/forecast.json?key=a8856d705d3b4b17b25151225240605&q=" + coordinates[i].coordinates + "&aqi=yes";
    String content = http.get(url);
    if (!content.length()) {
      Serial.println(" Fatal error on data dowbload.");
      return;
    }

    JsonObject json(content);
    if (!json.contains("forecast")) {
      Serial.println(" Invalid server response->" + content);
      return;
    }
    Serial.print(" Sucessful.");
    //Serial.println("data->" + content);
    JsonArray forecast = json.getJsonObject("forecast").getJsonArray("forecastday").getJsonObject(0).getJsonArray("hour");
    JsonArray jsonHours;
    for (int a = 0; a < forecast.length(); a++) {
      JsonObject jsonForecastHour = forecast.getJsonObject(a);
      JsonObject jsonHour;
      jsonHour.put("timestamp", jsonForecastHour.getLong("time_epoch"));
      jsonHour.put("date_time", jsonForecastHour.getString("time"));
      jsonHour.put("temperature", jsonForecastHour.getDouble("temp_c"));
      jsonHour.put("humidity", jsonForecastHour.getInt("humidity"));
      jsonHour.put("uv_index", jsonForecastHour.getInt("uv"));
      jsonHour.put("probability", calculateProbability(jsonForecastHour.getDouble("temp_c"), jsonForecastHour.getInt("humidity"), coordinates[i].daysWithoutRain, jsonForecastHour.getInt("uv")));
      jsonHours.put(jsonHour);
    }
    JsonObject jsonCity;
    jsonCity.put("city", coordinates[0].city);
    jsonCity.put("probabilities", jsonHours);
    //Serial.println("data->" + jsonCity.toString());

    delay(1000);
    Serial.print(" Sending to backend...");
    http.post("https://secretamensagem.000webhostapp.com/monitor_queimadas_cariri/weather/arduino_probabilities.php", jsonCity.toString());
    if (!http.getResponseCode()) {
      Serial.println(" Fatal fail on send to back-end.");
      return;
    }
    Serial.println(" Successful.");
    delay(1000);
  }
}

int calculateProbability(double temperature, int humidity, int daysWithoutRain, int uvIndex) {
  int valueTemperature = max(temperature - 30, 1.0); // consider using only high temperatures over 30 degrees
  int valueHumidity = 100 - humidity; // inversely proportional
  int weightTemperatureHumidity = ((valueTemperature * valueHumidity) / 1000) * uvIndex;
  return min((int) (pow(weightTemperatureHumidity, 3)) * max(1, daysWithoutRain), 100);
}

void wifiConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi connecting...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    while (WiFi.status() != WL_CONNECTED) {
      neopixelWrite(RGB_BUILTIN, 255, 255, 0);
      delay(250);
      neopixelWrite(RGB_BUILTIN, 255, 0, 0);
      delay(250);
      Serial.print(".");
    }
    neopixelWrite(RGB_BUILTIN, 0, 255, 0);
    Serial.println(" Connected.");
  }
}

void setup() {
  Serial.begin(115200);
  neopixelWrite(RGB_BUILTIN, 255, 255, 255);

  //FFat.format();
  if (!FFat.begin()) {
    Serial.println("FFat mount failed!");
    freezes();
    return;
  }
  readData();
  printData();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnect();
    if (!rtc.begin()) {
      Serial.println("Internal RTC Sync Fail!");
      while (1);
    }
    Serial.println("Internal RTC Initialization OK.");

    struct rtc_time time = rtc.getTime();
    int minute = time.minute;
    if (minute < 5) nextThrigger = 5;
    else if (minute < 20) nextThrigger = 20;
    else if (minute < 35) nextThrigger = 35;
    else if (minute < 50) nextThrigger = 50;
    else nextThrigger = 5;
    delay(1000);
    neopixelWrite(RGB_BUILTIN, 0, 0, 255);
    updateWeather();
    saveData();
    delay(1000);
    updateForecast();
    neopixelWrite(RGB_BUILTIN, 0, 255, 0);
    Serial.println("Next thrigger: " + String(time.hour) + ":" + (nextThrigger > 9 ? String(nextThrigger) : "05"));
  } else {
    if (oneSecond.gate()) {
      oneSecond.reset();
      struct rtc_time time = rtc.getTime();

      if (time.minute == nextThrigger) {
        nextThrigger += 15;
        if (nextThrigger == 65) nextThrigger = 5;
        neopixelWrite(RGB_BUILTIN, 0, 0, 255);
        updateWeather();
        saveData();
        neopixelWrite(RGB_BUILTIN, 0, 255, 0);
        Serial.println("Next thrigger: " + String(time.hour) + ":" + (nextThrigger > 9 ? String(nextThrigger) : "05"));
      } else if (time.hour == 0 && time.minute == 0 && time.second == 0) {
        delay(1000);
        neopixelWrite(RGB_BUILTIN, 0, 0, 255);
        incrementDays();
        saveData();
        neopixelWrite(RGB_BUILTIN, 0, 255, 0);
      } else if (time.hour == 0 && time.minute == 30 && time.second == 0) {
        delay(1000);
        neopixelWrite(RGB_BUILTIN, 0, 0, 255);
        updateForecast();
        neopixelWrite(RGB_BUILTIN, 0, 255, 0);
      }
    } else {
      led.setColor(0, 255, 0);
      led.compute();
    }
  }
}
