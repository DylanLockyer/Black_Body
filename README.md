# Purpose
The goal for this project is to control a black body heater. The black body module will be mounted to the 4K stage of the cryostat and must be heated to >40K. This is accomplished by using 4x 10ohm heaters in series (40 ohm equivalent)with a maximum heat output of 10W (20V @ 5A).

# Design choices
Overall design includes 2 PCB's (printed circuit boards) - one includes the microcontroller and heater control (voltage and current sensing, voltage PWMing), the other reads the temperature from the Germanium RTD sensor or silicone diode temperature sensor. 

### Microcontroller
ESP32 as it has lots of pins and supports wifi so it can self host a webpage to display the readings and control in an easy-to-use way. See below example of an microcontroller self hosted control website:
![Alt text](Images/Example_Website.png)

### Heater Controller
Using 4x 10ohms resistors in series gives a resistance of 40ohms total. To have a maximum output power of 10W, a peak power of 20V and 0.5A is requred. This will be mesured using a shunt resistor with amplifier and a voltage divider, both of which will be read by the ESP32's internal ADC, whose 12-bit resolution is likely sufficiently precise for the control required for this design. The max current measurement will be 0.66A with a theoretical step precision of 100uA. The max voltage measurement is 36V, giving a theoretical step of 8mV. An N-Channel mosfet rated for 60V and 3A is placed on the return side to allow PWM control to reduce the voltage. Since it is a resistive heater, a low PWM frequency could be used. The design was verified in Cadence's spice tool (it includes Texas Instruments shunt resistor measurement device) and the results of such can be seen below for a simulated 10% duty cycle on a 100hz PWM. 
<br>

![Alt text](Images/Pspice_Heater_Schematic.png)
<br>

The green pulse is the voltage the heater sees and the red is the output of the shunt resistor amplifier (current measurement).
![Alt text](Images/Pspice_Heater_Results.png)

### Power Source
The device will recieve 20V power from a power supply (either a lab adjustable power source or dedicated AC/DC power source). The ESP32 board has a buck converted to step it down to 5V at 2A. There will then be a simple LDO to convert to 3.3V to provide an even cleaner supply for the microcontroller for improved ADC fidelity. This 5V will be supplied to the temperature probe board where a low noise LDO and filtering will reduce noise for sensitive analog measurements.