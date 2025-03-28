# esp8266-pager

![IMG_2746-min](https://github.com/user-attachments/assets/d8434821-6b43-43ff-b8af-9790eba6b979)
![IMG_2745-min](https://github.com/user-attachments/assets/6ac1f94b-ea68-4ae6-a0b5-b354ff05ec5e)
<div style="display: flex;">
  <img src="https://github.com/user-attachments/assets/7ad8c4a2-dacb-4b94-9254-99a938c1cdbf" alt="server ui" width="49.5%" />
  <img src="https://github.com/user-attachments/assets/90781dde-d11c-498d-9449-1aff0a90739d" alt="server ui timeout" width="49.5%" />
</div>

## Description
My first embedded project - an LCD display (with I2C module), buzzer and a button connected to a ESP8266 module via a breadboard. A simple websocket server with Express that serves a simple HTML page where the user can send a message that will be printed on the LCD display.

## How it works / Features
- When the ESP8266 module is connected to power, it will connect to a WiFi network. Connecting... and Connected text will be printed on the display.
> **Note**: `ssid` and `password` need to be hardcoded in the code.
- Via the server (currently hosted at [ESP8266-Pager](https://esp8266-pager.onrender.com)) the user can send a message.
> **Note**: Due to the 16x2 size of the display, a 32 character limit is enforced on the input.
- On sending a message a loading spinner will be rendered on the send button. After receiving and displaying the message, the ESP8266 will send back a response to the server, informing the server that the button should be usable again.
- A notification sound will be played through the buzzer on a new message.
- The display will be turned off after 10 seconds. A new message or manually pressing the button on the breadboard will turn the display on for 10 seconds.
- If a message is not received by the ESP8266 in 5 seconds (probably it's offline), a timeout will occur and an error message will be displayed

## Developing
I have used the [Arduino IDE](https://www.arduino.cc/en/software/) to write the C++ code for the ESP8266 module.
Required Arduino libraries:
- **ESP Async TCP** 2.0.0 by ESP32Async
- **WebSockets** 2.6.1 by Markus Sattler
- **LiquidCrystal I2C** 1.1.2 by Frank de Brabander
