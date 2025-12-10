#define LV_FONT_FMT_TXT_LARGE 1
#define LV_USE_FONT_FMT_TXT 1
#define LV_USE_UTF8 1
#define LV_USE_FONT_CUSTOM 1

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
#include <Preferences.h>
#include "google_emoji.h"

// Wi-Fi credentials
static const char* WIFI_SSID     = "BTH_Guest";
static const char* WIFI_PASSWORD = "plommon86silver";

LV_FONT_DECLARE(google_emoji);

// ------------------------
// Forecast model
// ------------------------
struct DailyForecast {
    String dayName;    // e.g. "MM-DD"
    int   temp;        // °C
    int   symbolCode;  // SMHI Wsymb2 code
};

static DailyForecast forecast[7];
static lv_obj_t* forecast_table = nullptr;
static lv_obj_t* dropdown = nullptr;

// Cities: Karlskrona(65090), Stockholm(97400), Göteborg(72420), Malmö(53300), Kiruna(180940)

// Names
static const char* CITY_NAMES[5] = {
    "Karlskrona",
    "Stockholm",
    "Gothenburg",
    "Malmo",
    "Kiruna"
};

// Station IDs for SMHI historical data
static const int CITY_STATION_IDS[5] = {
    65090,   // Karlskrona-Soderstjerna
    97400,   // Stockholm-Observatorielunden
    72420,   // Göteborg-Landvetter
    53300,   // Malmö-A                    (closest active)
    180940   // Kiruna-Esrange
};

// Matched forecast coordinates (LAT,LON)
static const float CITY_LAT[5] = {
    56.1612,   // Karlskrona
    59.3293,   // Stockholm
    57.7089,   // Göteborg
    55.6050,   // Malmö
    67.8558    // Kiruna
};

static const float CITY_LON[5] = {
    15.5869,   // Karlskrona
    18.0686,   // Stockholm
    11.9746,   // Göteborg
    13.0038,   // Malmö
    20.2253    // Kiruna
};

// ------------------------
// Current selections (city/parameter)
// ------------------------

// 0..4 → Karlskrona, Stockholm, Gothenburg, Malmo, Kiruna
static int current_city_index  = 0;  // default Karlskrona

// Parameters for historical data 
static const int PARAM_IDS[4]      = { 1, 6, 4, 9 };  // temp, humidity, wind, pressure
static const char* PARAM_LABELS[4] = { "Temperature", "Humidity", "Wind", "Pressure" };
static int current_param_index     = 0;               // default: Temperature

static const char* get_current_city_name() {
    return CITY_NAMES[current_city_index];
}

static int get_current_station_id() {
    return CITY_STATION_IDS[current_city_index];
}

static int get_current_param_id() {
    return PARAM_IDS[current_param_index];
}

static int set_current_station_id(int id) {
    current_city_index = id;
    return CITY_STATION_IDS[current_city_index];
}

static int set_current_param_id(int id) {
    current_param_index = id;
    return PARAM_IDS[current_param_index];
}

Preferences prefs;

static const char* NVM_NAMESPACE   = "weatherapp";
static const char* KEY_DEF_CITY    = "def_city";
static const char* KEY_DEF_PARAM   = "def_param";

// Sparade default-värden
static int default_city_index  = 0; // 0..4
static int default_param_index = 0; // 0..3

// Pekare till dropdowns på Tile 4
static lv_obj_t* dd_param = nullptr;
static lv_obj_t* dd_city  = nullptr;

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
static lv_obj_t* t3_chart  = nullptr;
static lv_obj_t* t3_slider = nullptr;
static lv_chart_series_t* t3_series = nullptr;

static const int HIST_MAX_POINTS = 2160;
static float hist_values[HIST_MAX_POINTS];
static String hist_dates[HIST_MAX_POINTS];
static int   hist_count  = 0;   // how many valid points in hist_values[]
static int   hist_window = 50;  // how many points to show at once

static lv_chart_series_t* t3_series_mean = nullptr;
static float hist_mean = 0.0f;
static lv_obj_t* t3_mean_label = nullptr;

// Datumspann för historiska data (för x-axel)
static String hist_date_start;
static String hist_date_end;

// Axis labels för Tile 3
static lv_obj_t* t3_label_yaxis   = nullptr;  // enhet (°C, %, m/s, hPa)
static lv_obj_t* t3_label_xaxis   = nullptr;  // text "Date"
static lv_obj_t* t3_label_x_start = nullptr;  // första datum
static lv_obj_t* t3_label_x_end   = nullptr;  // sista datum

