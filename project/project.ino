#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <lvgl.h>
#include <math.h>
#include <string.h>

// Wi-Fi credentials (Delete these before commiting to GitHub)
static const char* WIFI_SSID     = "TP-Link_70AB";
static const char* WIFI_PASSWORD = "73522256";

LilyGo_Class amoled;

static lv_obj_t* tileview;
static lv_obj_t* t1;
static lv_obj_t* t2;
static lv_obj_t* t1_label;
static lv_obj_t* t2_label;
static bool t2_dark = false;  // start tile #2 in light mode
static lv_obj_t* t3;
static lv_obj_t* t4;
static lv_obj_t* t3_label;
static lv_obj_t* t4_label;
static const char* PROGRAM_VERSION = "v.1.0.0";
static const char* GROUP_NUMBER ="Group 17";

static void show_start_screen()
{
  lv_obj_t* scr = lv_scr_act();  // current (default) screen

  lv_obj_t* label = lv_label_create(scr);
  lv_label_set_text_fmt(label, "Version: %s\n%s", PROGRAM_VERSION, GROUP_NUMBER);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
  lv_obj_center(label);
}



// ------------------------------------------------------
// Helper: Tile #2 Color change
// ------------------------------------------------------
static void apply_tile_colors(lv_obj_t* tile, lv_obj_t* label, bool dark)
{
  // Background
  lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(tile, dark ? lv_color_black() : lv_color_white(), 0);

  // Text
  lv_obj_set_style_text_color(label, dark ? lv_color_white() : lv_color_black(), 0);
}

static void on_tile2_clicked(lv_event_t* e)
{
  LV_UNUSED(e);
  t2_dark = !t2_dark;
  apply_tile_colors(t2, t2_label, t2_dark);
}

// ------------------------------------------------------
// UI setup
// ------------------------------------------------------
// ------------------------------------------------------
// UI setup
// ------------------------------------------------------
static void create_ui()
{
  // Fullscreen Tileview
  tileview = lv_tileview_create(lv_scr_act());
  lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
  lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

  // Add four horizontal tiles: 0,1,2,3
  t1 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
  t2 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
  t3 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);
  t4 = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_HOR);

  // --------------------------------------------------
  // Tile #1 – current / simple forecast (already used)
  // --------------------------------------------------
  {
    t1_label = lv_label_create(t1);
    lv_label_set_text(t1_label, "Laddar väder...");
    lv_obj_set_style_text_font(t1_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t1_label);
    apply_tile_colors(t1, t1_label, /*dark=*/false);
  }

  // --------------------------------------------------
  // Tile #2 – welcome / interaction demo
  // --------------------------------------------------
  {
    t2_label = lv_label_create(t2);
    lv_label_set_text(t2_label, "Welcome to the workshop");
    lv_obj_set_style_text_font(t2_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t2_label);

    apply_tile_colors(t2, t2_label, /*dark=*/false);
    lv_obj_add_flag(t2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(t2, on_tile2_clicked, LV_EVENT_CLICKED, NULL);
  } 

  // --------------------------------------------------
  // Tile #3 – placeholder for historical data view
  // --------------------------------------------------
  {
    t3_label = lv_label_create(t3);
    lv_label_set_text(t3_label, "Historical data (tile 3)\nWIP");
    lv_obj_set_style_text_font(t3_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t3_label);
    apply_tile_colors(t3, t3_label, /*dark=*/false);
  }

  // --------------------------------------------------
  // Tile #4 – placeholder for settings screen
  // --------------------------------------------------
  {
    t4_label = lv_label_create(t4);
    lv_label_set_text(t4_label, "Settings (tile 4)\nWIP");
    lv_obj_set_style_text_font(t4_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t4_label);
    apply_tile_colors(t4, t4_label, /*dark=*/false);
  }
}


// ------------------------------------------------------
// WiFi
// ------------------------------------------------------
static void connect_wifi()
{
  Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi could not connect (timeout).");
  }
}

