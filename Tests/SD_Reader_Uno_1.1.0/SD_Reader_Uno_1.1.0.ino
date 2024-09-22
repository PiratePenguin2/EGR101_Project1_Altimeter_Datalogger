#include <SPI.h>
#include <SD.h>



File file = SD.open("/REC_007/data.csv");

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
  createRecording("REC_007");


  // Variables for parsing CSV data
  String line;
  float maxValue = -9999.0;  // Initialize with a very low value

  // Read header line (if needed)
  // line = file.readStringUntil('\n');  // Uncomment if the CSV file has headers

    // Read data lines
  while (file.available()) {
    line = file.readStringUntil('\n');
    line.trim();  // Remove leading/trailing whitespace

    // Split line into values
    int commaIndex = line.indexOf(',');
    if (commaIndex != -1) {
      String valueStr = line.substring(commaIndex + 1);  // Assuming the first column is time

      // Convert value to float (adjust as needed for your data type)
      float value = valueStr.toFloat();

      // Check if this value is the highest found so far
      if (value > maxValue) {
        maxValue = value;
      }
    }
  }

  file.close();  // Close the file

  // Print the highest value found
  Serial.print("Highest value in Sensor1 column: ");
  Serial.println(maxValue);
  
}

void loop() {
  // Nothing in loop for now
}

// Function to create a folder and save files
void createRecording(String folderName) {
  // Debugging message before trying to create the directory
  Serial.println("Attempting to create directory: " + folderName);

  // Check if the directory already exists
  if (SD.exists(folderName.c_str())) {
    Serial.println("Directory already exists: " + folderName);
  } else {
    // Create directory
    if (!SD.mkdir(folderName.c_str())) {
      Serial.println("Failed to create directory: " + folderName);
      return;
    } else {
      Serial.println("Directory created: " + folderName);
    }
  }

  // Create the CSV file path
  String csvFileName = folderName + "/data.csv";

  // Check if the CSV file already exists
  if (SD.exists(csvFileName.c_str())) {
    Serial.println("CSV file already exists: " + csvFileName);
  } else {
    // Create and write to CSV file
    File csvFile = SD.open(csvFileName.c_str(), FILE_WRITE);
    if (csvFile) {
      csvFile.println("Time,Sensor1,Sensor2");  // CSV header
      csvFile.println("10:00,23.5,1013");       // Example data
      csvFile.println("10:05,23.6,1012");
      csvFile.println("10:10,24.5,1025");
      csvFile.close();
      Serial.println("CSV file created and data written.");
    } else {
      Serial.println("Failed to open CSV file: " + csvFileName);
    }
  }

  // Create the TXT file path
  String txtFileName = folderName + "/metadata.txt";

  // Check if the TXT file already exists
  if (SD.exists(txtFileName.c_str())) {
    Serial.println("Text file already exists: " + txtFileName);
  } else {
    // Create and write to TXT file
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
}