// NYTT: info-label + index för aktuellt fönster
static lv_obj_t* t3_slider_info = nullptr;
static int current_window_start = 0;
static int current_window_end   = 0;

static const char* PROGRAM_VERSION = "v.1.1.0";
static const char* GROUP_NUMBER    = "Group 17";

// ------------------------------------------------------
// SMHI symbol → simple UTF-8 icon mapping
// ------------------------------------------------------
static const char* smhi_symbol_to_icon(int code) {
    switch(code) {

        // Clear
        case 0:
        case 1: return "☀";

        // Partly cloudy
        case 2:
        case 3: return "🌤";

        // Cloudy / overcast
        case 4:
        case 5: return "☁";

        // Fog
        case 6:
        case 7: return "🌫";

        // Rain showers
        case 8:
        case 9:
        case 10: return "🌦";

        // Thunderstorm
        case 11:
        case 21: return "⛈";

        // Sleet showers
        case 12:
        case 13:
        case 14: return "🌨";

        // Snow showers
        case 15:
        case 16:
        case 17: return "🌨";

        // Continuous rain
        case 18:
        case 19:
        case 20: return "🌧";

        // Continuous sleet
        case 22:
        case 23:
        case 24: return "🌨";

        // Continuous snowfall
        case 25:
        case 26:
        case 27: return "❄";

        default:
            Serial.printf("Unknown Wsymb2 code (UNEXPECTED): %d\n", code);
            return "?";
    }
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

    if (start_index < 0) start_index = 0;
    if (start_index > hist_count - hist_window) {
        start_index = max(0, hist_count - hist_window);
    }

    lv_chart_set_point_count(t3_chart, hist_window);

    for (int i = 0; i < hist_window; ++i) {
        int idx = start_index + i;
        lv_coord_t v = LV_CHART_POINT_NONE;

        if (idx < hist_count) {
            v = (lv_coord_t)hist_values[idx];
        }

        // main data series
        lv_chart_set_value_by_id(t3_chart, t3_series, i, v);

        // mean line series: constant at hist_mean
        if (t3_series_mean && hist_count > 0) {
            lv_chart_set_value_by_id(
                t3_chart,
                t3_series_mean,
                i,
                (lv_coord_t)hist_mean
            );
        }
    }

    // Uppdatera visat datumspann baserat på fönstret
    if (hist_count > 0) {
        int start_idx = start_index;
        int end_idx   = start_index + hist_window - 1;
        if (end_idx >= hist_count) end_idx = hist_count - 1;

        // NYTT: spara aktuellt fönster globalt
        current_window_start = start_idx;
        current_window_end   = end_idx;

        if (start_idx >= 0 && start_idx < hist_count) {
            hist_date_start = hist_dates[start_idx];
        } else {
            hist_date_start = "";
        }

        if (end_idx >= 0 && end_idx < hist_count) {
            hist_date_end = hist_dates[end_idx];
        } else {
            hist_date_end = "";
        }

        update_axis_labels();
    }

    lv_chart_refresh(t3_chart);
}

// Slider event: move window over data
static void t3_slider_event_cb(lv_event_t* e)
{
    lv_event_code_t code   = lv_event_get_code(e);
    lv_obj_t*       slider = lv_event_get_target(e);

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

        // NYTT: uppdatera info-label med datum + värde
        if (t3_slider_info && hist_count > 0) {
            int idx = current_window_end;   // sista punkten i fönstret
            if (idx < 0) idx = 0;
            if (idx >= hist_count) idx = hist_count - 1;

            const char* unit = "Value";
            switch (current_param_index) {
                case 0: unit = "°C";  break;  // Temperature
                case 1: unit = "%";   break;  // Humidity
                case 2: unit = "m/s"; break;  // Wind speed
                case 3: unit = "hPa"; break;  // Pressure
                default: break;
            }

            const char* date_str = hist_dates[idx].length()
                       ? hist_dates[idx].c_str()
                       : "no date";
            char buf[64];
            snprintf(
                buf,
                sizeof(buf),
                "%s: %.1f %s",
                date_str,
                hist_values[idx],
                unit
            );
            lv_label_set_text(t3_slider_info, buf);
        }
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
        hist_dates[i]  = "";   // inga riktiga datum för dummy-data
    }

    // window size: up to 50 points or all if fewer
    hist_window = min(50, hist_count);

    hist_date_start = "";
    hist_date_end   = "";
    update_axis_labels();
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

