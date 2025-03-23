#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BUZZER_PIN D5
#define BUTTON_PIN D4

ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_pass";

unsigned long lastTimeLCDBacklightOn = millis();
unsigned long LCDTimeOn = 10000;

volatile unsigned long lastTimeButtonReleased = millis();
unsigned long buttonDebounceDelay = 50;

bool isBacklightOn = true;
volatile bool isButtonReleased = false;

void handleRoot();
void handleNewMessage();

void initPager()
{
  WiFi.begin(ssid, password);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Connecting");
  lcd.setCursor(0, 1);
  lcd.print("to WiFi ...");

  while (WiFi.status() != WL_CONNECTED)
  {
    // wait until connected to wifi
    delay(1000);
  }

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

  // not currently working
  if (MDNS.begin("esp8266")) // Start the mDNS responder for esp8266.local
  {              
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/newMessage", HTTP_POST, handleNewMessage);

  server.begin();
  Serial.println("HTTP server started");
}

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

void handleRoot()
{
  server.send(200, "text/html", "<form action=\"/newMessage\" method=\"POST\"><input type=\"text\" name=\"newMessage\" placeholder=\"Some text...\"></br><input type=\"submit\" value=\"Send\"></form>");
}

void handleNewMessage()
{
  detachInterrupt(digitalPinToInterrupt(BUTTON_PIN));

  toggleLCDBacklight(true);

  String newMessage = server.arg("newMessage");
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

  server.send(200, "text/html", "<form action=\"/newMessage\" method=\"POST\"><input type=\"text\" name=\"newMessage\" placeholder=\"Some text...\"></br><input type=\"submit\" value=\"Send\"></form>");

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonReleasedInterrupt, FALLING);
}

void setup()
{
  Serial.begin(9600);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                  buttonReleasedInterrupt,
                  FALLING);

  initPager();
}

void loop()
{
  server.handleClient();

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
