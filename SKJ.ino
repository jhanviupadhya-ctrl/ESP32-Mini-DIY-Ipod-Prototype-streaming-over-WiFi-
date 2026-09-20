#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include "Screen1.h"
#include "Screen.h"

// ─────────────────────────────────────────────────────────────
// TYPES — declared up top so Arduino's auto-generated function
// prototypes (inserted near the top of the file) always compile,
// regardless of where the functions that use them are defined.
// ─────────────────────────────────────────────────────────────
enum Screen{TITLE,MENU,MUSIC_PLAYER};
enum ButtonPress{NO_BUTTON,SCROLL,SELECT,PLAY};
struct MenuBox { int x, y, w, h; };

#define CS 15
#define RST 4
#define AO 2
#define SDA 23
#define SCK 18

#define SCROLL_PIN 14
#define SELECT_PIN 12
#define PLAY_PIN 13

#include "Arduino.h"
#include "WiFi.h"
#include "Audio_nopsram.h"  // ESP32-audioI2S-nopsram v2.0.6+GCC14 patches (schreibfaul1)
                            // renamed from Audio.h to avoid conflict with
                            // ESP32-audioI2S (schreibfaul1 upstream, requires PSRAM)
                            // ⚠ DO NOT replace with schreibfaul1/ESP32-audioI2S — see SOFTWARE section above
#include "esp_system.h"     // esp_reset_reason()
#include "lwip/sockets.h"   // POSIX socket API (lwIP)
#include "lwip/netdb.h"     // getaddrinfo()

// ─────────────────────────────────────────────────────────────
// USER CONFIGURATION
// ─────────────────────────────────────────────────────────────

static const char* WIFI_SSID = "Agastya";
static const char* WIFI_PASSWORD = "7458813029";

// ─── Local music streaming over WiFi ───
// Serve your downloaded files with a simple local server on your computer,
// e.g. `python3 -m http.server 8000` run inside the folder containing them.
// Set PLAYLIST_SERVER to that computer's local IP + port (find via ipconfig/
// ifconfig — it looks like 192.168.1.XX). Both devices must be on the same WiFi.
const char* PLAYLIST_SERVER = "http://192.168.0.101"; // HFS server address (port 80, default)

struct Track { const char* title; const char* filename; };
Track playlist[] = {
    {"Freaked Out",  "/MUsic%202/freaked%20out.mp3"},
    {"Drag Race",    "/MUsic%202/drag%20race%20-%20resentful.mp3"},
    // Add more here, matching filenames in your served folder, e.g.:
    // {"My Song", "/MUSIC/my_song.mp3"},
};
const int NUM_TRACKS = sizeof(playlist) / sizeof(playlist[0]);
int currentTrackIndex = 0;

// ─────────────────────────────────────────────────────────────
// I2S PIN MAPPING
// ─────────────────────────────────────────────────────────────

#define I2S_BCLK   27   ///< Bit clock       → GPIO27
#define I2S_LRC    26   ///< Left/right clock → GPIO26
#define I2S_DOUT   25   ///< Data out         → GPIO25

// ─────────────────────────────────────────────────────────────
// AUDIO / TIMING SETTINGS
// ─────────────────────────────────────────────────────────────
#define VOLUME               20  ///< 0 (mute) – 21 (max) — turned up for testing, lower once confirmed working
#define PREBUFFER_MS        3000


#define CONNECTION_TIMEOUT  5000



#define WIFI_TIMEOUT_MS      15000
#define RECONNECT_DELAY_MS    5000
#define TCP_PROBE_TIMEOUT_MS  8000


Audio audio;
#include "AudioUI.h"

void audio_info(const char* info) {
    Serial.printf("[info]    %s\n", info);
    updateStatus(info, false);
}

void audio_id3data(const char* info) {
    Serial.printf("[id3]     %s\n", info);
}

void audio_showstation(const char* info) {
    Serial.printf("[station] %s\n", info);
    updateStation(info);
}

void audio_showstreamtitle(const char* info) {
    Serial.printf("[title]   %s\n", info);
    updateTitle(info);
}

void audio_bitrate(const char* info) {
    Serial.printf("[bitrate] %s\n", info);
    updateBitrate(info);
}

void audio_error(const char* info) {
    Serial.printf("[ERROR]   %s\n", info);
    updateStatus(info, true);
}

void audio_eof_stream(const char* info) {
    Serial.printf("[eof]     %s — advancing to next track\n", info);
    currentTrackIndex = (currentTrackIndex + 1) % NUM_TRACKS;
    playTrack(currentTrackIndex);
}

