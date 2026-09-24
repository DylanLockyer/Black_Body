#include "main.h"
#include "SegmentedCalibration.h"
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

//Wifi initilizations
IPAddress apIP(192,168,4,1);
IPAddress subnet(255,255,255,0);

DNSServer dns;
AsyncWebServer server(80);

SensorReading latestReading;

// Heater control state / feedback. Guarded by dataMutex, same as
// latestReading/gCal (see the FreeRTOS task / synchronization note
// below for why).
HeaterReading latestHeaterReading;
HeaterControl gHeater;

// DAC initilization
DAC60501 dac(I2C_ADDRESS_SCL);

// SPI communication with temp probe
STM32Sensor sensor(SPI_CS);

// ---------------------------------------------------------------------
// Profile / calibration state
// ---------------------------------------------------------------------
// A "profile" is a saved calibration: a small set of piecewise cubic
// segments (see SegmentedCalibration.h) mapping resistance -> temperature,
// e.g. transcribed/interpolated from a manufacturer data sheet such as the
// GR-300-AA table. Unlike an on-device least-squares fit, every
// coefficient and every segment's resistance range is set directly and
// edited as-is from the web UI, so live resistance readings can be
// converted to temperature locally without needing the browser to do any
// math.

static const char *PROFILES_DIR = "/profiles";
static const char *ACTIVE_PROFILE_FILE = "/active_profile.txt";

// Default calibration for a GR-300-AA-style sensor: 3 piecewise cubic
// segments in ln(R)->ln(T), matching the manufacturer's R-T table to
// within ~0.001 K. Seeded as the "GR-300-AA" profile on first boot only
// (see seedDefaultProfileIfMissing); fully editable afterward.
static const char *DEFAULT_PROFILE_NAME = "GR-300-AA";
static const CalSegment DEFAULT_CAL_SEGMENTS[3] = {
    // R >= 248.8 ohm (T <= 2 K)
    { 248.8f, 1.0e6f, -0.0091414284f, 0.2467387139f, -2.5443885697f, 8.7550629529f },
    // 33.2 ohm <= R < 248.8 ohm (2 K <= T <= 10 K)
    { 33.2f, 248.8f, 0.027577728f, -0.3426015171f, 0.5804353397f, 3.2875860941f },
    // R < 33.2 ohm (T >= 10 K)
    { 0.0f, 33.2f, -0.0910341581f, 0.5621404413f, -1.92412108f, 6.0572837321f },
};

SegmentedCalibration gCal;
String gActiveProfileName = "";

// ---------------------------------------------------------------------
// FreeRTOS task / synchronization
// ---------------------------------------------------------------------
// The STM32 SPI read (sensor.read()) can block for a little while waiting
// on the STM32 to clock data out. Doing that inline in Arduino's loop()
// (which shares a core with, and can starve, WiFi/TCP servicing) was
// causing the softAP to drop clients. sensorTask now owns all SPI polling
// and runs on its own core, so it can never stall WiFi or the DNS captive
// portal responder in loop(). dataMutex guards every field that both
// sensorTask (writer) and the async web server's request handlers
// (readers/writers, running on a different task) touch: latestReading,
// gCal, and gActiveProfileName.
static TaskHandle_t sensorTaskHandle = nullptr;
static TaskHandle_t heaterTaskHandle = nullptr;
static SemaphoreHandle_t dataMutex = nullptr;
static void sensorTask(void *pvParameters);
static void heaterTask(void *pvParameters);

// ---------------------------------------------------------------------
// Heater PID gain persistence
// ---------------------------------------------------------------------
// Only Kp/Ki/Kd are persisted (as "kp,ki,kd"); target temperature and
// manual-override state are intentionally session-only, per the web UI's
// requirements.
static const char *PID_CONFIG_FILE = "/pid_config.txt";

