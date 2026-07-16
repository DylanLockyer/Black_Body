#include "main.h"

DAC60501 dac(I2C_ADDRESS_SCL);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  Serial.println("Booted!!!");

  dac.init(SDA_PIN, SCL_PIN, STANDARD_MODE);
}

void loop() {
  // put your main code here, to run repeatedly:


  //int current = analogRead(current_pin);
  //int voltage = analogRead(voltage_pin);
  //dac.set_output_voltage(0.86, ADC_MAX_VOLTAGE);
  delay(1000);
}