// Uppdatera axel-etiketter (enhet + datumspann)
static void update_axis_labels()
{
    // Y-axel: enhet beroende på current_param_index
    if (t3_label_yaxis) {
        const char* unit = "Value";
        switch (current_param_index) {
            case 0: unit = "°C";  break;   // Temperature
            case 1: unit = "%";   break;   // Humidity
            case 2: unit = "m/s"; break;   // Wind speed
            case 3: unit = "hPa"; break;   // Pressure
            default: break;
        }
        lv_label_set_text(t3_label_yaxis, unit);
    }

    // X-axel huvudtext
    if (t3_label_xaxis) {
        lv_label_set_text(t3_label_xaxis, "Date");
    }

    // Första/sista datum under grafen
    if (t3_label_x_start) {
        lv_label_set_text(
            t3_label_x_start,
            hist_date_start.length() ? hist_date_start.c_str() : ""
        );
    }
    if (t3_label_x_end) {
        lv_label_set_text(
            t3_label_x_end,
            hist_date_end.length() ? hist_date_end.c_str() : ""
        );
    }

    // Debug: skriv ut vad vi *tror* datumen är
    Serial.print("Axis labels: start=");
    Serial.print(hist_date_start);
    Serial.print(" end=");
    Serial.println(hist_date_end);
}

// ------------------------------------------------------
// Forecast helpers
// ------------------------------------------------------
static String get_day_name_from_iso(const String& iso) {
    // iso like "2025-11-19T12:00:00Z"
    // We'll just return the date part (YYYY-MM-DD) or a short version.
    // If you want real weekday names, you can convert with time.h later.
    return iso.substring(5, 10); // "MM-DD"
}

static int find_param_int(JsonArray params, const char* name) {
    for (JsonObject p : params) {
        const char* pName = p["name"];
        if (strcmp(pName, name) == 0) {
            return (int)p["values"][0].as<float>();
        }
    }
    return 0;
}

static void update_forecast_from_smhi(JsonDocument& doc) {
    // Clear previous content
    for (int i = 0; i < 7; ++i) {
        forecast[i].dayName    = "";
        forecast[i].temp       = 0;
        forecast[i].symbolCode = 0;
    }

    JsonArray timeSeries = doc["timeSeries"];

    // --------------------------------------------------
    // Case 1: normal SMHI format – root has "timeSeries"
    // --------------------------------------------------
    if (!timeSeries.isNull()) {
        int    dayIndex = 0;
        String lastDate = "";

        for (JsonObject ts : timeSeries) {
            if (dayIndex >= 7) break;

            String validTime = ts["validTime"].as<String>();  // "2025-11-20T12:00:00Z"
            if (validTime.length() < 16) continue;

            String datePart = validTime.substring(0, 10);     // "2025-11-20"
            String hourPart = validTime.substring(11, 13);    // "12"

            // Only interested in 12:00 entries
            if (hourPart != "12") continue;

            // Skip if we already used this date
            if (datePart == lastDate) continue;
            lastDate = datePart;

            JsonArray params = ts["parameters"].as<JsonArray>();
            if (params.isNull()) continue;

            int temp       = find_param_int(params, "t");
            int symbolCode = find_param_int(params, "Wsymb2");

            forecast[dayIndex].dayName    = get_day_name_from_iso(validTime);  // "MM-DD"
            forecast[dayIndex].temp       = temp;
            forecast[dayIndex].symbolCode = symbolCode;

            Serial.printf("Day %d → %s (%s) = %d°C (symbol %d)\n",
                          dayIndex,
                          forecast[dayIndex].dayName.c_str(),
                          datePart.c_str(),
                          temp,
                          symbolCode);

            dayIndex++;
        }

        Serial.printf("update_forecast_from_smhi: filled %d days with 12:00 entries\n", dayIndex);
        return;
    }

    // --------------------------------------------------
    // Case 2: fallback – root IS a single forecast object
    // --------------------------------------------------
    Serial.println("update_forecast_from_smhi: no timeSeries, using root as single forecast");

    JsonObject ts = doc.as<JsonObject>();
    if (ts.isNull() || ts["validTime"].isNull()) {
        Serial.println("update_forecast_from_smhi: root has no validTime, cannot build forecast");
        return;
    }

    String validTime = ts["validTime"].as<String>();
    JsonArray params = ts["parameters"].as<JsonArray>();
    if (params.isNull()) {
        Serial.println("update_forecast_from_smhi: root has no parameters");
        return;
    }

    int temp       = find_param_int(params, "t");
    int symbolCode = find_param_int(params, "Wsymb2");

    forecast[0].dayName    = get_day_name_from_iso(validTime);
    forecast[0].temp       = temp;
    forecast[0].symbolCode = symbolCode;
}