static void savePidConfig(float kp, float ki, float kd) {
  File f = LittleFS.open(PID_CONFIG_FILE, "w");
  if (!f) return;
  f.printf("%.6f,%.6f,%.6f", kp, ki, kd);
  f.close();
}

static bool loadPidConfig(float &kp, float &ki, float &kd) {
  File f = LittleFS.open(PID_CONFIG_FILE, "r");
  if (!f) return false;
  String line = f.readString();
  f.close();
  int c1 = line.indexOf(',');
  int c2 = (c1 == -1) ? -1 : line.indexOf(',', c1 + 1);
  if (c1 == -1 || c2 == -1) return false;
  kp = line.substring(0, c1).toFloat();
  ki = line.substring(c1 + 1, c2).toFloat();
  kd = line.substring(c2 + 1).toFloat();
  return true;
}

// Only called from setup(), before heaterTask is created, so no locking needed.
static void restoreHeaterConfigOnBoot() {
  float kp, ki, kd;
  if (loadPidConfig(kp, ki, kd)) {
    gHeater.pidKp = kp;
    gHeater.pidKi = ki;
    gHeater.pidKd = kd;
  }
}

static String profileSegPath(const String &name) {
  return String(PROFILES_DIR) + "/" + name + ".seg";
}

// Validates and persists a segment set as profile `name`. Returns true on
// success (segments rejected as invalid are never written).
static bool saveSegmentsForProfile(const String &name, const CalSegment *segments, int numSegments) {
  SegmentedCalibration cal;
  if (!cal.setSegments(segments, numSegments)) return false;

  char buf[512];
  size_t written = cal.serialize(buf, sizeof(buf));
  if (written == 0) return false;

  File f = LittleFS.open(profileSegPath(name), "w");
  if (!f) return false;
  f.print(buf);
  f.close();
  return true;
}

static bool loadCalibrationForProfile(const String &name) {
  File f = LittleFS.open(profileSegPath(name), "r");
  if (!f) return false;
  String contents = f.readString();
  f.close();
  return gCal.deserialize(contents.c_str());
}

// Called from the async web server's request-handler context (a different
// task than sensorTask), so gActiveProfileName/gCal are updated under
// dataMutex — sensorTask reads both on every SPI sample.
static void setActiveProfile(const String &name) {
  if (dataMutex != nullptr && xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
    gActiveProfileName = name;
    loadCalibrationForProfile(name); // ok if it fails; gCal just stays invalid
    xSemaphoreGive(dataMutex);
  } else {
    // dataMutex not created yet (e.g. called during early boot) — safe to
    // touch directly since sensorTask can't be running yet at that point.
    gActiveProfileName = name;
    loadCalibrationForProfile(name);
  }
  File f = LittleFS.open(ACTIVE_PROFILE_FILE, "w");
  if (f) {
    f.print(name);
    f.close();
  }
}

// Only called from setup(), before sensorTask is created, so no locking
// needed here.
static void restoreActiveProfileOnBoot() {
  File f = LittleFS.open(ACTIVE_PROFILE_FILE, "r");
  if (!f) return;
  String name = f.readString();
  f.close();
  name.trim();
  if (name.length() > 0) {
    gActiveProfileName = name;
    loadCalibrationForProfile(name);
  }
}

// Creates the built-in "GR-300-AA" profile and marks it active, but only on
// a device's very first boot (i.e. no active-profile record exists yet).
// Gives the device a working calibration out of the box while leaving
// every later boot's user-edited profiles untouched.
static void seedDefaultProfileIfMissing() {
  if (LittleFS.exists(ACTIVE_PROFILE_FILE)) return;
  if (!saveSegmentsForProfile(DEFAULT_PROFILE_NAME, DEFAULT_CAL_SEGMENTS, 3)) return;
  File f = LittleFS.open(ACTIVE_PROFILE_FILE, "w");
  if (f) {
    f.print(DEFAULT_PROFILE_NAME);
    f.close();
  }
}

