#include "Sensor.h"
#include <Wire.h>

Sensor::Sensor() {}

void Sensor::attach(uint8_t pin) {
  _pin = pin;
  pinMode(_pin, INPUT_PULLUP);
}

bool Sensor::read() {  
  storedState = digitalRead(_pin);
  return storedState;
}

bool Sensor::isTripped()
{
  bool oldState = storedState;
  bool newState = this->read();
  if (oldState == LOW && newState == HIGH)
  {
    //this->count();
    return true;
  }
  return false;
}

bool Sensor::isUntripped()
{
  bool oldState = storedState;
  bool newState = this->read();
  if (oldState == HIGH && newState == LOW)
  {
    //this->count();
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
