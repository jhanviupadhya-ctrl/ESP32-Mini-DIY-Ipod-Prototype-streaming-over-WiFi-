#ifndef AUDIO_UI_H
#define AUDIO_UI_H
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// ---- Cute pastel theme (RGB565) ----
#define UI_BG      0xFEBC  // soft pastel pink background
#define UI_CARD    0xFFFF  // white "card" boxes behind text
#define UI_BORDER  0xFB56  // hot pink border/accents
#define UI_LABEL   0xFB56  // hot pink labels
#define UI_TEXT    0x594F  // deep purple main text (readable on white cards)
#define UI_ERROR   0xF800  // red for errors
#define UI_ACCENT  0xE6FF  // lavender accent

// ---- Small decorative icons ----
static const uint8_t PROGMEM icon_heart_bits[] = {
    0b01101100,
    0b11111110,
    0b11111110,
    0b11111110,
    0b01111100,
    0b00111000,
    0b00010000,
    0b00000000
};

static const uint8_t PROGMEM icon_note_bits[] = {
    0b00001100,
    0b00001100,
    0b00001100,
    0b00001100,
    0b00001100,
    0b01111100,
    0b11111100,
    0b01111000
};

struct TextArea { int x, y, w, h; };

TextArea stationArea = {14, 34, 100, 10};
TextArea titleArea   = {14, 60, 100, 20}; // up to 2 lines
TextArea bitrateArea = {14, 94, 92, 10};
TextArea statusArea  = {14, 134, 100, 20};

char currentStation[40] = "";
char currentTitle[64]   = "";
char currentBitrate[16] = "";
char currentStatus[48]  = "Connecting...";
bool isError = false;

void drawHeart(int x, int y, uint16_t color) {
    tft.drawBitmap(x, y, icon_heart_bits, 8, 8, color);
}

void drawNote(int x, int y, uint16_t color) {
    tft.drawBitmap(x, y, icon_note_bits, 8, 8, color);
}

// Draws a white rounded "card" so text areas pop off the pastel background
void drawCard(TextArea a) {
    tft.fillRoundRect(a.x - 4, a.y - 4, a.w + 8, a.h + 8, 5, UI_CARD);
    tft.drawRoundRect(a.x - 4, a.y - 4, a.w + 8, a.h + 8, 5, UI_BORDER);
}

void clearArea(TextArea a) {
    // redraw the card fresh instead of just blanking to background,
    // so text always sits on a clean white card
    drawCard(a);
}

// Wraps text into up to (a.h / 8*size) lines of (a.w / 6*size) chars each
void printWrapped(const char* text, TextArea a, uint16_t color, int textSize = 1) {
    tft.setTextColor(color);
    tft.setTextSize(textSize);
    int charW = 6 * textSize;
    int lineH = 8 * textSize;
    int maxChars = a.w / charW;
    int maxLines = a.h / lineH;

    int len = strlen(text);
    int pos = 0, line = 0;
    while (pos < len && line < maxLines) {
        int chunkLen = min(len - pos, maxChars);
        tft.setCursor(a.x, a.y + line * lineH);
        for (int i = 0; i < chunkLen; i++) tft.print(text[pos + i]);
        pos += chunkLen;
        line++;
    }
}

void updateStation(const char* name) {
    strncpy(currentStation, name, sizeof(currentStation) - 1);
    currentStation[sizeof(currentStation) - 1] = '\0';
    clearArea(stationArea);
    printWrapped(currentStation, stationArea, UI_TEXT);
}

void updateTitle(const char* title) {
    strncpy(currentTitle, title, sizeof(currentTitle) - 1);
    currentTitle[sizeof(currentTitle) - 1] = '\0';
    clearArea(titleArea);
    printWrapped(currentTitle, titleArea, UI_TEXT);
}

void updateBitrate(const char* br) {
    strncpy(currentBitrate, br, sizeof(currentBitrate) - 1);
    currentBitrate[sizeof(currentBitrate) - 1] = '\0';
    clearArea(bitrateArea);
    tft.setTextColor(UI_TEXT);
    tft.setTextSize(1);
    tft.setCursor(bitrateArea.x, bitrateArea.y);
    tft.print(currentBitrate);
    tft.print(" kbps");
}

void updateStatus(const char* msg, bool error = false) {
    strncpy(currentStatus, msg, sizeof(currentStatus) - 1);
    currentStatus[sizeof(currentStatus) - 1] = '\0';
    isError = error;
    clearArea(statusArea);
    printWrapped(currentStatus, statusArea, error ? UI_ERROR : UI_TEXT);
}

void drawAudioUI() {
    tft.fillScreen(UI_BG);

    // outer scalloped-look border: two nested rounded rects
    tft.drawRoundRect(2, 2, 124, 156, 10, UI_BORDER);
    tft.drawRoundRect(4, 4, 120, 152, 8, UI_ACCENT);

    // cute header with note + heart flourishes
    drawNote(8, 9, UI_BORDER);
    tft.setTextColor(UI_BORDER);
    tft.setTextSize(1);
    tft.setTextWrap(false);
    tft.setCursor(22, 10);
    tft.println("Now Playing");
    drawHeart(108, 9, UI_BORDER);

    tft.drawFastHLine(10, 22, 108, UI_ACCENT);

    // section labels in hot pink, sitting just above each white card
    tft.setTextColor(UI_LABEL);
    tft.setCursor(stationArea.x - 4, stationArea.y - 12);
    tft.println("Station");
    tft.setCursor(titleArea.x - 4, titleArea.y - 12);
    tft.println("Title");
    tft.setCursor(bitrateArea.x - 4, bitrateArea.y - 12);
    tft.println("Bitrate");

    tft.drawFastHLine(10, 124, 108, UI_ACCENT);

    // little decorative hearts along the bottom, just for cuteness
    drawHeart(14, 144, UI_ACCENT);
    drawHeart(58, 144, UI_BORDER);
    drawHeart(102, 144, UI_ACCENT);

    updateStation(currentStation);
    updateTitle(currentTitle);
    updateBitrate(currentBitrate);
    updateStatus(currentStatus, isError);
}

#endif