// Parses a JSON body of the form {"segments":[{"rMin":..,"rMax":..,
// "c0":..,"c1":..,"c2":..,"c3":..}, ...]} as sent by the web UI's segment
// table. Segments with a missing/non-numeric field or rMax <= rMin are
// silently skipped. Returns the number of segments parsed into `out`
// (capped at maxSegments), or 0 on a malformed body.
static int parseSegmentsJson(const char *json, CalSegment *out, int maxSegments) {
  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) return 0;

  JsonArrayConst segs = doc["segments"].as<JsonArrayConst>();
  if (segs.isNull()) return 0;

  int n = 0;
  for (JsonObjectConst seg : segs) {
    if (n >= maxSegments) break;
    if (!seg["rMin"].is<float>() || !seg["rMax"].is<float>() ||
        !seg["c0"].is<float>() || !seg["c1"].is<float>() ||
        !seg["c2"].is<float>() || !seg["c3"].is<float>()) {
      continue;
    }
    float rMin = seg["rMin"];
    float rMax = seg["rMax"];
    if (!(rMax > rMin)) continue;

    out[n].rMin = rMin;
    out[n].rMax = rMax;
    out[n].c0 = seg["c0"];
    out[n].c1 = seg["c1"];
    out[n].c2 = seg["c2"];
    out[n].c3 = seg["c3"];
    n++;
  }
  return n;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  // Created before anything else touches latestReading/gCal.
  dataMutex = xSemaphoreCreateMutex();

  dac.init(SDA_PIN, SCL_PIN, STANDARD_MODE);

  // Set to zero
  set_output_current(0);
  
  // Initilize wifi
  wifi_setup();

  // Restore last-used calibration profile / fit, if any.
  restoreActiveProfileOnBoot();

  // Restore persisted PID gains, if any (target temp / manual override are
  // intentionally not persisted).
  restoreHeaterConfigOnBoot();

  // Initilize temperature probe SPI read
  sensor.begin();

  // Run SPI polling entirely on its own FreeRTOS task, pinned to core 0
  // (Arduino's setup()/loop() and the WiFi/lwIP stack effectively live on
  // core 1 by default). This is what stops a slow/blocking SPI transfer
  // from ever delaying dns.processNextRequest() or WiFi's own servicing —
  // the two are now fully independent tasks instead of sharing one loop.
  xTaskCreatePinnedToCore(
    sensorTask,        // task function
    "SensorTask",      // name (for debugging, e.g. in a task list dump)
    4096,               // stack size, in bytes
    nullptr,            // parameters
    1,                  // priority (above idle, below anything time-critical)
    &sensorTaskHandle,
    0                   // core 0
  );

  // Heater PID / manual-override control loop. Runs on core 1 (alongside
  // loop()'s networking housekeeping) at a modest 5 Hz, separate from
  // sensorTask's tight SPI polling loop on core 0.
  xTaskCreatePinnedToCore(
    heaterTask,
    "HeaterTask",
    4096,
    nullptr,
    1,
    &heaterTaskHandle,
    1                   // core 1
  );
}

void loop() {
  // loop() now does networking/captive-portal housekeeping ONLY. All SPI
  // sensor polling happens in sensorTask (see setup() and sensorTask()
  // below), so nothing here can ever be blocked by a slow SPI transfer.
  dns.processNextRequest();
  vTaskDelay(pdMS_TO_TICKS(10)); // yield to the scheduler; no need to spin
}