static void refresh_forecast_table() {
    if (!forecast_table) return;

    for (int i = 0; i < 7; ++i) {
        int row = i + 1; // row 0 is header

        // If dayName is empty, leave row blank
        if (forecast[i].dayName.length() == 0) {
            lv_table_set_cell_value(forecast_table, row, 0, "");
            lv_table_set_cell_value(forecast_table, row, 1, "");
            lv_table_set_cell_value(forecast_table, row, 2, "");
            continue;
        }

        static char tempBuf[16];
        snprintf(tempBuf, sizeof(tempBuf), "%d°C", forecast[i].temp);

        lv_table_set_cell_value(forecast_table, row, 0, forecast[i].dayName.c_str());
        lv_table_set_cell_value(forecast_table, row, 1, tempBuf);
        lv_table_set_cell_value(forecast_table, row, 2,
                                smhi_symbol_to_icon(forecast[i].symbolCode));
    }
}



// ------------------------
// SMHI Historical URL (latest-months) for selected city/parameter
// ------------------------
static String build_smhi_hist_url()
{
    // Example:
    // https://opendata-download-metobs.smhi.se/api/version/1.0/
    //   parameter/{id}/station/{id}/period/latest-months/data.json
    String url =
        String("https://opendata-download-metobs.smhi.se/api/version/1.0/")
        + "parameter/" + String(get_current_param_id())
        + "/station/"  + String(get_current_station_id())
        + "/period/latest-months/data.json";
    return url;
}

static bool load_historical_data_from_smhi()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("load_historical_data_from_smhi: WiFi not connected");
        return false;
    }

    HTTPClient http;
    String url = build_smhi_hist_url();

    Serial.print("Requesting SMHI historical data: ");
    Serial.println(url);

    http.setReuse(false);
    http.useHTTP10(true);
    http.begin(url);
    http.addHeader("Accept-Encoding", "identity");

    int httpCode = http.GET();
    Serial.print("HTTP code (hist): ");
    Serial.println(httpCode);

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP error (hist): %d\n", httpCode);
        http.end();
        return false;
    }

    // Läs hela svaret som en String istället för att parsa direkt från streamen
    String payload = http.getString();
    http.end();
    
    // Debug: skriv ut längden så vi ser att vi fått något rimligt
    Serial.print("Hist payload length: ");
    Serial.println(payload.length());

    // Större buffer, metobs-json kan vara ganska stor
    DynamicJsonDocument doc(400000);
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
        Serial.print("JSON error (hist): ");
        Serial.println(err.c_str());
        return false;
    }

    JsonArray values = doc["value"].as<JsonArray>();
    if (values.isNull() || values.size() == 0) {
        Serial.println("load_historical_data_from_smhi: no 'value' array");
        return false;
    }

    int total = values.size();
    Serial.print("Historical points from SMHI: ");
    Serial.println(total);

    // Keep at most HIST_MAX_POINTS latest values
    int start_index = 0;
    if (total > HIST_MAX_POINTS) {
        start_index = total - HIST_MAX_POINTS;
    }

    hist_count = 0;
    float min_v = 1e9f;
    float max_v = -1e9f;
    float sum_v = 0.0f;
    
    for (int i = start_index; i < total && hist_count < HIST_MAX_POINTS; ++i) {
        JsonObject vobj = values[i].as<JsonObject>();
        if (vobj.isNull()) continue;
        if (vobj["value"].isNull()) continue;
        
        // Själva mätvärdet
        float v = vobj["value"].as<float>();
        hist_values[hist_count] = v;

        // ---- KONVERTERA "date" (Unix-ms) → "YYYY-MM-DD" ----
        String dateLabel = "?";

        // Läs date som long long (ms sedan 1970)
        long long ms = 0;
        if (!vobj["date"].isNull()) {
            // ArduinoJson kan ge oss det direkt som 64-bitars integer
            ms = vobj["date"].as<long long>();
        }
        
        if (ms > 0) {
            time_t secs = (time_t)(ms / 1000);  // ms → sekunder
            struct tm * t = gmtime(&secs);      // UTC
            
            if (t) {
                char buf[11];
                // YYYY-MM-DD
                snprintf(
                    buf,
                    sizeof(buf),
                    "%04d-%02d-%02d",
                    t->tm_year + 1900,
                    t->tm_mon + 1,
                    t->tm_mday
                );
                dateLabel = String(buf);
            } else {
                Serial.println("WARN: gmtime() returned null, using '?' as date");
            }
        } else {
            Serial.println("WARN: no valid 'date' field, using '?' as date");
        }

        // Spara datumetiketten
        hist_dates[hist_count] = dateLabel;
        
        // Debug: skriv ut vad vi fick
        Serial.print("Hist point ");
        Serial.print(hist_count);
        Serial.print(" -> '");
        Serial.print(hist_dates[hist_count]);
        Serial.print("' = ");
        Serial.println(v);
        
        // Min/max + summa för medelvärde
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
        
        sum_v += v;
        hist_count++;
    }

    if (hist_count == 0) {
        Serial.println("load_historical_data_from_smhi: no valid numeric values");
        return false;
    }

    // Beräkna medelvärde
    hist_mean = sum_v / (float)hist_count;
    Serial.print("Historical mean = ");
    Serial.println(hist_mean);
    if (t3_mean_label) {
        char mean_buf[32];
        snprintf(mean_buf, sizeof(mean_buf), "Mean: %.1f", hist_mean);
        lv_label_set_text(t3_mean_label, mean_buf);
    }

    // Fönsterstorlek (max 50 punkter eller alla om färre)
    hist_window = min(50, hist_count);

    // Justera Y-axelns range baserat på datan
    if (t3_chart) {
        int y_min = (int)floorf(min_v - 1.0f);
        int y_max = (int)ceilf (max_v + 1.0f);
        if (y_min == y_max) { y_min -= 1; y_max += 1; }
        lv_chart_set_range(t3_chart, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    }

    Serial.print("Historical data loaded. hist_count = ");
    Serial.println(hist_count);

    // Uppdatera axel-etiketter när nya data laddats
    update_axis_labels();

    return true;
}
// ------------------------------------------------------
// Forecast table UI
// ------------------------------------------------------
static void create_forecast_table(lv_obj_t* parent) {
    forecast_table = lv_table_create(parent);

    lv_obj_set_size(forecast_table, lv_pct(100), lv_pct(90));
    lv_obj_align(forecast_table, LV_ALIGN_BOTTOM_MID, 0, 0);

    static lv_style_t table_style;
    lv_style_init(&table_style);
    lv_style_set_text_font(&table_style, &google_emoji);
    lv_obj_add_style(forecast_table, &table_style, 0);

    // 3 columns: Day | Temp | Icon
    lv_table_set_col_cnt(forecast_table, 3);
    lv_table_set_row_cnt(forecast_table, 8); // 1 header + 7 days

    // Header row
    lv_table_set_cell_value(forecast_table, 0, 0, "Dag");
    lv_table_set_cell_value(forecast_table, 0, 1, "Temp");
    lv_table_set_cell_value(forecast_table, 0, 2, "Icon");

    lv_table_set_col_width(forecast_table, 0, 190);
    lv_table_set_col_width(forecast_table, 1, 190);
    lv_table_set_col_width(forecast_table, 2, 190);
}

