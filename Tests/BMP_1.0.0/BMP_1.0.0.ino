#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP3XX.h>

// Create an instance of the sensor
Adafruit_BMP3XX bmp;

// Set I2C address (default is 0x77, but some sensors might be 0x76)
#define BMP_I2C_ADDRESS 0x77

void setup() {
  // Start serial communication
  Serial.begin(115200);
  while (!Serial);

  // Initialize I2C communication
  if (!bmp.begin(BMP_I2C_ADDRESS)) {
    Serial.println("Could not find a valid BMP388 sensor, check wiring!");
    while (1);
  }

  // Set up the sensor for reading pressure and temperature
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);
}

void loop() {
  // Check if the sensor can be read
  if (!bmp.performReading()) {
    Serial.println("Failed to perform reading!");
    return;
  }

  // Print out temperature and pressure readings
  Serial.print("Temperature = ");
  Serial.print(bmp.temperature);
  Serial.println(" °C");

  Serial.print("Pressure = ");
  Serial.print(bmp.pressure / 100.0);  // Convert from Pa to hPa
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bmp.readAltitude(1013.25));  // Standard sea level pressure
  Serial.println(" m");

  delay(2000);  // Wait 2 seconds between readings
}
