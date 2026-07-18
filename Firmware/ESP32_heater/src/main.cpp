#include "main.h"

DAC60501 dac(I2C_ADDRESS_SCL);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);

  dac.init(SDA_PIN, SCL_PIN, STANDARD_MODE);

  // test littlefs
  littlefs_setup();
}

void loop() {
  // put your main code here, to run repeatedly:


  //int current = analogRead(current_pin);
  //int voltage = analogRead(voltage_pin);
  //dac.set_output_voltage(0.86, ADC_MAX_VOLTAGE);
  delay(1000);
}


void littlefs_setup(){
  // LITTLEFS code
  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  File file = LittleFS.open("/website.html", "r");
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  
  Serial.println("File Content:");
  while(file.available()){
    Serial.print(file.read());
  }
  file.close();

}