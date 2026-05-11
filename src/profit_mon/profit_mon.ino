#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include "Tiny4x6.h"

// --------------------------------------------------
// WIFI
// --------------------------------------------------
const char* WIFI_SSID = "FRITZ!Box 7530 CX";
const char* WIFI_PASS = "your wifi password";

// --------------------------------------------------
// PANEL + BUTTON
// --------------------------------------------------
#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

const int BUTTON1_PIN = 34;
const int BUTTON2_PIN = 36;

MatrixPanel_I2S_DMA *dma_display = nullptr;
WebServer server(80);
Preferences prefs;

// --------------------------------------------------
// APP CONFIG
// --------------------------------------------------
const int NUM_SCREENS = 8;
const int MAX_OBJECTS = 10;

volatile int currentScreen = 0;
volatile bool renderRequested = false;
volatile int brightnessLevel = 0;  // Start at Low

const int BRIGHTNESSES[] = {20, 64, 255};
const int NUM_BRIGHTNESS_LEVELS = sizeof(BRIGHTNESSES) / sizeof(BRIGHTNESSES[0]);

bool lastButtonState1 = HIGH;
bool buttonStableState1 = HIGH;
unsigned long lastDebounceTime1 = 0;

bool lastButtonState2 = HIGH;
bool buttonStableState2 = HIGH;
unsigned long lastDebounceTime2 = 0;

const unsigned long debounceDelay = 50;

// delayed save
bool prefsDirty = false;
unsigned long lastPrefsChangeMs = 0;
const unsigned long prefsSaveDelayMs = 1000;

// preferences
const char* PREF_NAMESPACE = "matrixapp";
const char* KEY_CURRENT_SCREEN = "curscr";
const char* KEY_BRIGHTNESS_LEVEL = "brightness";

// CONSTANTS
int16_t SENTINEL_SLIGHTLY_NEGATIVE_BAR_VALUE = -255;

// Stale chart variables
const unsigned long CHART_STALE_INTERVAL_MS = 60 * 1000;
unsigned long last_chart_update_time = 0;
bool is_chart_stale = false;

// --------------------------------------------------
// COLOURS
// --------------------------------------------------
uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myRED = dma_display->color565(255, 0, 0);
uint16_t myGREEN = dma_display->color565(0, 255, 0);
uint16_t myBLUE = dma_display->color565(0, 0, 255);

// Chart colours
uint16_t colourUpShade = dma_display->color565(0, 0, 60);
uint16_t colourUpRange = dma_display->color565(0, 0, 120);
uint16_t colourUpClose = dma_display->color565(0, 0, 255);
uint16_t colourDownShade = dma_display->color565(60, 0, 0);
uint16_t colourDownRange = dma_display->color565(120, 0, 0);
uint16_t colourDownClose = dma_display->color565(255, 0, 0);

// --------------------------------------------------
// DATA MODEL
// --------------------------------------------------
enum ObjectType {
  OBJ_NONE = 0,
  OBJ_TEXT,
  OBJ_RECT,
  OBJ_CIRCLE,
  OBJ_LINE,
  OBJ_CHART
};

struct ColorRGB {
  uint8_t r = 255;
  uint8_t g = 255;
  uint8_t b = 255;
};

struct Bar {
  int16_t  h = 0;
  int16_t  l = 0;
  int16_t  c = 0;
};

struct DrawingObject {
  bool used = false;
  bool visible = true;
  ObjectType type = OBJ_NONE;
  ColorRGB color;

  // Shared
  int x = 0;
  int y = 0;

  // Text
  char text[64] = "";
  uint8_t size = 1;

  // Rect
  int w = 0;
  int h = 0;
  bool filled = false;

  // Circle
  int radius = 0;

  // Line
  int x1 = 0;
  int y1 = 0;
  int x2 = 0;
  int y2 = 0;

  // Chart
  uint8_t base = 15;
  uint8_t len = 0;
  Bar* bars = nullptr;
};

DrawingObject screens[NUM_SCREENS][MAX_OBJECTS];