// ------------------------------------------------------
// Dropdown menu
// ------------------------------------------------------
static lv_obj_t* create_dropdown_menu(lv_obj_t* parent)
{
    dropdown = lv_dropdown_create(parent);
    lv_obj_set_size(dropdown, lv_pct(25), lv_pct(10));
    lv_obj_align(dropdown, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(dropdown, dropdown_event, LV_EVENT_VALUE_CHANGED, NULL);
    return dropdown;
}

// ------------------------------------------------------
// Get dropdown selected index
// ------------------------------------------------------
static void dropdown_event(lv_event_t* event)
{
    lv_obj_t* dropdown = lv_event_get_target(event);

    if (lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED)
    {
        int tag      = (int) lv_obj_get_user_data(dropdown);
        int selected = lv_dropdown_get_selected(dropdown);
        Serial.printf("Dropdown selected index: %d\n", selected);

        if (tag == 1)
        {
            // Parameter dropdown (Temperature / Humidity / Wind / Pressure)
            set_current_param_id(selected);
        }
        else if (tag == 2)
        {
            // City dropdown (Karlskrona / Stockholm / Gothenburg / Malmo / Kiruna)
            set_current_station_id(selected);   // updates current_city_index
            update_t2_with_weather();           
        }

        // Historical data always depends on both city and param
        if (load_historical_data_from_smhi())
        {
            t3_bind_data_to_ui();
        }
        else
        {
            fill_dummy_historical_data();
            t3_bind_data_to_ui();
        }
    }
}

// ------------------------------------------------------
// Default-hantering (lagras i flash med Preferences)
// ------------------------------------------------------
static void load_defaults_from_nvm()
{
    if (!prefs.begin(NVM_NAMESPACE, true)) { // true = read-only
        Serial.println("Preferences: failed to begin (read)");
        return;
    }

    default_city_index  = prefs.getInt(KEY_DEF_CITY,  0);
    default_param_index = prefs.getInt(KEY_DEF_PARAM, 0);

    prefs.end();

    if (default_city_index < 0 || default_city_index > 4) default_city_index = 0;
    if (default_param_index < 0 || default_param_index > 3) default_param_index = 0;

    Serial.printf("Loaded defaults: city=%d, param=%d\n",
                  default_city_index, default_param_index);
}

static void save_defaults_to_nvm()
{
    if (!prefs.begin(NVM_NAMESPACE, false)) { // false = read/write
        Serial.println("Preferences: failed to begin (write)");
        return;
    }

    prefs.putInt(KEY_DEF_CITY,  current_city_index);
    prefs.putInt(KEY_DEF_PARAM, current_param_index);

    prefs.end();

    default_city_index  = current_city_index;
    default_param_index = current_param_index;

    Serial.printf("Saved defaults: city=%d, param=%d\n",
                  default_city_index, default_param_index);
}

static void apply_defaults_to_selection()
{
    // Använd sparade värden
    current_city_index  = default_city_index;
    current_param_index = default_param_index;

    Serial.printf("Applying defaults: city=%d, param=%d\n",
                  current_city_index, current_param_index);

    // Uppdatera dropdowns om de finns
    if (dd_param) {
        lv_dropdown_set_selected(dd_param, current_param_index);
    }
    if (dd_city) {
        lv_dropdown_set_selected(dd_city, current_city_index);
    }

    // Uppdatera Tile 2 (prognos)
    update_t2_with_weather();

    // Ladda historiska data för Tile 3
    if (!load_historical_data_from_smhi()) {
        Serial.println("Falling back to dummy data after applying defaults");
        fill_dummy_historical_data();
    }
    t3_bind_data_to_ui();
}

// ------------------------------------------------------
// Knappar på Tile 4 (spara/använd default)
// ------------------------------------------------------
static void save_default_btn_event_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    save_defaults_to_nvm();
    Serial.println("Default settings saved.");
}

