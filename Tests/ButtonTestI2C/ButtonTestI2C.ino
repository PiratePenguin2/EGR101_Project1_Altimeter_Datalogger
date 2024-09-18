#include <Wire.h>
#include "Sensor.h"

Sensor btn1;
Sensor btn2;

void setup() {
  // put your setup code here, to run once:
  btn1.attach(34);
  btn2.attach(36);
}

void loop() {
  // put your main code here, to run repeatedly:

}