void connectWiFi() {
    updateStatus("Connecting to WiFi...", false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
        delay(200);
    }
    if (WiFi.status() != WL_CONNECTED) {
        updateStatus("WiFi failed", true);
    }
}

void playTrack(int index) {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
        if (WiFi.status() != WL_CONNECTED) return; // give up if still not connected
    }
    char url[160];
    snprintf(url, sizeof(url), "%s%s", PLAYLIST_SERVER, playlist[index].filename);
    updateStatus("Loading track...", false);
    updateStation(playlist[index].title); // reuse the "station" display field for the track name
    audio.connecttohost(url);
}


Adafruit_ST7735 tft(CS, AO, SDA, SCK, RST);
Screen currentScreen = TITLE;

const int buttonPins[3] = {SCROLL_PIN,SELECT_PIN,PLAY_PIN};
const ButtonPress buttonNames[3] = {SCROLL,SELECT,PLAY};
bool previousbuttonStates[3] ={HIGH,HIGH,HIGH};

ButtonPress getPreesedButton(){
  ButtonPress pressedButton = NO_BUTTON;
  for(int i=0; i<3; i++){
    bool isPressedButton = digitalRead(buttonPins[i]);
    if(previousbuttonStates[i]==HIGH && isPressedButton==LOW) pressedButton = buttonNames[i];
    previousbuttonStates[i] = isPressedButton;
  }
  if (pressedButton != NO_BUTTON) delay(20);
  return pressedButton;
}

void setup()
{
    Serial.begin(115200);

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(VOLUME);

    tft.initR(INITR_BLACKTAB);
    tft.setRotation(0);
    pinMode(SCROLL_PIN, INPUT);
    pinMode(SELECT_PIN, INPUT);
    pinMode(PLAY_PIN, INPUT);

    delay(10); // let external pull-ups settle
    for (int i = 0; i < 3; i++) {
        previousbuttonStates[i] = digitalRead(buttonPins[i]);
    }

    drawScreen_1();
}
MenuBox menuBoxes[3] = {
  {14, 13, 26, 31},   // music icon box
  {13, 64, 27, 25},   // clock icon box
  {17, 108, 24, 26}   // camera icon box
};
int selectedMenu = 0;
void highlightMenu(int index) {
    uint16_t highlightColor = 0xF800; // red = selected
    uint16_t normalColor = 0x4208;    // default box color

    for (int i = 0; i < 3; i++) {
        uint16_t color = (i == index) ? highlightColor : normalColor;
        tft.drawRect(menuBoxes[i].x, menuBoxes[i].y, menuBoxes[i].w, menuBoxes[i].h, color);
    }
}
void loop()
{
    if (currentScreen == TITLE) {
        animate();
    }

    ButtonPress pressedButton = getPreesedButton();

    if (currentScreen == TITLE && pressedButton != NO_BUTTON) {
        drawScreen_3();
        currentScreen = MENU;
        selectedMenu = 0;
        highlightMenu(selectedMenu);
    }
    else if (currentScreen == MENU) {
        if (pressedButton == SCROLL) {
            selectedMenu = (selectedMenu + 1) % 3;
            highlightMenu(selectedMenu);
        }
        else if (pressedButton == SELECT) {
            if (selectedMenu == 0) {
                // MUSIC tile chosen
                currentScreen = MUSIC_PLAYER;
                drawAudioUI();
                playTrack(currentTrackIndex); // start playing the first track
            }
            else if (selectedMenu == 1) {
                // drawScreen_time(); currentScreen = TIME_SCREEN;
            }
            else if (selectedMenu == 2) {
                // drawScreen_photos(); currentScreen = PHOTOS_SCREEN;
            }
        }
    }
    else if (currentScreen == MUSIC_PLAYER) {
        audio.loop();   // keep the audio stream running while this screen is active

        if (pressedButton == SCROLL) {
            // move to the next track and preview its name,
            // without loading it yet — press PLAY to actually play it
            currentTrackIndex = (currentTrackIndex + 1) % NUM_TRACKS;
            updateStation(playlist[currentTrackIndex].title);
            updateStatus("Press PLAY to listen", false);
        }
        if (pressedButton == PLAY) {
            playTrack(currentTrackIndex); // play whichever track is currently highlighted
        }
        if (pressedButton == SELECT) {
            // go back to the menu
            audio.stopSong();
            drawScreen_3();
            currentScreen = MENU;
            highlightMenu(selectedMenu);
        }
    }
}