static void load_default_btn_event_cb(lv_event_t* e)
{
    LV_UNUSED(e);
    apply_defaults_to_selection();
    Serial.println("Default settings applied.");
}

// ------------------------------------------------------
// UI setup
// ------------------------------------------------------
static void create_ui()
{
    // Create full screen tileview container
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

    // Four tiles horizontally (0,1,2,3)
    t1 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
    t2 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);
    t3 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);
    t4 = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_HOR);

    // --------------------------------------------------
    // Tile 1 – Information screen
    // --------------------------------------------------
    {
        t1_label = lv_label_create(t1);
        lv_label_set_text_fmt(t1_label,
                              "Weather App\n%s\n%s",
                              PROGRAM_VERSION,
                              GROUP_NUMBER);
        lv_obj_set_style_text_font(t1_label, &lv_font_montserrat_28, 0);
        lv_obj_center(t1_label);

        apply_tile_colors(t1, t1_label, false);
    }

    // --------------------------------------------------
    // Tile 2 – Forecast screen
    // --------------------------------------------------
    {
        t2_label = lv_label_create(t2);
        lv_label_set_text(t2_label, "Loading weather...");
        lv_obj_set_style_text_font(t2_label, &lv_font_montserrat_22, 0);
        lv_obj_align(t2_label, LV_ALIGN_TOP_MID, 0, 5);

        apply_tile_colors(t2, t2_label, false);
        create_forecast_table(t2);   // Forecast table now belongs to Tile 2
    }

