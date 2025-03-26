#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BUZZER_PIN D5
#define BUTTON_PIN D4

const char* ssid = "*";
const char* password = "*";

unsigned long lastTimeLCDBacklightOn = millis();
unsigned long LCDTimeOn = 10000;

volatile unsigned long lastTimeButtonReleased = millis();
unsigned long buttonDebounceDelay = 50;

bool isBacklightOn = true;
volatile bool isButtonReleased = false;

const char* websocketServerHost = "esp8266-pager.onrender.com";
const uint16_t websocketServerPort = 443;
const char* websocketServerPath = "/wss";

WebSocketsClient webSocket;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// AsyncWebServer server(80);
// AsyncWebSocket ws("/ws");

ICACHE_RAM_ATTR void buttonReleasedInterrupt()
{
  unsigned long timeNow = millis();
  if (timeNow - lastTimeButtonReleased > buttonDebounceDelay)
  {
    lastTimeButtonReleased = timeNow;
    isButtonReleased = true;
  }
}

void toggleLCDBacklight(bool turnOn)
{
  if (turnOn)
  {
    lcd.backlight();
    isBacklightOn = true;
    lastTimeLCDBacklightOn = millis();
  }
  else
  {
    lcd.noBacklight();
    isBacklightOn = false;
  }
}

void checkLCDBacklight()
{
  unsigned long timeNow = millis();
  if (isBacklightOn && (timeNow - lastTimeLCDBacklightOn > LCDTimeOn))
  {
    toggleLCDBacklight(false);
  }
}

void playNotificationSound()
{
  tone(BUZZER_PIN, 1000, 150);
  delay(200);
  tone(BUZZER_PIN, 1200, 350);
  delay(1000);
}

// void notifyClients()
// {
//   ws.textAll("1");
// }

// void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
// {
//   AwsFrameInfo *info = (AwsFrameInfo*)arg;
//   if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
//   {
//     data[len] = 0;

//     detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));

//     toggleLCDBacklight(true);

//     String newMessage = String((char*)data);
//     lcd.clear();
//     if (newMessage.length() > 16) {
//       lcd.setCursor(0, 0);
//       lcd.print(newMessage.substring(0, 16));
//       lcd.setCursor(0, 1);
//       lcd.print(newMessage.substring(16));
//     } else {
//       lcd.setCursor(0, 0);
//       lcd.print(newMessage);
//     }

//     playNotificationSound();

//     attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonReleasedInterrupt, FALLING);

//     notifyClients();
//   }
// }

// void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
//              void *arg, uint8_t *data, size_t len)
// {
//   switch (type)
//   {
//     case WS_EVT_CONNECT:
//       Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
//       break;
//     case WS_EVT_DISCONNECT:
//       Serial.printf("WebSocket client #%u disconnected\n", client->id());
//       break;
//     case WS_EVT_DATA:
//       handleWebSocketMessage(arg, data, len);
//       break;
//     case WS_EVT_PONG:
//     case WS_EVT_ERROR:
//     break;
//   }
// }

void handleNewMessage(uint8_t *value)
{
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));

  toggleLCDBacklight(true);

  String message = (char*)value;

  lcd.clear();
  if (message.length() > 16)
  {
    lcd.setCursor(0, 0);
    lcd.print(message.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(message.substring(16));
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print(message);
  }

  playNotificationSound();

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonReleasedInterrupt, FALLING);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.println("[WSc] Connected to WebSocket server!");
      // Send a message after connecting
      webSocket.sendTXT("Hello from ESP8266!");
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] Received text: %s\n", payload);
      handleNewMessage(payload);
      break;
    case WStype_BIN:
      Serial.printf("[WSc] Received binary data (%u bytes)\n", payload);
      break;
    default:
      break;
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                  buttonReleasedInterrupt,
                  FALLING);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Connecting");
  lcd.setCursor(0, 1);
  lcd.print("to WiFi ...");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for a");
  lcd.setCursor(0, 1);
  lcd.print("message...");

  webSocket.beginSSL(websocketServerHost, websocketServerPort, websocketServerPath);
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  webSocket.loop();
  
  checkLCDBacklight();

  if (isButtonReleased)
  {
    isButtonReleased = false;
    toggleLCDBacklight(!isBacklightOn);
    if (isBacklightOn)
    {
      lastTimeLCDBacklightOn = millis();
    }
  }
}