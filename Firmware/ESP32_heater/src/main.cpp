#include "main.h"

//Wifi initilizations
IPAddress apIP(192,168,4,1);
IPAddress subnet(255,255,255,0);

DNSServer dns;
AsyncWebServer server(80);

SensorReading latestReading;

// DAC initilization
DAC60501 dac(I2C_ADDRESS_SCL);

// SPI communication with temp probe
STM32Sensor sensor(SPI_CS);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  dac.init(SDA_PIN, SCL_PIN, STANDARD_MODE);

  // Set to zero
  set_output_current(0);
  
  // Initilize wifi
  wifi_setup();

  // Initilize temperature probe SPI read
  sensor.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  dns.processNextRequest();

  // Temp probe SPI receive
  Sensor_Data data;
  sensor.read(&data);
  if (sensor.read(&data)) {
    // Send data to website
    latestReading.current = data.current;
    latestReading.voltage = data.voltage;
    latestReading.resistance = data.resistance;
    latestReading.temperature = 2.65;
  }


  // Read data for heater 
  //float heater_current = get_current();
  //float heater_voltage = get_voltage() + 0.5; // 0.5 is an offset

}


void wifi_setup(){
  // LITTLEFS code
  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

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

  // Send data to website 
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    char buf[160];
    snprintf(buf, sizeof(buf),
      "{\"temperature\":%.4f,\"resistance\":%.3f,\"voltage\":%.4f,\"current\":%.4f}",
      latestReading.temperature, latestReading.resistance,
      latestReading.voltage, latestReading.current);
    request->send(200, "application/json", buf);
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
  float tmp = (float)analogRead(voltage_pin);
  return map(tmp, 0, 4095, 0, 36.3);
}

// Sets output current to a max of MAX_CURRENT (0.5A)
void set_output_current(float current){
  if (current > 0.5) current = MAX_CURRENT;
  float cur_voltage = map(current, 0.0, MAX_CURRENT, 0.0, 2.5);
  dac.set_output_voltage(cur_voltage, ADC_MAX_VOLTAGE);

}
