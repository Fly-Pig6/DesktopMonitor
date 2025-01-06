#include <TFT_eSPI.h>
#include "XBM.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

const char* ssid = "<WiFi名字>";
const char* password = "<WiFi密码>";
const char* host = "<电脑的IPv4地址>";
const int httpPort = 8085;  // Open Hardware Monitor上提供的端口

#define UPDATE_INTERVAL_S 2
TFT_eSPI tft = TFT_eSPI();

unsigned long targetTime = 0;

String HOST_NAME = "";
String PROCESSOR_NAME = "";
String PROCESSOR_PW = "";
String PROCESSOR_TEMP = "";
String MEM_AVAILABLE = "";
int CPU_PERCENT = 0;
int MEM_PERCENT = 0;

void drawProgressbar(int n, int x, int y) {
  tft.fillRect(x, y, 100, 14, 0x630C);  // 白色
  tft.fillRect(x, y, n, 14, 0xF79E);    // 灰色
  tft.setTextSize(2);tft.setTextDatum(TC_DATUM);
  tft.setTextColor(0xB596);             // 介于白色灰色
  tft.drawString(String(n) + "%", x + 50, y);
}

void updateScreen() {
  tft.drawXBitmap(5, 10, CPU, xbmWidth, xbmHeight, 0x630C);
  tft.setTextSize(2);tft.setTextDatum(TL_DATUM);
  tft.setTextColor(0x630C);
  int w = tft.drawString("CPU: ", 110, 10);
  drawProgressbar(CPU_PERCENT, w + 120, 10);
  tft.fillRect(110, 30, 210, 60, TFT_BLACK);  // 覆盖旧背景
  tft.setTextSize(2);tft.setTextDatum(TL_DATUM);
  tft.setTextColor(0x630C);
  tft.drawString(String("Temp:") + PROCESSOR_TEMP, 110, 30);
  tft.drawString(String("PW:") + PROCESSOR_PW, 110, 50);
  tft.drawString(PROCESSOR_NAME, 110, 70);

  tft.fillRect(130, 90, 20, 10, 0xF800);
  tft.fillRect(150, 90, 20, 10, 0x07E0);
  tft.fillRect(170, 90, 20, 10, 0x001F);

  tft.drawXBitmap(5, 130, SYS, xbmWidth, xbmHeight, 0x630C);
  tft.setTextSize(2);tft.setTextDatum(TL_DATUM);
  tft.setTextColor(0x630C);
  w = tft.drawString("RAM: ", 110, 130);
  drawProgressbar(MEM_PERCENT, w + 120, 130);
  tft.fillRect(110, 150, 210, 60, TFT_BLACK);  // 覆盖旧背景
  tft.setTextSize(2);tft.setTextDatum(TL_DATUM);
  tft.setTextColor(0x630C);
  tft.drawString(String("Free:") + MEM_AVAILABLE, 110, 150);
  tft.drawString(HOST_NAME, 110, 170);
  tft.drawString(host, 110, 190);
}

void getData() {
  NetworkClient client;
  while (!client.connect(host, httpPort)) {
    Serial.println("Connection failed !");
    delay(10);
  }
  client.println("GET /data.json HTTP/1.1");
  client.println("Host: " + String(host));
  client.println("Connection: close\r\n");

  unsigned long timeout = millis();
  while (!client.available()) {
    if (millis() - timeout > 5000) {
      Serial.println("Response timeout !");
      client.stop();
      return;
    }
  }

  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println("Invalid Response !");
    client.stop();
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.println("deserializeJson() failed !");
    client.stop();
    return;
  }

  HOST_NAME = doc["Children"][0]["Text"].as<String>();  // 电脑名称
  PROCESSOR_NAME = doc["Children"][0]["Children"][1]["Text"].as<String>();  // CPU名称
  PROCESSOR_PW = doc["Children"][0]["Children"][1]["Children"][3]["Children"][0]["Value"].as<String>();  // CPU功耗
  PROCESSOR_TEMP = doc["Children"][0]["Children"][1]["Children"][1]["Children"][0]["Value"].as<String>();  // CPU温度
  MEM_AVAILABLE = doc["Children"][0]["Children"][2]["Children"][1]["Children"][1]["Value"].as<String>();  // 可用内存
  String t = doc["Children"][0]["Children"][1]["Children"][2]["Children"][0]["Value"];    // CPU占用
  CPU_PERCENT = t.toInt();
  t = doc["Children"][0]["Children"][2]["Children"][0]["Children"][0]["Value"].as<String>(); // 内存占用
  MEM_PERCENT = t.toInt();

  client.stop();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(1500);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  if (targetTime < millis()) {
    getData();
    updateScreen();
    targetTime = millis() + UPDATE_INTERVAL_S * 1000;
  }
}
















