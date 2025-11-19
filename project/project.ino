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

// --- Tiles ---
static lv_obj_t* tileview;
static lv_obj_t* t1;
static lv_obj_t* t2;
static lv_obj_t* t3;
static lv_obj_t* t4;

static lv_obj_t* t1_label;
static lv_obj_t* t2_label;

// Tile 2 state
static bool t2_dark = false;  // start tile #2 in light mode

// --- Tile 3: historical chart + slider ---
static lv_obj_t* t3_chart = nullptr;
static lv_obj_t* t3_slider = nullptr;
static lv_chart_series_t* t3_series = nullptr;

static const int HIST_MAX_POINTS = 256;
static float hist_values[HIST_MAX_POINTS];
static int hist_count = 0;       // how many valid points in hist_values[]
static int hist_window = 50;     // how many points to show at once

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
// Tile 3 – historical chart helpers
// ------------------------------------------------------

// Re-render the chart based on slider value
static void t3_update_chart_window(int start_index)
{
  if (!t3_chart || !t3_series) return;
  if (hist_count <= 0) return;
  if (hist_window <= 0) hist_window = 1;

  // Clamp start so we don't read out of bounds
  if (start_index < 0) start_index = 0;
  if (start_index > hist_count - hist_window) {
    start_index = max(0, hist_count - hist_window);
  }

  // Set chart point count to window size
  lv_chart_set_point_count(t3_chart, hist_window);

  // Fill series by index instead of "clearing"
  for (int i = 0; i < hist_window; ++i) {
    int idx = start_index + i;
    lv_coord_t v = LV_CHART_POINT_NONE;
    if (idx < hist_count) {
      v = (lv_coord_t)hist_values[idx];
    }
    lv_chart_set_value_by_id(t3_chart, t3_series, i, v);
  }

  lv_chart_refresh(t3_chart);
}

// Slider event: move window over data
static void t3_slider_event_cb(lv_event_t* e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* slider = lv_event_get_target(e);

  // While pressing the slider, turn off tileview scrolling
  if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
    if (tileview) {
      lv_obj_clear_flag(tileview, LV_OBJ_FLAG_SCROLLABLE);
    }
  }

  // When finger leaves slider, re-enable tileview scrolling
  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    if (tileview) {
      lv_obj_add_flag(tileview, LV_OBJ_FLAG_SCROLLABLE);
    }
  }

  if (code == LV_EVENT_VALUE_CHANGED) {
    int pos = lv_slider_get_value(slider);
    t3_update_chart_window(pos);
  }
}


// Fill hist_values[] with dummy data so UI can be tested
// (Replace later with real SMHI "latest-months" parsing)
static void fill_dummy_historical_data()
{
  hist_count = HIST_MAX_POINTS;
  for (int i = 0; i < hist_count; ++i) {
    // some sine-ish wave between -5 and +15
    float x = (float)i / 10.0f;
    hist_values[i] = 5.0f + 10.0f * sinf(x * 0.5f);
  }

  // window size: up to 50 points or all if fewer
  hist_window = min(50, hist_count);
}

// Call this after data is loaded to init slider + chart content
static void t3_bind_data_to_ui()
{
  if (!t3_slider || !t3_chart || !t3_series) return;
  if (hist_count <= 0) return;

  int max_start = max(0, hist_count - hist_window);

  // Slider: 0 => oldest window, max => latest window
  lv_slider_set_range(t3_slider, 0, max_start);
  lv_slider_set_value(t3_slider, max_start, LV_ANIM_OFF);

  // Start showing latest window by default
  t3_update_chart_window(max_start);
}



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
  // Tile #1 – simple forecast label
  // --------------------------------------------------
  {
    t1_label = lv_label_create(t1);
    lv_label_set_text(t1_label, "Laddar väder...");
    lv_obj_set_style_text_font(t1_label, &lv_font_montserrat_28, 0);
    lv_obj_center(t1_label);
    apply_tile_colors(t1, t1_label, /*dark=*/false);
  }

  // --------------------------------------------------
  // Tile #2 – welcome / click to toggle colors
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
  // Tile #3 – historical data: chart + slider
  // --------------------------------------------------
  {
    // Background + base style using same helper
    lv_obj_t* tmp_label = lv_label_create(t3);  // just for text color
    lv_label_set_text(tmp_label, "");
    apply_tile_colors(t3, tmp_label, /*dark=*/false);
    lv_obj_del(tmp_label);  // we don't need the label

    // Chart
    t3_chart = lv_chart_create(t3);
    lv_obj_set_size(t3_chart,
                    lv_pct(95),    // width
                    lv_pct(65));   // height
    lv_obj_align(t3_chart, LV_ALIGN_TOP_MID, 0, 10);

    lv_chart_set_type(t3_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(t3_chart, 4, 4);
    lv_chart_set_update_mode(t3_chart, LV_CHART_UPDATE_MODE_SHIFT);

    // y-axis range — adjust if you know real data range
    lv_chart_set_range(t3_chart, LV_CHART_AXIS_PRIMARY_Y, -10, 30);

    // Series
    t3_series = lv_chart_add_series(t3_chart, lv_color_black(), LV_CHART_AXIS_PRIMARY_Y);

    // Label to show what parameter/city (placeholder)
    lv_obj_t* title = lv_label_create(t3);
    lv_label_set_text(title, "Historical (dummy data)");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align_to(title, t3_chart, LV_ALIGN_OUT_TOP_LEFT, 5, -5);

t3_slider = lv_slider_create(t3);
lv_obj_set_size(t3_slider, lv_pct(90), 60);   // width: 90% of screen, height: 60px
lv_obj_align(t3_slider, LV_ALIGN_BOTTOM_MID, 0, -10);
lv_obj_set_style_pad_all(t3_slider, 15, LV_PART_KNOB);  // bigger knob hitbox
lv_slider_set_range(t3_slider, 0, 100);  // real range set after data load
lv_obj_add_event_cb(t3_slider, t3_slider_event_cb, LV_EVENT_ALL, NULL);

  }


  // --------------------------------------------------
  // Tile #4 – settings placeholder
  // --------------------------------------------------
  {
    lv_obj_t* label = lv_label_create(t4);
    lv_label_set_text(label, "Settings (tile 4)\nWIP");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_center(label);
    apply_tile_colors(t4, label, /*dark=*/false);
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

  // --- Tile 3: prepare some data and bind to chart ---
  fill_dummy_historical_data();   // later: replace with real SMHI call
  t3_bind_data_to_ui();
}


void loop()
{
  lv_timer_handler();
  delay(5);
}

