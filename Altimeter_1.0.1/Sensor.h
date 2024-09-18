#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

class Sensor {
public:
    Sensor();

    // Attaches the sensor to a pin (for direct Arduino pin)
    void attach(uint8_t pin);

    // Reads the current state of the sensor (HIGH/LOW)
    bool read();

    // Checks if the sensor was tripped (transition from HIGH to LOW)
    bool isTripped();
    bool isTripped(bool updateState);

    // Checks if the sensor was untripped (transition from LOW to HIGH)
    bool isUntripped();
    bool isUntripped(bool updateState);

    // Increments the internal count
    void count();

    // Sets the internal count to a specific value
    void count(int num);

    // Gets the internal count
    int getCount();

private:
    uint8_t _pin;                    // The pin number on Arduino or PCF8574
    bool storedState;                // Stores the last read state
    bool currentState;               // Stores the current state
    int _count = 0;                  // Counter for tripped events


};

#endif // SENSOR_H
