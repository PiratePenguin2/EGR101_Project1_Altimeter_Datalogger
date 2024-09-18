#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include <Adafruit_PCF8574.h>

class Sensor {
public:
    Sensor();

    // Attaches the sensor to a pin (for direct Arduino pin)
    void attach(uint8_t pin);

    // Attaches the sensor to a pin on a PCF8574
    void attach(uint8_t pin, Adafruit_PCF8574 &pcf);

    // Reads the current state of the sensor (HIGH/LOW)
    bool read();

    // Checks if the sensor was tripped (transition from HIGH to LOW)
    bool isTripped();

    // Checks if the sensor was untripped (transition from LOW to HIGH)
    bool isUntripped();

    // Analog read from the pin (only valid if not using PCF8574)
    int analogRead();

    // Increments the internal count
    void count();

    // Sets the internal count to a specific value
    void count(int num);

    // Gets the internal count
    int getCount();

private:
    uint8_t _pin;                    // The pin number on Arduino or PCF8574
    Adafruit_PCF8574 *_pcf;          // Pointer to the PCF8574 instance
    bool _usingPCF;                  // Whether the sensor is using PCF8574
    bool storedState;                // Stores the last read state
    int _count = 0;                  // Counter for tripped events

};

#endif // SENSOR_H
