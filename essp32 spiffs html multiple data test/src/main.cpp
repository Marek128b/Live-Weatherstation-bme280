#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal_I2C.h>

// Replace with your network credentials
const char* ssid = "htl-IoT";
const char* password = "iot..2015";

float temperature = 0;
float humidity = 0;
float air_pressure = 0;

//Webseite update interval
unsigned long previousMillis = 0;
long interval_web = 1000;

//LCD I2C
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
unsigned long lastMillis = 0;
const int interval = 5000;
unsigned long lastMillis2 = 0;
const int interval2 = 1000;

int displayState = 0;
int countTimeLeft = interval/interval2-1;

//BME280 Sensor
#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//###########################################################################################
//FUNKTION Define
void initSPIFFS();
void initWIFI(const char* ssid,const char* password);
void onIndexRequest(AsyncWebServerRequest *request);
void onCSSRequest(AsyncWebServerRequest *request);
void onJSRequest (AsyncWebServerRequest *request);
void onPageNotFound(AsyncWebServerRequest *request);

void initWebSocket();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void notifyClients();
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len);
String processor(const String& var);

void printValues(int count);
void printValuesAnimation(int interval, int interval2);
void printValuesSerial();
//###########################################################################################
//SETUP
void setup() {
  Serial.begin(115200);
  
  // initialize the lcd 
  lcd.init();                      
  lcd.backlight();

  //setartup bme280 sensor
  bme.begin(0x76); 

  initSPIFFS();
  initWIFI(ssid, password);
  initWebSocket();
}

void loop() {
  ws.cleanupClients();
  printValuesAnimation(interval, interval2);
  if(millis() - previousMillis > interval_web) //messurement Interval 1s to Webseite
  {
    // save the last time you blinked the LED 
    previousMillis = millis();
    
    humidity = bme.readHumidity();
    temperature = bme.readTemperature();
    air_pressure = bme.readPressure() / 100.0F;
    notifyClients(); //sends the json data via websockets to the webseite
  }
}


//###########################################################################################
//FUNKTIONS

//Init SPIFFS File System
void initSPIFFS(){
  if(!SPIFFS.begin()){
    Serial.println("Error mounting SPIFFS");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ERROR SPIFFS");
  }
}

// Callback: send homepage
void onIndexRequest (AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/index.html", "text/html");
}

// Callback: send style sheet
void onCSSRequest (AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/style.css", "text/css");
}

// Callback: send style sheet
void onJSRequest (AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/script.js", "text/js");
}

// callback: send 404 if requested file does not exist
void onPageNotFound (AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client ()->remoteIP();
  Serial.println("[" + remote_ip.toString() + 
                  "] HTTP GET request of " + request->url());
  request->send (404, "text/plain", "Not found");
}

//Init the WIFI
void initWIFI(const char* ssid,const char* password){
  // start WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi ");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());
  delay(10000);

  Serial.println(WiFi.localIP());
  // On HTTP request for root, provide index.html file
  server.on("/", HTTP_GET, onIndexRequest);
  // On HTTP request for style sheet, provide style.css
  server.on("/style.css", HTTP_GET, onCSSRequest);
  //On HTTP request for java script, provide script.js
  server.on("/script.js", HTTP_GET, onJSRequest);
  // Handle requests for pages that do not exist
  server.onNotFound(onPageNotFound);
  // Start web server
  server.begin();
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    
    String message = String((char*)(data));
    Serial.println(message);
    
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, message);

    if(error){
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }
    notifyClients();
  }
}

void notifyClients() {
  String json =   "{\"TEMPERATURE\":";
         json +=  temperature;
         json +=  ", \"HUMIDITY\":";
         json +=  humidity;
         json +=  ", \"PRESSURE\":";
         json +=  air_pressure;
         json +=  "}";
  ws.textAll(json);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        notifyClients();
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  Serial.println(var);
  return String();
}

void printValues(int count){
  if(count == 0){
    lcd.setCursor(0,0);
    lcd.print("Temperature: ");
    lcd.setCursor(0,1);
    lcd.print(bme.readTemperature());
    lcd.print("C");
  }else if(count == 1){
    lcd.setCursor(0,0);
    lcd.print("Humidity: ");
    lcd.setCursor(0,1);
    lcd.print(bme.readHumidity());
    lcd.print("%");
  }else if(count == 2){
    lcd.setCursor(0,0);
    lcd.print("Pressure: ");
    lcd.setCursor(0,1);
    lcd.print(bme.readPressure()/100.0F);
    lcd.print("hPa");
  }else if(count == 3){
    lcd.setCursor(0,0);
    lcd.print("Altitude: ");
    lcd.setCursor(0,1);
    lcd.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    lcd.print("m");
  }
}

void printValuesAnimation(int interval, int interval2){
  if(millis()-lastMillis >= interval){
    lastMillis = millis();
    lcd.clear();
    if(countTimeLeft<=0){
      countTimeLeft = interval/interval2-1;
    }
    printValuesSerial();
    
    displayState++;
    if(displayState>=4){
      displayState = 0;
    }
  }
  if(millis()-lastMillis2 >= interval2){
    lastMillis2 = millis();

    printValues(displayState);
    for(int i = 0; i <= countTimeLeft; i++){
      lcd.setCursor(16-i,1);
      //lcd.write(0xFF);
      lcd.print(".");
    }
    int countTimeLeft2 = (interval/interval2-1)-countTimeLeft;
    for(int i = 0; i <= countTimeLeft2; i++){
      lcd.setCursor(16-(interval/interval2)+i,1);
      lcd.print(" ");
    }

    Serial.println(countTimeLeft);
    countTimeLeft--;
  }
}

void printValuesSerial(){
  Serial.printf("Temperature: %.2fC\n",bme.readTemperature());
  Serial.printf("humidity: %.2f%%\n",bme.readHumidity());
  Serial.printf("Pressure: %.2fhPa\n",bme.readPressure() / 100.0F);
  Serial.printf("Altitude: %.2fm\n",bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println();
}