// --------------------------------------------------
// DISPLAY FUNCTIONS
// --------------------------------------------------
void displayBegin() {
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN
  );

  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::FM6047;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

  if (!dma_display->begin()) {
    Serial.println("Matrix allocation failed!");
    while (true) delay(1000);
  }

  dma_display->setBrightness8(BRIGHTNESSES[brightnessLevel]);
  dma_display->clearScreen();

  delay(1000);
  for (int y = 0; y < PANEL_RES_Y; y++) {
    int intensity = y / 2;
    dma_display->drawLine(0, y, PANEL_RES_X - 1, y, dma_display->color444(intensity, intensity, intensity));
  }

  dma_display->fillCircle(40, 22, 10, dma_display->color444(15, 0, 15));
  delay(1000);
}

uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
  return dma_display->color565(r, g, b);
}

void displayClear() {
  dma_display->clearScreen();
}

void drawTextOnDisplay(int x, int y, const char* text, uint8_t size, uint16_t color) {
  dma_display->setTextWrap(false);
  dma_display->setTextSize(size);
  dma_display->setTextColor(color);
  dma_display->setCursor(x, y);
  dma_display->print(text);
}

void drawRectOnDisplay(int x, int y, int w, int h, bool filled, uint16_t color) {
  if (filled) dma_display->fillRect(x, y, w, h, color);
  else dma_display->drawRect(x, y, w, h, color);
}

void drawCircleOnDisplay(int x, int y, int radius, bool filled, uint16_t color) {
  if (filled) dma_display->fillCircle(x, y, radius, color);
  else dma_display->drawCircle(x, y, radius, color);
}

void drawLineOnDisplay(int x1, int y1, int x2, int y2, uint16_t color) {
  dma_display->drawLine(x1, y1, x2, y2, color);
}

int16_t toY(int16_t y) {
  return PANEL_RES_Y - y;

}

void drawChartOnDisplay(int base, int len, Bar* bars) {

  for (int x = 0; x < len; x++) {
    Bar& bar = bars[x];
    int bar_c = (bar.c == SENTINEL_SLIGHTLY_NEGATIVE_BAR_VALUE) ? base : bar.c;

    if (bar_c > bar.h or bar_c < bar.l or bar.h < bar.l) {
      Serial.printf("ERROR: Invalid bar: base=%d, h=%d, l=%d, c=%d\n", base, bar.h, bar.l, bar.c);
      continue;
    }

    if (bar.h == base and bar.l == base) {
      // do nothing

    } else if (bar.h > base && bar.l < base) {
      dma_display->drawLine(x, toY(bar.l), x, toY(base - 1), colourDownRange);
      dma_display->drawLine(x, toY(base), x, toY(bar.h), colourUpRange);

    } else if (bar.l < base) {
      if (bar.h < base) {
        dma_display->drawLine(x, toY(bar.h + 1), x, toY(base), colourDownShade);
      }
      if (bar.l < bar.h) {
        dma_display->drawLine(x, toY(bar.l), x, toY(bar.h), colourDownRange);
      }
    
    } else if (bar.h > base) {
      if (bar.l > base) {
        dma_display->drawLine(x, toY(base), x, toY(bar.l - 1), colourUpShade);
      }
      if (bar.l < bar.h) {
        dma_display->drawLine(x, toY(bar.l), x, toY(bar.h), colourUpRange);
      }
    }

    dma_display->drawPixel(x, toY(bar_c), bar.c < base ? colourDownClose : colourUpClose);
  }

  // Draw stale mark
  if (is_chart_stale) {
    dma_display->drawLine(0, 1, 8, 9, myRED);
    dma_display->drawLine(0, 9, 8, 1, myRED);
  }
}

// --------------------------------------------------
// HELPERS
// --------------------------------------------------
bool validScreen(int s) {
  return s >= 0 && s < NUM_SCREENS;
}

bool validSlot(int s) {
  return s >= 0 && s < MAX_OBJECTS;
}