// Dedicated FreeRTOS task: polls the STM32 over SPI as fast as it can and
// publishes results into latestReading/gCal-derived temperature under
// dataMutex. Runs forever on core 0, independent of loop()/WiFi (core 1).
static void sensorTask(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    Sensor_Data data;
    if (sensor.read(&data) && data.resistance > 1.0 && data.resistance < 50000.0) {
      float temperature = NAN;

      if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        // resistance == 0 means no sensor is connected (or the reading is
        // otherwise invalid), so there's nothing to evaluate the
        // calibration against — report temperature as null rather than a
        // bogus extrapolated value.
        if (gCal.isValid() && data.resistance > 0.0f) {
          temperature = gCal.evaluate(data.resistance);
        }

        latestReading.current = data.current;
        latestReading.voltage = data.voltage;
        latestReading.resistance = data.resistance;
        // Convert resistance -> temperature locally using the active
        // calibration profile's piecewise segments, when one is loaded.
        // Falls back to NAN (reported as "temperature":null) if no
        // calibration is active yet, or the resistance falls outside every
        // configured segment, rather than silently reporting resistance as
        // if it were temperature.
        latestReading.temperature = temperature;

        // Debug-only raw fields, surfaced by the debug panel in the UI.
        latestReading.curSource = (uint8_t)data.cur_source;
        latestReading.curDirection = (uint8_t)data.current_direction;
        latestReading.shuntResistor = (uint8_t)data.shunt_resistor;

        xSemaphoreGive(dataMutex);
      }

      //Serial.print(data.resistance);
      //Serial.print(" ohm; ");
      //Serial.print(data.voltage, 6);
      //Serial.print(" mV; ");
      //Serial.print(data.current, 6);
      //Serial.println(" uA");
    }

    // sensor.read() already blocks briefly inside spi_slave_transmit's own
    // wait, but this small extra delay guarantees the task yields to the
    // scheduler on every iteration — including a run of instant
    // failed/empty reads — so it can never hog its core or trip the
    // watchdog.
    vTaskDelay(1);
  }
}

// Dedicated FreeRTOS task: reads heater board feedback (get_voltage() /
// get_current()), runs the temperature PID loop (when gHeater.running) or
// applies the manual-override current (when set and not running), and
// publishes the result via set_output_current(). Runs at a fixed ~5 Hz on
// core 1, independent of sensorTask's SPI polling on core 0.
static void heaterTask(void *pvParameters) {
  (void)pvParameters;

  const TickType_t periodTicks = pdMS_TO_TICKS(200); // 5 Hz
  float integral = 0.0f;
  float prevError = 0.0f;
  uint32_t prevTimeMs = millis();
  bool wasRunning = false;

  for (;;) {
    vTaskDelay(periodTicks);

    bool running = false, manual = false;
    float target = NAN, manualAmps = 0.0f;
    float kp = 0.0f, ki = 0.0f, kd = 0.0f;
    float currentTemp = NAN;

    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      running = gHeater.running;
      target = gHeater.targetTemperature;
      manual = gHeater.manualOverride;
      manualAmps = gHeater.manualCurrent;
      kp = gHeater.pidKp;
      ki = gHeater.pidKi;
      kd = gHeater.pidKd;
      currentTemp = latestReading.temperature;
      xSemaphoreGive(dataMutex);
    } else {
      continue;
    }

    uint32_t nowMs = millis();
    float dt = (nowMs - prevTimeMs) / 1000.0f;
    prevTimeMs = nowMs;
    if (dt <= 0.0f) dt = 0.2f;

    float outputCurrent = 0.0f;

    if (running) {
      if (!wasRunning) {
        // Just started: reset the integrator so a stale error term from
        // before doesn't cause an output kick.
        integral = 0.0f;
        prevError = 0.0f;
      }
      if (!isnan(currentTemp) && !isnan(target)) {
        float error = target - currentTemp;
        integral += error * dt;
        float derivative = (error - prevError) / dt;
        prevError = error;
        outputCurrent = kp * error + ki * integral + kd * derivative;
      }
      if (outputCurrent < 0.0f) outputCurrent = 0.0f;
      if (outputCurrent > MAX_CURRENT) outputCurrent = MAX_CURRENT;
    } else if (manual) {
      outputCurrent = manualAmps;
      if (outputCurrent < 0.0f) outputCurrent = 0.0f;
      if (outputCurrent > MAX_CURRENT) outputCurrent = MAX_CURRENT;
    }
    wasRunning = running;

    set_output_current(outputCurrent);

    float measuredVoltage = get_voltage();
    float measuredCurrent = get_current();

    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      latestHeaterReading.voltage = measuredVoltage;
      latestHeaterReading.current = measuredCurrent;
      latestHeaterReading.outputCurrent = outputCurrent;
      xSemaphoreGive(dataMutex);
    }
  }
}


