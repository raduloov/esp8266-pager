#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
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

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
LiquidCrystal_I2C lcd(0x27, 16, 2);

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

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>esp8266 Pager</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style>
      html {
        font-family: Arial, Helvetica, sans-serif;
        text-align: center;
      }
      h1 {
        font-size: 1.8rem;
        color: white;
      }
      h2 {
        font-size: 1.5rem;
        font-weight: bold;
        color: #143642;
      }
      .topnav {
        overflow: hidden;
        background-color: #143642;
      }
      body {
        margin: 0;
      }
      .content {
        padding: 30px;
        max-width: 600px;
        margin: 0 auto;
      }
      .card {
        background-color: #f8f7f9;
        box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, 0.5);
        padding-top: 10px;
        padding-bottom: 20px;
      }
      .button {
        padding: 15px 50px;
        font-size: 24px;
        text-align: center;
        outline: none;
        color: #fff;
        background-color: #0f8b8d;
        border: none;
        border-radius: 5px;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(0, 0, 0, 0);
      }
      /*.button:hover {background-color: #0f8b8d}*/
      .button:active {
        background-color: #0f8b8d;
        box-shadow: 2 2px #cdcdcd;
        transform: translateY(2px);
      }
      .state {
        font-size: 1.5rem;
        color: #8c8c8c;
        font-weight: bold;
      }
    </style>
  </head>
  <body>
    <div class="topnav">
      <h1>esp8266 Pager</h1>
    </div>
    <div class="content">
      <div class="card">
        <input type="input" id="input" placeholder="Type a message" />
        <p><button id="button" class="button">Send message</button></p>
      </div>
    </div>
    <script>
      const gateway = `ws://${window.location.hostname}/ws`;
      let websocket;
      window.addEventListener("load", onLoad);
      function initWebSocket() {
        console.log("Trying to open a WebSocket connection...");
        websocket = new WebSocket(gateway);
        websocket.onopen = onOpen;
        websocket.onclose = onClose;
      }
      function onOpen(event) {
        console.log("Connection opened");
      }
      function onClose(event) {
        console.log("Connection closed");
        setTimeout(initWebSocket, 2000);
      }
      function onLoad(event) {
        initWebSocket();
        sendButton();
      }
      function sendButton() {
        document.getElementById("button").addEventListener("click", e => sendNewMessage(e));
      }
      function sendNewMessage(e) {
        const inputValue = document.getElementById("input").value;
        console.log("Input Value:", inputValue);
        websocket.send(inputValue);
      }
    </script>
  </body>
</html>
)rawliteral";

void notifyClients()
{
  ws.textAll("1");
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;

    detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));

    toggleLCDBacklight(true);

    String newMessage = String((char*)data);
    lcd.clear();
    if (newMessage.length() > 16) {
      lcd.setCursor(0, 0);
      lcd.print(newMessage.substring(0, 16));
      lcd.setCursor(0, 1);
      lcd.print(newMessage.substring(16));
    } else {
      lcd.setCursor(0, 0);
      lcd.print(newMessage);
    }

    playNotificationSound();

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonReleasedInterrupt, FALLING);

    notifyClients();
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
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

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
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
    delay(1000);
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

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send_P(200, "text/html", index_html);
  });

  server.begin();
}

void loop()
{
  ws.cleanupClients();
  
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