void clearObject(int screen, int slot) {
  if (screens[screen][slot].bars != nullptr) {
    delete[] screens[screen][slot].bars;
  }
  screens[screen][slot] = DrawingObject();
}

void clearScreenData(int screen) {
  for (int i = 0; i < MAX_OBJECTS; i++) {
    clearObject(screen, i);
  }
}

ObjectType parseType(const char* typeStr) {
  if (strcmp(typeStr, "text") == 0) return OBJ_TEXT;
  if (strcmp(typeStr, "rect") == 0) return OBJ_RECT;
  if (strcmp(typeStr, "circle") == 0) return OBJ_CIRCLE;
  if (strcmp(typeStr, "line") == 0) return OBJ_LINE;
  if (strcmp(typeStr, "chart") == 0) return OBJ_CHART;
  return OBJ_NONE;
}

void renderCurrentScreen() {
  displayClear();

  for (int i = 0; i < MAX_OBJECTS; i++) {
    DrawingObject &obj = screens[currentScreen][i];
    if (!obj.used || !obj.visible) continue;

    uint16_t color565 = rgbTo565(obj.color.r, obj.color.b, obj.color.g);

    switch (obj.type) {
      case OBJ_TEXT:
        drawTextOnDisplay(obj.x, obj.y, obj.text, obj.size, color565);
        break;

      case OBJ_RECT:
        drawRectOnDisplay(obj.x, obj.y, obj.w, obj.h, obj.filled, color565);
        break;

      case OBJ_CIRCLE:
        drawCircleOnDisplay(obj.x, obj.y, obj.radius, obj.filled, color565);
        break;

      case OBJ_LINE:
        drawLineOnDisplay(obj.x1, obj.y1, obj.x2, obj.y2, color565);
        break;

      case OBJ_CHART:
        drawChartOnDisplay(obj.base, obj.len, obj.bars);
        break;

      default:
        break;
    }
  }
}

// --------------------------------------------------
// PREFERENCES
// --------------------------------------------------
String makeObjectKey(int screen, int slot) {
  return "s" + String(screen) + "o" + String(slot);
}

bool loadObjectFromPrefs(int screen, int slot) {
  if (!validScreen(screen) || !validSlot(slot)) return false;

  String key = makeObjectKey(screen, slot);
  size_t len = prefs.getBytesLength(key.c_str());

  if (len != sizeof(DrawingObject)) {
    clearObject(screen, slot);
    return false;
  }

  size_t read = prefs.getBytes(key.c_str(), &screens[screen][slot], sizeof(DrawingObject));
  return read == sizeof(DrawingObject);
}

void saveAllScreensToPrefs() {
  for (int s = 0; s < NUM_SCREENS; s++) {
    for (int i = 0; i < MAX_OBJECTS; i++) {
      DrawingObject& obj = screens[s][i];
      String key = makeObjectKey(s, i);
      // Do not save chart which is big.
      if (obj.used && obj.type != ObjectType::OBJ_CHART) {
        size_t written = prefs.putBytes(key.c_str(), &obj, sizeof(DrawingObject));
        if (written != sizeof(DrawingObject)) {
          Serial.printf("ERROR: Failed to save s%do%d (written=%d)\n", s, i, written);
        }
      } else {
        prefs.remove(key.c_str());
      }
    }
  }
  prefs.putUChar(KEY_CURRENT_SCREEN, (uint8_t)currentScreen);
  prefs.putUChar(KEY_BRIGHTNESS_LEVEL, (uint8_t)brightnessLevel);
  Serial.println("Saved screens to Preferences");
}

void loadAllScreensFromPrefs() {
  for (int s = 0; s < NUM_SCREENS; s++) {
    for (int i = 0; i < MAX_OBJECTS; i++) {
      loadObjectFromPrefs(s, i);
    }
  }

  currentScreen = prefs.getUChar(KEY_CURRENT_SCREEN, 0);
  if (currentScreen >= NUM_SCREENS) currentScreen = 0;

  brightnessLevel = prefs.getUChar(KEY_BRIGHTNESS_LEVEL, 0);
  if (brightnessLevel >= NUM_BRIGHTNESS_LEVELS) brightnessLevel = 0;

  Serial.println("Loaded screens from Preferences");
}

