#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Timer.h"
#include "Sensor.h"

#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include "SD.h"
#include "FS.h"


#define CSV_FILE_NAME "data"
#define TEXT_FILE_NAME "metadata"
#define SD_CS_PIN 5  // SD Card CS pin (adjust as per your wiring)
#define RECORDING_SLOTS 200

void setup() {
    Serial.begin(9600);

    // Initialize SD card
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        return;
    }

    Serial.println("SD card initialized.");
}

void loop() {
    
}

bool createNewRecording(String baseName) {
    for (int i = 1; i <= RECORDING_SLOTS; i++) {
        // Format folder name with leading zeros (001, 002, ...)
        String folderNumber;
        if (i < 10) {
          folderNumber = "00" + String(i);
        } else if (i < 100) {
          folderNumber = "0" + String(i);
        } else {
          folderNumber = String(i);
        }

        String folderName = baseName + folderNumber;

        // Check if recording folder can be created
        if (createRecording(folderName)) {
            Serial.println("Recording created: " + folderName);
            return true;
        }
    }

    Serial.println("TOO MANY FILES - Unable to create new recording.");
    return false;
}

bool createRecording(String folderName) {
    // Check if folder already exists
    if (SD.exists(folderName.c_str())) {
        Serial.println("Directory already exists: " + folderName);
        return false;
    }

    // Try to create the folder
    if (!SD.mkdir(folderName.c_str())) {
        Serial.println("Failed to create directory: " + folderName);
        return false;
    }

    Serial.println("Directory created: " + folderName);

    // Create and write to CSV file
    String csvFileName = folderName + "/" + CSV_FILE_NAME + ".csv";
    if (!createCSVFile(csvFileName)) {
        Serial.println("Failed to create CSV file.");
        return false;
    }

    // Create and write to metadata TXT file
    String txtFileName = folderName + "/" + TEXT_FILE_NAME + ".txt";
    if (!createTextFile(txtFileName)) {
        Serial.println("Failed to create text file.");
        return false;
    }

    return true;
}

bool createCSVFile(String csvFileName) {
    File csvFile = SD.open(csvFileName.c_str(), FILE_WRITE);
    if (csvFile) {
        csvFile.println("Time,Sensor1,Sensor2");  // CSV header
        csvFile.println("10:00,23.5,1013");       // Example data
        csvFile.println("10:05,23.6,1012");
        csvFile.println("10:10,24.5,1025");
        csvFile.close();
        Serial.println("CSV file created and data written: " + csvFileName);
        return true;
    } else {
        Serial.println("Failed to open CSV file: " + csvFileName);
        return false;
    }
}

bool createTextFile(String txtFileName) {
    File txtFile = SD.open(txtFileName.c_str(), FILE_WRITE);
    if (txtFile) {
        txtFile.println("Recording Date: 2024-09-19");
        txtFile.println("Sensor: BMP388");
        txtFile.println("Comments: Test recording with sample data.");
        txtFile.close();
        Serial.println("Text file created and metadata written: " + txtFileName);
        return true;
    } else {
        Serial.println("Failed to open text file: " + txtFileName);
        return false;
    }
}

int countRECDirectories(fs::FS &fs, const char *dirname = "/", uint8_t levels = 0) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open directory or directory is invalid.");
    return 0;
  }

  int recDirCount = 0;
  File file = root.openNextFile();
  
  while (file) {
    if (file.isDirectory()) {
      String dirName = file.name();
      if (dirName.startsWith("REC")) {
        recDirCount++;
      }
      if (levels && recDirCount > 0) {
        // Recursively count in subdirectories if needed
        recDirCount += countRECDirectories(fs, file.name(), levels - 1);
      }
    }
    file = root.openNextFile();
  }

  return recDirCount;
}