// --------------------------------------------------
// Tile 3 – Historical Data
// --------------------------------------------------
{
    // Prepare base style
    lv_obj_t* tmp_label = lv_label_create(t3);
    lv_label_set_text(tmp_label, "");
    apply_tile_colors(t3, tmp_label, false);
    lv_obj_del(tmp_label);

    // Hämta displaybredd för att ge plats åt Y-axelns labels
    lv_coord_t scr_w = lv_disp_get_hor_res(NULL);

    // Temperature chart
    t3_chart = lv_chart_create(t3);
    // NYTT: lämna ~40 px marginal till vänster så siffrorna syns
    lv_obj_set_size(t3_chart, scr_w - 40, lv_pct(65));
    lv_obj_align(t3_chart, LV_ALIGN_TOP_LEFT, 40, 50);

    // --- Y-axel-etikett (enhet) ---
    t3_label_yaxis = lv_label_create(t3);
    lv_label_set_text(t3_label_yaxis, "Value");  // uppdateras senare av update_axis_labels()
    lv_obj_set_style_text_font(t3_label_yaxis, &lv_font_montserrat_20, 0);
    // Placera enheten under grafen, till vänster
    lv_obj_align_to(t3_label_yaxis, t3_chart, LV_ALIGN_OUT_BOTTOM_LEFT, 5, 10);

    // --- X-axel-huvudetikett ---
    t3_label_xaxis = lv_label_create(t3);
    lv_label_set_text(t3_label_xaxis, "Date");
    lv_obj_set_style_text_font(t3_label_xaxis, &lv_font_montserrat_20, 0);
    lv_obj_align_to(t3_label_xaxis, t3_chart, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_chart_set_type(t3_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(t3_chart, 4, 4);
    lv_chart_set_update_mode(t3_chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_range(t3_chart, LV_CHART_AXIS_PRIMARY_Y, -10, 30);

    // NYTT: lite mindre inre padding nu när vi har yttre marginal
    lv_obj_set_style_pad_left(t3_chart, 40, 0);

    // Aktivera skalsiffror på Y-axeln
    lv_chart_set_axis_tick(
        t3_chart,
        LV_CHART_AXIS_PRIMARY_Y,
        10, 5,   // längd på stora/små streck
        6, 2,    // antal stora/små intervall
        true,    // rita labels
        50       // utrymme för labels
    );

    t3_series = lv_chart_add_series(t3_chart, lv_color_black(), LV_CHART_AXIS_PRIMARY_Y);
    // Mean value line
    t3_series_mean = lv_chart_add_series(
        t3_chart,
        lv_color_make(200, 0, 0),  
        LV_CHART_AXIS_PRIMARY_Y
    );

    lv_obj_t* title = lv_label_create(t3);
    lv_label_set_text(title, "Historical Data");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_22, 0);
    lv_obj_align_to(title, t3_chart, LV_ALIGN_OUT_TOP_LEFT, 5, -10);

    // Mean label
    t3_mean_label = lv_label_create(t3);
    lv_label_set_text(t3_mean_label, "Mean: --");
    lv_obj_set_style_text_font(t3_mean_label, &lv_font_montserrat_20, 0);
    lv_obj_align_to(t3_mean_label, t3_chart, LV_ALIGN_OUT_TOP_RIGHT, -5, -10);

    // Data position slider
    t3_slider = lv_slider_create(t3);
    lv_obj_set_size(t3_slider, lv_pct(80), 35);
    lv_obj_align(t3_slider, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_set_style_pad_all(t3_slider, 8, LV_PART_KNOB);
    lv_slider_set_range(t3_slider, 0, 100);
    lv_obj_add_event_cb(t3_slider, t3_slider_event_cb, LV_EVENT_ALL, NULL);

    // NYTT: info-label under slidern som visar datum + värde
    t3_slider_info = lv_label_create(t3);
    lv_label_set_text(t3_slider_info, "");
    lv_obj_set_style_text_font(t3_slider_info, &lv_font_montserrat_18, 0);
    lv_obj_align_to(t3_slider_info, t3_slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
}

        // --------------------------------------------------
    // Tile 4 – Settings Screen
    // --------------------------------------------------
    {
        lv_obj_t* label = lv_label_create(t4);
        lv_label_set_text(label, "Customize the historical graph");
        lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);
        apply_tile_colors(t4, label, /*dark=*/false);

        // Dropdown menu to customize weather parameters
        dd_param = create_dropdown_menu(t4);
        lv_dropdown_set_options(dd_param, "Temperature\nHumidity\nWind speed\nAir pressure");
        lv_obj_align(dd_param, LV_ALIGN_CENTER, -100, 0);
        lv_obj_set_user_data(dd_param, (void*)1);

        // Dropdown menu to choose city
        dd_city = create_dropdown_menu(t4);
        lv_dropdown_set_options(dd_city, "Karlskrona\nStockholm\nGothenburg\nMalmo\nKiruna");
        lv_obj_align(dd_city, LV_ALIGN_CENTER, 100, 0);
        lv_obj_set_user_data(dd_city, (void*)2);

        // --- Button: Save current as default ---
        lv_obj_t* btn_save = lv_btn_create(t4);
        lv_obj_set_size(btn_save, 180, 50);
        lv_obj_align(btn_save, LV_ALIGN_BOTTOM_LEFT, 20, -20);
        lv_obj_add_event_cb(btn_save, save_default_btn_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t* lbl_save = lv_label_create(btn_save);
        lv_label_set_text(lbl_save, "Save default");
        lv_obj_center(lbl_save);

        // --- Button: Use default ---
        lv_obj_t* btn_load = lv_btn_create(t4);
        lv_obj_set_size(btn_load, 180, 50);
        lv_obj_align(btn_load, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
        lv_obj_add_event_cb(btn_load, load_default_btn_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t* lbl_load = lv_label_create(btn_load);
        lv_label_set_text(lbl_load, "Use default");
        lv_obj_center(lbl_load);
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
// SMHI → T1 label + 7-day table
// ------------------------------------------------------
static void update_t2_with_weather()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("update_t1_with_weather: WiFi not connected");
        lv_label_set_text(t2_label, "WiFi fail");
        lv_obj_align(t2_label, LV_ALIGN_TOP_MID, 0, 5);
        return;
    }

    HTTPClient http;

    // Build forecast URL for the currently selected city using lat/lon
    String url =
        String("https://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/")
        + "geotype/point/lon/" + String(CITY_LON[current_city_index], 4)
        + "/lat/" + String(CITY_LAT[current_city_index], 4)
        + "/data.json";

    Serial.print("Requesting SMHI forecast: ");
    Serial.println(url);

    http.setReuse(false);
    http.useHTTP10(true);
    http.begin(url);
    http.addHeader("Accept-Encoding", "identity");


    // you can keep this or drop it; SMHI should return plain JSON anyway
    http.addHeader("Accept-Encoding", "identity");

    int httpCode = http.GET();

    Serial.print("HTTP code: ");
    Serial.println(httpCode);

    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP error: %d\n", httpCode);
        lv_label_set_text(t2_label, "HTTP error");
        lv_obj_align(t2_label, LV_ALIGN_TOP_MID, 0, 5);
        http.end();
        return;
    }

    WiFiClient& stream = http.getStream();

    DynamicJsonDocument doc(220000);
    DeserializationError err = deserializeJson(doc, stream);
    http.end();

    if (err) {
        Serial.print("JSON error (stream): ");
        Serial.println(err.c_str());

        char buf[64];
        snprintf(buf, sizeof(buf), "JSON err: %s", err.c_str());
        lv_label_set_text(t2_label, buf);
        lv_obj_align(t2_label, LV_ALIGN_TOP_MID, 0, 5);
        return;
    }

    Serial.println("JSON parsed OK (stream)");

    // --- Fill our 7-day forecast model + table ---
    update_forecast_from_smhi(doc);
    refresh_forecast_table();

    // --- Also keep the single-line label with current conditions ---
    JsonObject first;
    JsonArray  timeSeries = doc["timeSeries"];

    if (!timeSeries.isNull() && timeSeries.size() > 0) {
        Serial.println("Using doc[\"timeSeries\"][0] for label");
        first = timeSeries[0];
    } else {
        Serial.println("No timeSeries array, using root object as forecast for label");
        first = doc.as<JsonObject>();
    }

    const char* validTime = first["validTime"] | "N/A";
    (void)validTime; // not used right now

    float temp   = NAN;
    float precip = NAN;

    JsonArray params = first["parameters"];
    if (params.isNull()) {
        Serial.println("parameters missing");
        lv_label_set_text(t2_label, "No params");
        lv_obj_align(t2_label, LV_ALIGN_TOP_MID, 0, 5);
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
    snprintf(buf, sizeof(buf), "%s %.1f°C / %.1f mm",
            get_current_city_name(), temp, precip);

    Serial.print("Setting  label: ");
    Serial.println(buf);

    lv_label_set_text(t2_label, buf);
    lv_obj_align(t2_label, LV_ALIGN_TOP_MID, 0, 5);

}


// ------------------------------------------------------
// Arduino setup / loop
// ------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    Serial.println("BOOTING...");
    delay(200);

    if (!amoled.begin()) {
        Serial.println("Failed to init LilyGO AMOLED.");
        while (true) delay(1000);
    }

    beginLvglHelper(amoled);   // init LVGL för detta kort

    // 1) Läs sparade default-inställningar från flash
    load_defaults_from_nvm();

    // 2) Skapa UI
    create_ui();

    // 3) Koppla upp WiFi
    connect_wifi();

    // 4) Applicera defaultval (sätter dropdowns + hämtar väder + historik)
    apply_defaults_to_selection();
}

void loop()
{
    lv_timer_handler();
    delay(5);
}