void markPrefsDirty() {
  prefsDirty = true;
  lastPrefsChangeMs = millis();
}

void handleDeferredPrefsSave() {
  if (prefsDirty && (millis() - lastPrefsChangeMs > prefsSaveDelayMs)) {
    saveAllScreensToPrefs();
    prefsDirty = false;
  }
}

// --------------------------------------------------
// JSON OBJECT PARSE
// --------------------------------------------------
bool parseDrawingObject(JsonObject objJson, DrawingObject &obj, String &err) {
  const char* typeStr = objJson["type"] | "";
  ObjectType type = parseType(typeStr);

  if (type == OBJ_NONE) {
    err = "Invalid type";
    return false;
  }

  obj = DrawingObject();
  obj.used = true;
  obj.visible = objJson["visible"] | true;
  obj.type = type;
  obj.color.r = objJson["r"] | 255;
  obj.color.g = objJson["g"] | 255;
  obj.color.b = objJson["b"] | 255;

  switch (type) {
    case OBJ_TEXT: {
      obj.x = objJson["x"] | 0;
      obj.y = objJson["y"] | 8;
      obj.size = objJson["size"] | 1;
      const char* txt = objJson["text"] | "";
      strncpy(obj.text, txt, sizeof(obj.text) - 1);
      obj.text[sizeof(obj.text) - 1] = '\0';
      break;
    }

    case OBJ_RECT: {
      obj.x = objJson["x"] | 0;
      obj.y = objJson["y"] | 0;
      obj.w = objJson["w"] | 0;
      obj.h = objJson["h"] | 0;
      obj.filled = objJson["filled"] | false;
      break;
    }

    case OBJ_CIRCLE: {
      obj.x = objJson["x"] | 0;
      obj.y = objJson["y"] | 0;
      obj.radius = objJson["radius"] | 0;
      obj.filled = objJson["filled"] | false;
      break;
    }

    case OBJ_LINE: {
      obj.x1 = objJson["x1"] | 0;
      obj.y1 = objJson["y1"] | 0;
      obj.x2 = objJson["x2"] | 0;
      obj.y2 = objJson["y2"] | 0;
      break;
    }

    case OBJ_CHART: {
      obj.base = objJson["base"] | 15;

      JsonArray arr = objJson["bars"].as<JsonArray>();
      size_t numBars = arr.size();
      obj.bars = new Bar[numBars];

      int i = 0;
      for (JsonObject jsonBar : arr) {
        Bar& bar = obj.bars[i];
        bar.h = jsonBar["h"];
        bar.l = jsonBar["l"];
        bar.c = jsonBar["c"];
        i++;
      }

      obj.len = i;
      last_chart_update_time = millis();
      break;
    }

    default:
      err = "Unsupported type";
      return false;
  }

  return true;
}

// --------------------------------------------------
// HTTP HELPERS
// --------------------------------------------------
void sendJsonResponse(int code, const String &status, const String &message = "") {
  JsonDocument doc;
  doc["status"] = status;
  if (message.length()) doc["message"] = message;
  doc["currentScreen"] = currentScreen;

  String out;
  serializeJson(doc, out);

  server.sendHeader("Connection", "close");
  server.send(code, "application/json", out);

  // The library can only serve one client at time, so forcing stop client connection after every response.
  server.client().stop();
}

// --------------------------------------------------
// HTTP ROUTES
// --------------------------------------------------
void handleRoot() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain",
    "ESP32 RGB Matrix API\n"
    "GET  /api/status\n"
    "POST /api/object\n"
    "POST /api/clearObject\n"
    "POST /api/clearScreen\n"
    "POST /api/showScreen\n"
    "POST /api/resetStorage\n"
  );
}

