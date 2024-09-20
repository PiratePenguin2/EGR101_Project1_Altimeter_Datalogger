#include <SD.h>

const int chipSelect = 10;

void setup() {
  Serial.begin(9600);
  if (!SD.begin(chipSelect)) {
    Serial.println("Initialization failed!");
    return;
  }
  Serial.println("Initialization done.");
}

void loop() {
  // Your code here
}