// ------------------------------------------------------
// SMHI → T1 label
// ------------------------------------------------------
static void update_t1_with_weather()
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("update_t1_with_weather: WiFi not connected");
    lv_label_set_text(t1_label, "WiFi fail");
    lv_obj_center(t1_label);
    return;
  }

  HTTPClient http;
  const char* url =
    "https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/"
    "geotype/point/lon/15.5869/lat/56.1612/data.json";

  Serial.println("Requesting SMHI data...");
  http.begin(url);

  // Ask server not to gzip/compress if possible
  http.addHeader("Accept-Encoding", "identity");

  int httpCode = http.GET();

  Serial.print("HTTP code: ");
  Serial.println(httpCode);

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP error: %d\n", httpCode);
    lv_label_set_text(t1_label, "HTTP error");
    lv_obj_center(t1_label);
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  Serial.print("Payload length: ");
  Serial.println(payload.length());

  Serial.println("Payload (first 300 chars):");
  Serial.println(payload.substring(0, 300));

  // --- CRUCIAL: trim everything before the first '{' ---
  int braceIndex = payload.indexOf('{');
  if (braceIndex < 0) {
    Serial.println("No '{' found in payload, not JSON?");
    lv_label_set_text(t1_label, "No JSON");
    lv_obj_center(t1_label);
    return;
  }

  if (braceIndex > 0) {
    Serial.print("Trimming leading bytes before JSON: ");
    Serial.println(braceIndex);
    payload.remove(0, braceIndex);
  }

  // Now payload should start with '{'
  Serial.println("Payload after trim (first 200 chars):");
  Serial.println(payload.substring(0, 200));

    DynamicJsonDocument doc(220000);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("JSON error AFTER trim: ");
    Serial.println(err.c_str());

    char buf[64];
    snprintf(buf, sizeof(buf), "JSON err: %s", err.c_str());
    lv_label_set_text(t1_label, buf);
    lv_obj_center(t1_label);
    return;
  }

  JsonObject first;  // this will be the object we read temp from

  JsonArray timeSeries = doc["timeSeries"];
  if (!timeSeries.isNull() && timeSeries.size() > 0) {
    // Normal SMHI structure: root has "timeSeries"
    Serial.println("Using doc[\"timeSeries\"][0]");
    first = timeSeries[0];
  } else {
    // Fallback: root IS the forecast object
    Serial.println("No timeSeries array, using root object as forecast");
    first = doc.as<JsonObject>();
  }

  const char* validTime = first["validTime"] | "N/A";

  float temp = NAN;
  float precip = NAN;

  JsonArray params = first["parameters"];
  if (params.isNull()) {
    Serial.println("parameters missing");
    lv_label_set_text(t1_label, "No params");
    lv_obj_center(t1_label);
    return;
  }

  for (JsonObject p : params) {
    const char* name = p["name"] | "";
    if (!name) continue;

    if (strcmp(name, "t") == 0) {
      temp = p["values"][0] | NAN;
    } else if (strcmp(name, "pmean") == 0) {
      precip = p["values"][0] | NAN;
    }
  }

  char buf[96];
  snprintf(buf, sizeof(buf), "Karlskrona %.1f°C / %.1f mm", temp, precip);

  Serial.print("Setting label: ");
  Serial.println(buf);

  lv_label_set_text(t1_label, buf);
  lv_obj_center(t1_label);
}




 

void setup()
{
  Serial.begin(115200);
  Serial.println("BOOTING...");
  delay(200);

  if (!amoled.begin()) {
    Serial.println("Failed to init LilyGO AMOLED.");
    while (true) delay(1000);
  }

  beginLvglHelper(amoled);   // init LVGL for this board

  // --- START SCREEN: version + group --- 
  show_start_screen();   // create label on default screen

  // Let LVGL render the splash for 0,8 seconds. 
  uint32_t start = millis();
  while (millis() - start < 800) {
    lv_timer_handler();  // refresh LVGL
    delay(5);
  }

  // Clear everything on the current screen
  lv_obj_clean(lv_scr_act());

  // --- NORMAL APP UI ---
  create_ui();
  connect_wifi();
  update_t1_with_weather();
}

void loop()
{
  lv_timer_handler();
  delay(5);
}

