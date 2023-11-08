/* sem7 IoT project: Home Automation
- led light
- backyard movement detection
- temperature, humidity
- door: ultrasonic

send: temperature, humidity, led light button, pir, ultrasonic
receive: light state
*/

#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "WebSocketsServer.h"
#include "ESPmDNS.h"
#include "DHT.h"
#include "ESP32Servo.h"

const char* ssid = "LAPTOP101";
const char* password = "88888888";
const char* hostname = "home";

const int touch_threshold = 40;

const char* rooms[] = {"Bedroom 1", "Bathroom", "Bedroom 2", "Kitchen", "Dining", "Living"};
bool lights[] = {true,true,true,true,true,true};
int light_pins[] = {19, 18, 21, 16, 17, 22};
int light_touch_pins[] = {13, 12, 14, 27, 33, 32};
bool light_touch_bool[] = {true, true, true, true, true, true};

bool door = false;
bool door_touch_bool = true;
const int door_touch_pin = 4;
const int door_servo_pin = 23;

bool pir_pin_bool = true;
const int pir_pin = 34;
const int dht_pin = 26;


DHT dht(dht_pin, DHT11);
Servo servo;

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected from %s\n", num, webSocket.remoteIP(num).toString().c_str());
      break;
    case WStype_TEXT:
      Serial.printf("[%u] Received text: %s\n", num, payload);
      if (strcmp((char*)payload, "toggle") == 0) {
        Serial.println("toggle");
      }
      for(int i=0; i<sizeof(rooms)/sizeof(*rooms); i++){
        if (strcmp((char*)payload, rooms[i]) == 0) {
          Serial.println(rooms[i]);
          lights[i] = !lights[i];
          digitalWrite(light_pins[i], lights[i]);
          Serial.println(lights[i]);
          String jsonString = "{\"light\": \"" + String(rooms[i]) + "\"}";
          webSocket.broadcastTXT(jsonString);
        }
      }
      if (strcmp((char*)payload, "door") == 0) {
        Serial.println("Door");
        door = !door;
        servo.write(int(door) * 90);
        String jsonString = "{\"door\": " + String(door) + "}";
        webSocket.broadcastTXT(jsonString);
      }
      break;
  }
}

void setup_spiffs_wifi_dns() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi. IP address: " + WiFi.localIP().toString());

  if (!MDNS.begin(hostname)) {
    Serial.println("Error setting up mDNS");
  }
  Serial.print("mDNS started. Hostname: ");
  Serial.println(hostname);
}

void setup_async_server() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Route to load style.css file
  server.on("/bootstrap_min.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/bootstrap_min.css", "text/css");
  });

  server.on("/bootstrap_min.js", HTTP_GET, [](AsyncWebServerRequest *request){   
    request->send(SPIFFS, "/bootstrap_min.js", "text/javascript");
  });

  server.on("/chart.js", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(SPIFFS, "/chart.js", "text/javascript");
  });

  server.on("/websocket_script.js", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(SPIFFS, "/websocket_script.js", "text/javascript");
  });

  server.on("/images/esp32_1.jpg", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(SPIFFS, "/images/esp32_1.jpg", "image/jpeg");
  });
  server.on("/images/esp32_2.jpg", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(SPIFFS, "/images/esp32_2.jpg", "image/jpeg");
  });
  server.on("/images/esp32_macro.jpg", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(SPIFFS, "/images/esp32_macro.jpg", "image/jpeg");
  });
}

void setup(){
  Serial.begin(115200);

  setup_spiffs_wifi_dns();
  setup_async_server();

  for(int i=0; i<sizeof(light_pins)/sizeof(*light_pins); i++) {
    pinMode(light_pins[i], OUTPUT);
    digitalWrite(light_pins[i], HIGH);
  }
  pinMode(door_servo_pin, OUTPUT);
  pinMode(pir_pin, INPUT);
  pinMode(dht_pin, INPUT);

  dht.begin();
  servo.attach(door_servo_pin);

  server.begin();
  webSocket.begin();

  webSocket.onEvent(onWebSocketEvent);
}


bool touch_button_read(int pin, bool *b) {
  if((touchRead(pin) < touch_threshold) && *b) {
    *b = false;
    return true;
  }
  if(touchRead(pin) > touch_threshold) {
    *b = true;
  }
  return false;
}

bool pir_read(int pin, bool *b) {
  if(digitalRead(pin) && *b) {
    *b = false;
    return true;
  }
  if(!digitalRead(pin)) {
    *b = true;
  }
  return false;
}

unsigned long lastExecutedMillis_1 = 0;
unsigned long lastExecutedMillis_2 = 0;
unsigned long currentMillis = 0;

void loop(){
  currentMillis = millis();

  if(currentMillis - lastExecutedMillis_1 >= 50) {
    
    for(int i=0; i<sizeof(light_pins)/sizeof(*light_pins); i++) {
      if(touch_button_read(light_touch_pins[i], &light_touch_bool[i])) {
        lights[i] = !lights[i];
        digitalWrite(light_pins[i], lights[i]);
        String jsonString = "{\"light\": \"" + String(rooms[i]) + "\"}";
        webSocket.broadcastTXT(jsonString);
      }
    }

    if(pir_read(pir_pin, &pir_pin_bool)) {
      String jsonString = "{\"pir\": 1}";
      webSocket.broadcastTXT(jsonString);
    }

    if(touch_button_read(door_touch_pin, &door_touch_bool)) {
      Serial.println("door touch pin");
      door = !door;
      servo.write(int(door) * 90);
      String jsonString = "{\"door\": " + String(door) + "}";
      webSocket.broadcastTXT(jsonString);
    }

    lastExecutedMillis_1 = currentMillis;
  }

  if(currentMillis - lastExecutedMillis_2 >= 2000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if(!isnan(t) && ! isnan(h)) {
      String jsonString = "{\"dht\": [" + String(t) + "," + String(h) + "]}";
      webSocket.broadcastTXT(jsonString);
    }

    lastExecutedMillis_2 = currentMillis;
  }

  webSocket.loop();
}