void handleStatus() {
  JsonDocument doc;
  doc["status"] = "ok";
  doc["currentScreen"] = currentScreen;

  JsonArray arr = doc["screens"].to<JsonArray>();
  for (int s = 0; s < NUM_SCREENS; s++) {
    int count = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
      if (screens[s][i].used) count++;
    }
    arr.add(count);
  }

  String out;
  serializeJson(doc, out);
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", out);
}

bool applyObjectUpdate(int screen, int slot, JsonObject objJson, String &err, int index) {
  if (!validScreen(screen) || !validSlot(slot)) {
    err = String("Invalid screen or slot at index ") + index;
    return false;
  }
  if (objJson.isNull()) {
    err = String("Missing object at index ") + index;
    return false;
  }
  DrawingObject temp;
  if (!parseDrawingObject(objJson, temp, err)) {
    err = String("Index ") + index + ": " + err;
    return false;
  }
  clearObject(screen, slot);  // Make sure allocated memory is freed up.
  screens[screen][slot] = temp;
  if (screen == currentScreen) renderRequested = true;
  return true;
}

void handleSetObject() {
  String body = server.arg("plain");
  if (body.length() == 0) {
    sendJsonResponse(400, "error", "Missing JSON body");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    sendJsonResponse(400, "error", String("JSON parse failed: ") + error.c_str());
    return;
  }

  String err;
  int updated = 0;

  if (doc["objects"].is<JsonArray>()) {
    int index = 0;
    for (JsonObject item : doc["objects"].as<JsonArray>()) {
      if (!applyObjectUpdate(item["screen"] | -1, item["slot"] | -1, item["object"].as<JsonObject>(), err, index)) {
        sendJsonResponse(400, "error", err);
        return;
      }
      updated++;
      index++;
    }
  } else {
    if (!applyObjectUpdate(doc["screen"] | -1, doc["slot"] | -1, doc["object"].as<JsonObject>(), err, 0)) {
      sendJsonResponse(400, "error", err);
      return;
    }
    updated = 1;
  }

  markPrefsDirty();
  sendJsonResponse(200, "ok", String(updated) + " object(s) updated");
}

void handleClearObject() {
  int screen = -1;
  int slot = -1;

  if (server.hasArg("screen")) screen = server.arg("screen").toInt();
  if (server.hasArg("slot")) slot = server.arg("slot").toInt();

  if (screen < 0 || slot < 0) {
    String body = server.arg("plain");
    if (body.length()) {
      JsonDocument doc;
      if (deserializeJson(doc, body) == DeserializationError::Ok) {
        screen = doc["screen"] | -1;
        slot = doc["slot"] | -1;
      }
    }
  }

  if (!validScreen(screen) || !validSlot(slot)) {
    sendJsonResponse(400, "error", "Invalid screen or slot");
    return;
  }

  clearObject(screen, slot);
  markPrefsDirty();

  if (screen == currentScreen) {
    renderRequested = true;
  }

  sendJsonResponse(200, "ok", "Object cleared");
}

void handleClearScreen() {
  int screen = -1;

  if (server.hasArg("screen")) {
    screen = server.arg("screen").toInt();
  } else {
    String body = server.arg("plain");
    if (body.length()) {
      JsonDocument doc;
      if (deserializeJson(doc, body) == DeserializationError::Ok) {
        screen = doc["screen"] | -1;
      }
    }
  }

  if (!validScreen(screen)) {
    sendJsonResponse(400, "error", "Invalid screen");
    return;
  }

  clearScreenData(screen);
  markPrefsDirty();

  if (screen == currentScreen) {
    renderRequested = true;
  }

  sendJsonResponse(200, "ok", "Screen cleared");
}

void handleShowScreen() {
  int screen = -1;

  if (server.hasArg("screen")) {
    screen = server.arg("screen").toInt();
  } else {
    String body = server.arg("plain");
    if (body.length()) {
      JsonDocument doc;
      if (deserializeJson(doc, body) == DeserializationError::Ok) {
        screen = doc["screen"] | -1;
      }
    }
  }

  if (!validScreen(screen)) {
    sendJsonResponse(400, "error", "Invalid screen");
    return;
  }

  currentScreen = screen;
  prefs.putUChar(KEY_CURRENT_SCREEN, (uint8_t)currentScreen);
  renderRequested = true;

  sendJsonResponse(200, "ok", "Screen changed");
}