void wifi_setup(){
  // LITTLEFS code
  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  if (!LittleFS.exists(PROFILES_DIR)) {
    LittleFS.mkdir(PROFILES_DIR);
  }
  seedDefaultProfileIfMissing();

  // Initialize wifi
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, subnet);
  WiFi.softAP(WIFI_NAME);
  WiFi.setSleep(false);

  // Initialize dns
  dns.start(53, "*", apIP);

  // Serve the actual portal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/website.html", "text/html");
  });

  // Android
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("http://192.168.4.1/");
  });
  server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("http://192.168.4.1/");
  });

  // Windows
  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("http://192.168.4.1/");
  });
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("http://192.168.4.1/");
  });

  // Apple
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("http://192.168.4.1/");
  });
  server.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("http://192.168.4.1/");
  });

  // Fallback for anything else (unknown probes, random domains)
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("http://192.168.4.1/");
  });

  // Send data to website. Runs on the AsyncTCP task, a different task
  // than sensorTask, so it takes a quick snapshot under dataMutex rather
  // than reading latestReading/gCal directly.
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    SensorReading snapshot;
    HeaterReading heaterSnapshot;
    HeaterControl heaterCtrl;
    bool calValid = false;
    String activeProfile;

    if (dataMutex != nullptr && xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      snapshot = latestReading;
      heaterSnapshot = latestHeaterReading;
      heaterCtrl = gHeater;
      calValid = gCal.isValid();
      activeProfile = gActiveProfileName;
      xSemaphoreGive(dataMutex);
    } else {
      snapshot = latestReading;
      heaterSnapshot = latestHeaterReading;
      heaterCtrl = gHeater;
    }

    char buf[700];
    bool tempValid = !isnan(snapshot.temperature);
    bool targetValid = !isnan(heaterCtrl.targetTemperature);
    snprintf(buf, sizeof(buf),
      "{\"temperature\":%s,\"resistance\":%.3f,\"voltage\":%.4f,\"current\":%.4f,"
      "\"calValid\":%s,\"activeProfile\":\"%s\","
      "\"curSource\":%u,\"curDirection\":%u,\"shuntResistor\":%u,"
      "\"heaterRunning\":%s,\"targetTemperature\":%s,"
      "\"manualOverride\":%s,\"manualCurrent\":%.4f,"
      "\"pidKp\":%.6f,\"pidKi\":%.6f,\"pidKd\":%.6f,"
      "\"heaterVoltage\":%.4f,\"heaterCurrent\":%.4f,\"heaterOutputCurrent\":%.4f,"
      "\"heaterMaxCurrent\":%.4f}",
      tempValid ? String(snapshot.temperature, 4).c_str() : "null",
      snapshot.resistance,
      snapshot.voltage, snapshot.current,
      calValid ? "true" : "false",
      activeProfile.c_str(),
      snapshot.curSource, snapshot.curDirection, snapshot.shuntResistor,
      heaterCtrl.running ? "true" : "false",
      targetValid ? String(heaterCtrl.targetTemperature, 3).c_str() : "null",
      heaterCtrl.manualOverride ? "true" : "false",
      heaterCtrl.manualCurrent,
      heaterCtrl.pidKp, heaterCtrl.pidKi, heaterCtrl.pidKd,
      heaterSnapshot.voltage, heaterSnapshot.current, heaterSnapshot.outputCurrent,
      (double)MAX_CURRENT);
    request->send(200, "application/json", buf);
  });

  // ---------------- Heater control endpoints ----------------

  server.on("/heater/start", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (dataMutex != nullptr && xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      gHeater.manualOverride = false; // PID takes priority over manual override
      gHeater.running = true;
      xSemaphoreGive(dataMutex);
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/heater/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (dataMutex != nullptr && xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      gHeater.running = false;
      xSemaphoreGive(dataMutex);
    }
    request->send(200, "text/plain", "OK");
  });

  // POST /heater/target?value=<K> -- session-only, not persisted.
  server.on("/heater/target", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("value")) {
      request->send(400, "text/plain", "missing value");
      return;
    }
    float v = request->getParam("value")->value().toFloat();
    if (dataMutex != nullptr && xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      gHeater.targetTemperature = v;
      xSemaphoreGive(dataMutex);
    }
    request->send(200, "text/plain", "OK");
  });

  // POST /heater/pid?kp=&ki=&kd= -- applied immediately and persisted to LittleFS.
  server.on("/heater/pid", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("kp") || !request->hasParam("ki") || !request->hasParam("kd")) {
      request->send(400, "text/plain", "missing kp/ki/kd");
      return;
    }
    float kp = request->getParam("kp")->value().toFloat();
    float ki = request->getParam("ki")->value().toFloat();
    float kd = request->getParam("kd")->value().toFloat();
    if (dataMutex != nullptr && xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      gHeater.pidKp = kp;
      gHeater.pidKi = ki;
      gHeater.pidKd = kd;
      xSemaphoreGive(dataMutex);
    }
    savePidConfig(kp, ki, kd);
    request->send(200, "text/plain", "OK");
  });

  // POST /heater/manual?enabled=0|1&current=<A> -- only takes effect while
  // the PID loop is stopped; rejected (409) otherwise.
  server.on("/heater/manual", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("enabled") || !request->hasParam("current")) {
      request->send(400, "text/plain", "missing enabled/current");
      return;
    }
    bool enabled = request->getParam("enabled")->value().toInt() != 0;
    float current = request->getParam("current")->value().toFloat();
    if (current < 0.0f) current = 0.0f;
    if (current > MAX_CURRENT) current = MAX_CURRENT;

    bool ok = false;
    if (dataMutex != nullptr && xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      if (!gHeater.running) {
        gHeater.manualOverride = enabled;
        gHeater.manualCurrent = current;
        ok = true;
      }
      xSemaphoreGive(dataMutex);
    }
    request->send(ok ? 200 : 409, "text/plain", ok ? "OK" : "PID is running");
  });

  // ---------------- Profile management endpoints ----------------

  // GET /profiles -> {"profiles":["name1","name2",...]}
  server.on("/profiles", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"profiles\":[";
    File dir = LittleFS.open(PROFILES_DIR);
    bool first = true;
    if (dir && dir.isDirectory()) {
      File entry = dir.openNextFile();
      while (entry) {
        String name = String(entry.name());
        // Only list the segment files; strip the directory prefix and extension.
        int slash = name.lastIndexOf('/');
        if (slash != -1) name = name.substring(slash + 1);
        if (name.endsWith(".seg")) {
          name = name.substring(0, name.length() - 4);
          if (!first) json += ",";
          json += "\"" + name + "\"";
          first = false;
        }
        entry = dir.openNextFile();
      }
    }
    json += "]}";
    request->send(200, "application/json", json);
  });

  // GET /profile/load?name=X -> {"segments":[{"rMin":..,"rMax":..,"c0":..,"c1":..,"c2":..,"c3":..},...]}
  server.on("/profile/load", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("name")) {
      request->send(400, "text/plain", "missing name");
      return;
    }
    String name = request->getParam("name")->value();
    File f = LittleFS.open(profileSegPath(name), "r");
    if (!f) {
      request->send(404, "text/plain", "not found");
      return;
    }
    String contents = f.readString();
    f.close();

    SegmentedCalibration cal;
    if (!cal.deserialize(contents.c_str())) {
      request->send(500, "text/plain", "corrupt profile");
      return;
    }

    JsonDocument doc;
    JsonArray segs = doc["segments"].to<JsonArray>();
    for (int i = 0; i < cal.numSegments(); i++) {
      const CalSegment &s = cal.segments()[i];
      JsonObject o = segs.add<JsonObject>();
      o["rMin"] = s.rMin;
      o["rMax"] = s.rMax;
      o["c0"] = s.c0;
      o["c1"] = s.c1;
      o["c2"] = s.c2;
      o["c3"] = s.c3;
    }
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  // POST /profile/delete?name=X -> removes the profile's segment file
  server.on("/profile/delete", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("name")) {
      request->send(400, "text/plain", "missing name");
      return;
    }
    String name = request->getParam("name")->value();
    LittleFS.remove(profileSegPath(name));
    if (gActiveProfileName == name) {
      if (dataMutex != nullptr && xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        gActiveProfileName = "";
        gCal.reset();
        xSemaphoreGive(dataMutex);
      }
      LittleFS.remove(ACTIVE_PROFILE_FILE);
    }
    request->send(200, "text/plain", "OK");
  });

  // POST /profile/save?name=X, JSON body = {"segments":[...]} as built by
  // the web UI's segment table. Buffers the body in request->_tempObject
  // (freed automatically by ESPAsyncWebServer once the request is done),
  // then once fully received, validates/stores the segments and marks this
  // profile as active.
  server.on(
    "/profile/save", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      // Final response is sent from the body handler once the segments
      // have been parsed, so nothing to do here unless params are missing.
      if (!request->hasParam("name")) {
        request->send(400, "text/plain", "missing name");
      }
    },
    nullptr,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!request->hasParam("name")) return;

      if (request->_tempObject == nullptr) {
        request->_tempObject = calloc(total + 1, sizeof(uint8_t));
        if (request->_tempObject == nullptr) {
          request->send(500, "text/plain", "out of memory");
          return;
        }
      }
      memcpy((uint8_t *)request->_tempObject + index, data, len);

      if (index + len != total) return; // wait for the rest of the body

      String name = request->getParam("name")->value();
      const char *body = (const char *)request->_tempObject;

      CalSegment segments[CAL_MAX_SEGMENTS];
      int numSegments = parseSegmentsJson(body, segments, CAL_MAX_SEGMENTS);

      bool ok = numSegments > 0 && saveSegmentsForProfile(name, segments, numSegments);
      if (ok) setActiveProfile(name); // loads what we just wrote

      String resp = String("{\"ok\":") + (ok ? "true" : "false") + "}";
      request->send(200, "application/json", resp);
    });

  server.begin();
}


// Static private arduino style map helper function
float map(float x, float x_min, float x_max, float y_min, float y_max){
    return (x - x_min) * (y_max - y_min) / (x_max - x_min) + y_min;
}

// Returns current read from pcb
float get_current(){
  float tmp = (float)analogRead(current_pin);
  return map(tmp, 0, 4095, 0, 0.66);
}

// Returns voltage read from pcb
float get_voltage(){
  float v_high = (float)analogRead(voltage_pin_high);
  float v_low = (float)analogRead(voltage_pin_low);

  return map(v_high, 0, 4095, 0, HEATER_VOLTAGE_FULL_SCALE) - map(v_low, 0, 4095, 0, HEATER_VOLTAGE_FULL_SCALE);
}

// Sets output current to a max of MAX_CURRENT (0.5A)
void set_output_current(float current){
  if (current > 0.5) current = MAX_CURRENT;
  float cur_voltage = map(current, 0.0, MAX_CURRENT, 0.0, 2.5);
  dac.set_output_voltage(cur_voltage, ADC_MAX_VOLTAGE);

}