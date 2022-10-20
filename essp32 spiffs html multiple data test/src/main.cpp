#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Replace with your network credentials
const char* ssid = "htl-IoT";
const char* password = "iot..2015";

bool ledState = 0;
bool ledState2 = 0;
const int ledPin = 2;
const int ledPin2 = 4;

float temperature = 0;
float humidity = 0;

unsigned long previousMillis = 0;
long interval = 5000;

//BME280 Sensor
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

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

//###########################################################################################
//SETUP
void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin2, LOW);
  //setartup bme280 sensor
  bme.begin(0x76); 

  initSPIFFS();
  initWIFI(ssid, password);
  initWebSocket();
}

void loop() {
  ws.cleanupClients();
  digitalWrite(ledPin, ledState);
  digitalWrite(ledPin2, ledState2);
  
  if(millis() - previousMillis > interval) //messurement Interval 5s
  {
    // save the last time you blinked the LED 
    previousMillis = millis();
    
    humidity = bme.readHumidity();
    temperature = bme.readTemperature();
    notifyClients(); //sends the json data via websockets to the webseite
  }
}


//###########################################################################################
//FUNKTIONS

//Init SPIFFS File System
void initSPIFFS(){
  if(!SPIFFS.begin()){
    Serial.println("Error mounting SPIFFS");
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
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  // On HTTP request for root, provide index.html file
  server.on("/", HTTP_GET, onIndexRequest);
  // On HTTP request for style sheet, provide style.css
  server.on("/style.css", HTTP_GET, onCSSRequest);
  //On HTTP request for java script, provide script.js
  server.on("/script.js", HTTP_GET, onJSRequest);
  // Handle requests for pages that do not exist
  server.onNotFound (onPageNotFound) ;
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
    
    ledState = doc["LED1"];
    Serial.println(ledState);
    ledState2 = doc["LED2"];
    Serial.println(ledState2);
    Serial.println();
    notifyClients();
  }
}

void notifyClients() {
  String json =   "{\"LED1\":";
         json +=  ledState;
         json +=  ", \"LED2\":";
         json +=  ledState2;
         json +=  ", \"TEMPERATURE\":";
         json +=  temperature;
         json +=  ", \"HUMIDITY\":";
         json +=  humidity;
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