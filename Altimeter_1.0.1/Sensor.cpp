#include "Sensor.h"
#include <Wire.h>

Sensor::Sensor() {}

void Sensor::attach(uint8_t pin) {
  _pin = pin;
  pinMode(_pin, INPUT_PULLUP);
}

bool Sensor::read() {  
  storedState = currentState;
  currentState = digitalRead(_pin);
  return currentState;
}

bool Sensor::isTripped() {
  this->read();
  if (storedState == HIGH && currentState == LOW)
  {
    //this->count();
    return true;
  }
  return false;
}

bool Sensor::isTripped(bool updateState) {
  if (updateState) {
  this->read();
  }
  if (storedState == HIGH && currentState == LOW)
  {
    //this->count();
    return true;
  }
  return false;
}

bool Sensor::isUntripped() {
  this->read();
  if (storedState == LOW && currentState == HIGH)
  {
    //this->count();
    return true;
  }
  return false;
}

bool Sensor::isUntripped(bool updateState) {
  if (updateState) {
    this->read();
  }
  if (storedState == LOW && currentState == HIGH)
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
