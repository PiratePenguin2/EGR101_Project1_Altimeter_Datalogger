#include <SPI.h>
#include <SD.h>

#define CSV_FILE_NAME data
#define TEXT_FILE_NAME metadata
//#define RECORDING_SLOTS 9;


void setup() {
    Serial.begin(9600);
    createNewRecording("REC_");
}

void loop() {
    
}

bool folderExists(String folderName) {

}

bool csvExists(String folderName) {

}

bool textExists(String folderName) {

}

bool createNewRecording(String name) {
    for(int i = 1; i <= 9; i++)
    {
        String num = static_cast<String>(i);
        if (i < 10) {
            num = "00" + num;
        }
        else if (i < 100) {
            num = "0" + num;
        }
        String newName = name + num;
        Serial.println(newName);
        if (createRecording(newName)) {
            Serial.println("FILE CREATED!!!");
            return true;
        }
    }
    Serial.println("TOO MANY FILES");
    return false;
}

bool createRecording(String name) {
    bool folderExists = false;
    bool csvExists = false;
    bool textExists = false;


    if (SD.exists(name.c_str())) {
        Serial.println("Directory already exists: " + name);
        folderExists = true;
    }
    else {
        // Create directory
        if (!SD.mkdir(name.c_str())) {
            Serial.println("Failed to create directory: " +  name);
        } else {
            Serial.println("Directory created: " + name);
        }
    }


    // Create the CSV file path
    String csvFileName = name + "/data.csv";

    // Check if the CSV file already exists
    if (SD.exists(csvFileName.c_str())) {
        Serial.println("CSV file already exists: " + csvFileName);
        csvExists = true;
    }
    else {
        // Create and write to CSV file
        File csvFile = SD.open(csvFileName.c_str(), FILE_WRITE);
        if (csvFile) {
            csvFile.println("Time,Sensor1,Sensor2");  // CSV header
            csvFile.println("10:00,23.5,1013");       // Example data
            csvFile.println("10:05,23.6,1012");
            csvFile.println("10:10,24.5,1025");
            csvFile.close();
            Serial.println("CSV file created and data written.");
        }
        else {
            Serial.println("Failed to open CSV file: " + csvFileName);
        }
    }

    // Create the TXT file path
    String txtFileName = name + "/metadata.txt";

    // Check if the TXT file already exists
    if (SD.exists(txtFileName.c_str())) {
        Serial.println("Text file already exists: " + txtFileName);
        textExists = true;
    }
    else {
        // Create and write to TXT file
        File txtFile = SD.open(txtFileName.c_str(), FILE_WRITE);
        if (txtFile) {
            txtFile.println("Recording Date: 2024-09-19");
            txtFile.println("Sensor: BMP388");
            txtFile.println("Comments: Test recording with sample data.");
            txtFile.close();
            Serial.println("Text file created and metadata written.");
        }
        else {
        Serial.println("Failed to open text file: " + txtFileName);
        }
    }
}
