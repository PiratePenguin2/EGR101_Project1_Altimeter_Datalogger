#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;  // Set to your SD card module's CS pin

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("Card Mount Failed");
    return;
  }
  
  Serial.println("SD card initialized.");

  // Try to create folder and files
  createRecording("REC_002");
}

void loop() {
  // Nothing in loop for now
}

// Function to create a folder and save files
void createRecording(String folderName) {
  // Debugging message before trying to create the directory
  Serial.println("Attempting to create directory: " + folderName);

  // Create directory (simpler name, no special characters or spaces)
  if (!SD.mkdir(folderName.c_str())) {
    Serial.println("Failed to create directory: " + folderName);
    return;
  } else {
    Serial.println("Directory created: " + folderName);
  }

  // Create and write to CSV file
  String csvFileName = folderName + "/data.csv";
  File csvFile = SD.open(csvFileName.c_str(), FILE_WRITE);
  if (csvFile) {
    csvFile.println("Time,Sensor1,Sensor2");  // CSV header
    csvFile.println("10:00,23.5,1013");       // Example data
    csvFile.println("10:05,23.6,1012");
    csvFile.close();
    Serial.println("CSV file created and data written.");
  } else {
    Serial.println("Failed to open CSV file: " + csvFileName);
  }

  // Create and write to TXT file
  String txtFileName = folderName + "/metadata.txt";
  File txtFile = SD.open(txtFileName.c_str(), FILE_WRITE);
  if (txtFile) {
    txtFile.println("Recording Date: 2024-09-19");
    txtFile.println("Sensor: BMP388");
    txtFile.println("Comments: Test recording with sample data.");
    txtFile.close();
    Serial.println("Text file created and metadata written.");
  } else {
    Serial.println("Failed to open text file: " + txtFileName);
  }
}