void handleResetStorage() {
  prefs.clear();

  for (int s = 0; s < NUM_SCREENS; s++) {
    clearScreenData(s);
  }

  currentScreen = 0;
  renderRequested = true;
  prefsDirty = false;

  sendJsonResponse(200, "ok", "Storage cleared");
}

// Switch to the next non-blank screen
void toNextScreen() {
  int screen = (currentScreen + 1) % NUM_SCREENS;

  while (not screens[screen][0].used and screen != currentScreen) {
    screen = (screen + 1) % NUM_SCREENS;
  }

  currentScreen = screen;

  Serial.printf("Current screen: %d\n", currentScreen);
}

// Rotate brightness level
void setNextBrightness() {
    brightnessLevel++;
    if (brightnessLevel >= NUM_BRIGHTNESS_LEVELS) brightnessLevel = 0; // Cycle: Low -> Med -> High

    int brightness = BRIGHTNESSES[brightnessLevel];

    dma_display->setBrightness8(brightness);
    Serial.print("Set brightness to "); Serial.println(brightness);
}

// --------------------------------------------------
// WIFI / SERVER
// --------------------------------------------------
void connectWifi() {
  esp_wifi_set_ps(WIFI_PS_NONE);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());

  drawText4x6(dma_display, 0, 2, WiFi.localIP().toString().c_str(), myBLUE);

  delay(5000);
}

void setupServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/object", HTTP_POST, handleSetObject);
  server.on("/api/clearObject", HTTP_POST, handleClearObject);
  server.on("/api/clearScreen", HTTP_POST, handleClearScreen);
  server.on("/api/showScreen", HTTP_POST, handleShowScreen);
  server.begin();
}

// --------------------------------------------------
// SETUP / LOOP
// --------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  displayBegin();

  prefs.begin(PREF_NAMESPACE, false);

  for (int s = 0; s < NUM_SCREENS; s++) {
    clearScreenData(s);
  }

  loadAllScreensFromPrefs();

  connectWifi();
  setupServer();
  renderRequested = true;
}

void loop() {
  server.handleClient();

  bool reading1 = digitalRead(BUTTON1_PIN);

  if (reading1 != lastButtonState1) {
    lastDebounceTime1 = millis();
  }

  if ((millis() - lastDebounceTime1) > debounceDelay) {
    if (reading1 != buttonStableState1) {
      buttonStableState1 = reading1;

      if (buttonStableState1 == LOW) {
        setNextBrightness();
        prefs.putUChar(KEY_CURRENT_SCREEN, (uint8_t)currentScreen);
        renderRequested = true;
      }
    }
  }

  lastButtonState1 = reading1;

  bool reading2 = digitalRead(BUTTON2_PIN);

  if (reading2 != lastButtonState2) {
    lastDebounceTime2 = millis();
  }

  if ((millis() - lastDebounceTime2) > debounceDelay) {
    if (reading2 != buttonStableState2) {
      buttonStableState2 = reading2;

      if (buttonStableState2 == LOW) {
        toNextScreen();
        prefs.putUChar(KEY_CURRENT_SCREEN, (uint8_t)currentScreen);
        renderRequested = true;

      }
    }
  }

  lastButtonState2 = reading2;

  // Check for stale chart
  if (millis() - last_chart_update_time > CHART_STALE_INTERVAL_MS) {
    if (!is_chart_stale) {
      Serial.println("stale chart");
      is_chart_stale = true;
      renderRequested = true;
    }
  } else {
    if (is_chart_stale) {
      Serial.println("chart ok");
      is_chart_stale = false;
      renderRequested = true;
    }
  }

  if (renderRequested) {
    renderRequested = false;
    renderCurrentScreen();
  }

  handleDeferredPrefsSave();
}
