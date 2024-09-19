#include "Sensor.h"
#include <Wire.h>

Sensor::Sensor() {}

void Sensor::attach(uint8_t pin) {
  _pin = pin;
  pinMode(_pin, INPUT_PULLUP);
}

void Sensor::update() {  
  oldState = currentState;
  currentState = digitalRead(_pin);
}

bool Sensor::read() {
  return currentState;
}

bool Sensor::isTripped() {
  if (oldState == HIGH && currentState == LOW)
  {
    return true;
  }
  return false;
}

bool Sensor::isUntripped() {
  if (oldState == LOW && currentState == HIGH)
  {
    return true;
  }
  return false;
}

void Sensor::count() {
  _count += 1;
}

void Sensor::count(int count) {
  _count = count;
}

int Sensor::getCount() {
  return _count;
}
