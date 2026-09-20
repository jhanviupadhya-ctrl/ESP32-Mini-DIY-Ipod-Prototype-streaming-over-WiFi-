# ESP32-Mini-DIY-Ipod-Prototype-streaming-over-WiFi-
This is about New ESP32 player prototype Project . I have made use of cheap material. It is kind of similar to mini ipod which  You can built under 2 dollars or 700 rupees. I have Used following materials for my project :
1. Breadboard
2. bread board wires
3. 3x 10 kilo ohm resistors for each button
4. 3x push buttons
5. 1.8 inch TFT display
6. ESP32 DEV  Module
7. USB cable
8. And a laptop
9. I2S MAX98357A class D amplifier along with speaker
# Wirring
Tft & ESP32             I2S MAX98357 A class d amplifier
CS 15                     //< Bit clock       → GPIO27
RST 4                          ///< Left/right clock → GPIO26
AO 2                            ///< Data out         → GPIO25  
SDA 23
SCK 18


push button & ep32 (10 k ohm -------------- GND) 2nd terminal of push button to common power rail
SCROLL_PIN 14                     |
SELECT_PIN 12                     | 
PLAY_PIN 13                      GPIO PIN 

I created my own server on hfs . You can download exe from github . Make sure to open your hfs folder on your terminal.
You'll be directed to a website and u have to upload music folder over there. Copy the IP address to your code
const char* PLAYLIST_SERVER = "http://......";
struct Track { const char* title; const char* filename; };
Track playlist[] = {
    {"Music 1",  "addsks.mp3"},
    {"MUISC 2",    "sdjds.mp3"},
    // Add more here, matching filenames in your served folder, e.g.:
    // {"My Song", "/MUSIC/my_song.mp3"},
};

