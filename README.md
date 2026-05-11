# Profit Monitor

This project turns a small LED Panel into a display showing live profit-and-loss chart with different screens on different time-frames.

### Main Components:
LED Panel (64x32)
https://thepihut.com/products/rgb-full-colour-led-matrix-panel-2-5mm-pitch-64x32-pixels

Any ESP32 Dev Board

The link below shows how to use the LED Panel in conjunction with ESP32 and other drivers.
https://www.waveshare.com/wiki/RGB-Matrix-P2.5-64x32#Working_With_ESP32
Downloaded as `RGB-Matrix-P2.5-64x32 - Waveshare Wiki.pdf` in case the website takes the page down.

### ESP32 program
The ESP32 program was originally created by ChatGPT then trouble-shot by Claude Code.  I added the Chart Object, stale chart detection and various touch-up later on.  You can download sample programs and library from the above link.

Download the Arduino IDE, the ESP32 library and the LED Panel library.  It's important to downgrade ESP32 library to 2.0.17 because the program is based on an older library.

It is the prompt I asked in ChatGPT:
> I have a RGB-Matrix-P2.5-64x32 LED panel that connected to a ESP32 dev board.  I have the demo program working using Arduino IDE.  Now I want to make it more usable by implementing a data format that it receives from WiFi and display.  The idea is to have 8 different virtual screens.  The ESP32 is connected to a momentary push button which allows selecting which screen to display.  Each screen can have up to 10 drawing objects.  An object can be a text or a shape of rectangle, circle or a line in a colour that the data specifies.  The program in ESP32 should be able to process requests from http to update or clear an object on a screen.   Can you help designing the data format and making the program?  Objects should persist in the non-volatile memory of ESP32.

### Scripts
Examples of sending commands from python and shell scripts can be found in `scripts` directory.

The profit monitor chart driver are `scripts/profit_mon.py` and .scripts/profit_mon.sh`.  It queries database and convert into chart command and send to ESP32.

### Case
The case is designed using FreeCAD and 3D printed.  Files are put under the `case` directory.

### Future development
Add an internal LiPO battery.  Using PowerBoost charger (3.7 Lion/Lipo battery) https://www.adafruit.com/product/1944

### Reference
Similar product: https://www.etsy.com/uk/listing/1255228529/crypto-ticker-stocks-forex-live-price
