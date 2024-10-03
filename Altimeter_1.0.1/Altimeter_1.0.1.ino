/**
* Source:
* https://randomnerdtutorials.com/altimeter-datalogger-esp32-bmp388/
 */

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
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Pin declarations
#define BUTTON_1_PIN 26
#define BUTTON_2_PIN 14
#define BUTTON_3_PIN 12
#define BUTTON_4_PIN 13
#define SPEAKER_PIN 15
#define SD_CS_PIN 5

// User variables
#define SEALEVELPRESSURE_HPA (1013.25)
#define RECORDING_SLOTS 40

// Wifi credentials (required)
const char* ssid = "CBU-LANCERS";       // Replace with your Wi-Fi SSID
const char* password = "L@ncerN@tion"; // Replace with your Wi-Fi password


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, false);

#define REC_BLINK_DELAY 650
#define REC_LIVE_INTERVAL 500

#define CSV_FILE_NAME "data"
#define TEXT_FILE_NAME "info"

AsyncWebServer server(80);

// HTML for the web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Altimeter Dashboard</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f5f5f7;
            color: #1d1d1f;
        }
        header {
            background-color: #fff;
            padding: 20px;
            text-align: center;
        }
        #liveReadout {
            font-size: 2em;
            background-color: #fff;
            padding: 20px;
            border-radius: 10px;
            margin-top: 50px;
        }
    </style>
</head>
<body>
    <header>
        <h1>Altimeter Dashboard</h1>
    </header>
    <main>
        <div id="liveReadout">
            <h1>Live Readout</h1>
            <p>Altitude: <span id="altitude">Waiting for data...</span></p>
        </div>
    </main>
    <script>
        function fetchAltitude() {
            fetch('/altitude')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('altitude').innerText = data;
                })
                .catch(error => console.error('Error fetching altitude:', error));
        }
        setInterval(fetchAltitude, 500);
    </script>
</body>
</html>

)rawliteral";


enum Menus {
  CUR_ALT,
  MAX_ALT,
  STORAGE,
  WIFI, 
  UNIT,
  OFFSET
};
const int NUM_MENUS = 6;

bool menuActive = false;
bool liveCapture = true;
bool manualCapture = false;
bool showRecord = false;
bool captureActive = false;

bool useMeters = false;
bool applyOffset = false;
double altOffset = 0.0;
double currentAltitude = 0.0;
//double currentPressure = 0.0;
//double currentTemp = 0.0;
String currentWebAltitude = "0.0";
double maxAltitude = 0.0;
int frameCount = 0;
int recordTimestamp = 0;
int numDirectories = 0;

String currentRecording;

Adafruit_BMP3XX bmp;

Menus menuId = CUR_ALT;

Timer recordDot, recordTimer;

Sensor btn1, btn2, btn3, btn4;

int countRECDirectories(fs::FS &fs, const char *dirname = "/", uint8_t levels = 0) { // returns the amount of logs already stored, partially generated by ChatGPT
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

void displayMenu(Menus id, bool clear=false, bool update=false) { // handles displaying of the menu pages
  menuId = id;

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);

  if (clear) {
    display.clearDisplay();
  }

  switch(menuId) {
    case CUR_ALT:
      display.println("Current");
      display.setCursor(10, 16);
      display.println("Altitude");
      break;
    
    case MAX_ALT:
      display.println("Max");
      display.setCursor(10, 16);
      display.println("Altitude");
      break;
    
    case STORAGE:
      display.println("Storage");
      break;

    case UNIT:
      display.println("Switch to");
      display.setCursor(10, 16);
      if (useMeters) {
        display.println("Feet");
      } else {
        display.println("Meters");
      }
      break;

    case WIFI:
      display.println("Wifi");
      break;

    case OFFSET:
      if (applyOffset) {
        display.print("Reset\nAltitude");
      } else {
        display.print("Zero\nAltitude");
      }
      display.display();
      break;

    default:
      display.println("Undefined Menu");
      for(;;);
      break;
  }

  if (update) {
    display.display();
  }

}

void displayScreen(int id, bool clear=false, bool update=false) { // handles displaying of the main pages
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (clear) {
    display.clearDisplay();
  }
  display.setTextSize(1);
  display.setCursor(120, 0);
  if (manualCapture) {
    display.println("C");
  } else if (liveCapture) {
    display.println("R");
  }
  display.setCursor(0, 0);
  switch(menuId) {
    
    case CUR_ALT:
      display.println("Current Altitude");
      display.setTextSize(2);
      display.setCursor(67, 13);
      if (!useMeters) {
        display.print(static_cast<int>(currentAltitude * 3.28084));
        display.setTextSize(1);
        display.setCursor(103, 20);
        display.println("ft");
      } else {
        display.print(static_cast<int>(currentAltitude));
        display.println("m");
      }
      display.setTextSize(1.5);
      display.setCursor(10, 20);
      break;
    
    case MAX_ALT:
      display.println("Max Altitude");
      display.setTextSize(2);
      display.setCursor(67, 13);
      if (!useMeters) {
        display.print(static_cast<int>(maxAltitude * 3.28084));
        display.setTextSize(1);
        display.setCursor(103, 20);
        display.println("ft");
      } else {
        display.print(static_cast<int>(maxAltitude));
        display.println("m");
      }
      display.setTextSize(1.5);
      display.setCursor(10, 20);
      break;
    
    case STORAGE:
      display.println("Storage");
      display.setCursor(0, 10);
      display.print(numDirectories);
      display.print(" / ");
      display.print(RECORDING_SLOTS);
      display.println(" Records Used");
      break;

    case UNIT:
      if (useMeters) {
        useMeters = false;
      } else {
        useMeters = true;
      }
      menuActive = true;
      digitalWrite(SPEAKER_PIN, HIGH);
      delay(100);
      digitalWrite(SPEAKER_PIN, LOW);
      break;

    case WIFI:
      display.print("Wifi");
      if (WiFi.status() == WL_CONNECTED) {
        display.setTextSize(1);
        display.println(" Connected");
        display.setCursor(0, 10);
        display.print("SSID: ");
        display.println(ssid);
        display.setCursor(15, 20);
        display.print("IP: ");
        display.println(WiFi.localIP());
      } else {
        display.print(" Disconnected");
      }
      break;

    case OFFSET:
      display.setTextSize(1);
      if (applyOffset) {
        applyOffset = false;
        display.print("Altitude Reset");
      } else {
        applyOffset = true;
        display.print("Altitude Zeroed");
        altOffset = currentAltitude;
      }
      display.display();
      digitalWrite(SPEAKER_PIN, HIGH);
      delay(200);
      digitalWrite(SPEAKER_PIN, LOW);
      delay(200);
      menuActive = true;
      break;
    
    default:
      display.println("Undefined Menu");
      for(;;);
      break;
  }


  if (update) {
    display.display();
  }

}

void swipeDown() { // downwards swipe animation, partially ChatGPT
  int swipeSpeed = 7; // Speed of swipe, increase to make it faster

  for (int y = 0; y <= SCREEN_HEIGHT+10; y += swipeSpeed) {
    
    // Draw a black rectangle swiping from top to bottom
    display.fillRect(0, SCREEN_HEIGHT - y, SCREEN_WIDTH, y, SSD1306_BLACK);
    
    // Update the display
    display.display();
    
    delay(40); // Adjust delay for smoother animation
  }

  // Make sure the screen is completely cleared
  display.clearDisplay();
  display.display();
}

void swipeRight() { // right swipe animation, partially ChatGPT
  int swipeSpeed = 14; // Speed of swipe, increase to make it faster

  for (int x = 0; x <= SCREEN_WIDTH; x += swipeSpeed) {
    
    // Draw a black rectangle swiping from right to left
    display.fillRect(SCREEN_WIDTH - x, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_BLACK);
    
    // Update the display
    display.display();
    
    delay(30); // Adjust delay for smoother animation
  }

  // Make sure the screen is completely cleared
  display.clearDisplay();
  display.display();
}

void swipeLeft() { // left swipe animation, partially ChatGPT
  int swipeSpeed = 14; // Speed of swipe, increase to make it faster

  for (int x = 0; x <= SCREEN_WIDTH; x += swipeSpeed) {
    
    // Draw a black rectangle swiping from right to left
    display.fillRect(0, 0, x, SCREEN_HEIGHT, SSD1306_BLACK);

    
    // Update the display
    display.display();
    
    delay(30); // Adjust delay for smoother animation
  }

  // Make sure the screen is completely cleared
  display.clearDisplay();
  display.display();
}

void showRecordingState(bool show) { // Handles display/hide of flashing rec icon
  if (show) {
    display.setCursor(7, 20);
    if (liveCapture && captureActive) {
      display.fillCircle(2, 23, 2, SSD1306_WHITE);
      display.println("REC");
    } else if (manualCapture && captureActive) {
      display.fillCircle(2, 23, 2, SSD1306_WHITE);
      display.println("WAIT CAPT");
    }
  }
}


void setup() {
  Serial.begin(9600);

  /* Initialize Display */
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  display.clearDisplay();
  display.setRotation(2);

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    display.setCursor(5, 10);
    display.setTextSize(1);
    display.println("SD CARD MISSING");
    display.display();
    return;
  }
  Serial.println("SD card initialized.");


  // Initialize BMP388 sensor
  if (!bmp.begin_I2C()) {
    Serial.println("Could not find a valid BMP388 sensor, check wiring!");
    display.clearDisplay();
    display.setCursor(5, 10);
    display.setTextSize(1);
    display.println("BMP MISSING");
    display.display();
    while (1);
  }

  // Set up oversampling and filter initialization
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  // Attach button pins
  btn1.attach(BUTTON_1_PIN);
  btn2.attach(BUTTON_2_PIN);
  btn3.attach(BUTTON_3_PIN);
  btn4.attach(BUTTON_4_PIN);

  WiFi.begin(ssid, password); // Connect to Wi-Fi
  
  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  // Print the IP address

  // Initialize the web server
  initWebServer(server, currentWebAltitude);

  // Initialize speaker
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(100);
  digitalWrite(SPEAKER_PIN, LOW);
  delay(100);
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(100);
  digitalWrite(SPEAKER_PIN, LOW);
}

void loop() {

  // update primary functions
  btn1.update(); btn2.update(); btn3.update(); btn4.update();
  checkRecordDot();

  currentAltitude = (static_cast<int>(bmp.readAltitude(SEALEVELPRESSURE_HPA) * 100.0)) / 100.0;
  //currentPressure = bmp.readPressure();
  //currentTemp = bmp.readTemperature();

  if (applyOffset) {
    currentAltitude = currentAltitude - altOffset;
  }

  if (useMeters) {
    currentWebAltitude = String(currentAltitude) + " m";
  } else {
    currentWebAltitude = String(currentAltitude * 3.28084) + " ft";
  }

  // avoid miscalibration during first cycles
  if (currentAltitude > maxAltitude && currentAltitude < maxAltitude + 300) {
    maxAltitude = currentAltitude;
  }


  if (btn1.isTripped()) { // Toggles between the display screen and the menu
    if (menuActive) {
      menuActive = false;
      if (menuId == STORAGE) {
        // store num directories
        display.setTextSize(2);
        display.setCursor(0, 16);
        display.println("Loading...");
        display.display();
        numDirectories = countRECDirectories(SD);
      }
      swipeRight();
    } else {
      menuActive = true;
      swipeLeft();
    }
  }

  if (menuActive) {       // Cycles through the different menu options
    if (btn2.isTripped()) {
      if (menuId < NUM_MENUS - 1) {
        menuId = static_cast<Menus>(static_cast<int>(menuId) + 1);
        swipeDown();
      } else {
        menuId = CUR_ALT;
        swipeDown();
      }
    }
    displayMenu(menuId, true, true);
  } else {
    displayScreen(menuId, true, false);

    // display the recording symbol
    showRecordingState(showRecord);

    display.display();
  }

  if (btn3.isTripped() && !menuActive) {
    if (!captureActive) {

      if (manualCapture) {
        manualCapture = false;
        liveCapture = true;
        digitalWrite(SPEAKER_PIN, HIGH);
        display.clearDisplay();
        display.setCursor(15, 12);
        display.setTextSize(2);
        display.println("REC MODE");
        display.display();
        delay(250);
        display.clearDisplay();
        digitalWrite(SPEAKER_PIN, LOW);
      } else if (liveCapture) {
        liveCapture = false;
        manualCapture = true;
        digitalWrite(SPEAKER_PIN, HIGH);
        display.clearDisplay();
        display.setCursor(15, 12);
        display.setTextSize(2);
        display.println("CAPT MODE");
        display.display();
        delay(250);
        display.clearDisplay();
        digitalWrite(SPEAKER_PIN, LOW);
      } else {
        liveCapture = true;
        manualCapture = false;
      }
    } else {
      if (manualCapture) {
        stopRecording();
      } else if (liveCapture) {
        stopRecording();
      }
    }
  }


  if (btn4.isTripped() && !menuActive) {
    if (liveCapture) {  // If recording in live capture mode
      if (captureActive) {  // If displaying recording symbol
        stopRecording();
      }
      else {
        createNewRecording("/REC_");
        recordDot.setTimer(REC_BLINK_DELAY);
        recordTimer.setTimer(REC_LIVE_INTERVAL);
        captureActive = true;

        display.clearDisplay();
        display.setCursor(15, 12);
        display.setTextSize(2);
        display.println("REC START");
        display.display();

        digitalWrite(SPEAKER_PIN, HIGH);
        delay(100);
        digitalWrite(SPEAKER_PIN, LOW);
        delay(100);
        digitalWrite(SPEAKER_PIN, HIGH);
        delay(100);
        digitalWrite(SPEAKER_PIN, LOW);

        delay(500);
        display.clearDisplay();
      }
    } else if (manualCapture) {
      if (captureActive) {
        // idk man
      } else {
        captureActive = true;
        createNewRecording("/REC_");
      }

      storeCSVData();

      display.clearDisplay();
      display.setCursor(15, 12);
      display.setTextSize(2);
      display.println("CAPTURED");
      display.display();

      digitalWrite(SPEAKER_PIN, HIGH);
      delay(100);
      digitalWrite(SPEAKER_PIN, LOW);
      delay(100);
      digitalWrite(SPEAKER_PIN, HIGH);
      delay(100);
      digitalWrite(SPEAKER_PIN, LOW);

      delay(500);
      display.clearDisplay();

      // capture an altitude
    }
  }


  if (liveCapture && captureActive) {
    if (recordTimer.isFinished()) {
      storeCSVData();
      recordTimer.setTimer(REC_LIVE_INTERVAL);
    }
  }

  if (!bmp.performReading()) {
  Serial.println("Failed to perform reading :(");
  return;
  }


  //updateAltitude(currentAltitude);
  
  delay(10);
}

void checkRecordDot() { // timer handler for the flashing record icon (required so that it doesn't stall the rest of the program with a delay())
  if (recordDot.isFinished()) {
    showRecord = !showRecord;
    recordDot.setTimer(REC_BLINK_DELAY);
  }
}

bool createNewRecording(String baseName) {
  frameCount = 0;
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
          currentRecording = folderName;
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

    // Create and write to info TXT file
    String txtFileName = folderName + "/" + TEXT_FILE_NAME + ".txt";
    if (!createTextFile(txtFileName)) {
        Serial.println("Failed to create text file.");
        return false;
    }

    return true;
}

bool createCSVFile(String csvFileName) { // create the csv file
    File csvFile = SD.open(csvFileName.c_str(), FILE_WRITE);
    if (csvFile) {
      if (manualCapture) {
        csvFile.println("Frame,Altitude(m)");//,Pressure(HPa),Temperature(C)");  // CSV header
      }
      else {
        csvFile.println("Frame,Timestamp(mSec),Altitude(m)");//,Pressure(HPa),Temperature(C)");  // CSV header
      }
      csvFile.close();
      return true;
    } else {
        Serial.println("Failed to open CSV file: " + csvFileName);
        return false;
    }
} 

bool createTextFile(String txtFileName) {
    File txtFile = SD.open(txtFileName.c_str(), FILE_WRITE);
    if (txtFile) {
        //txtFile.println("Recording Date: 2024-09-19");
        txtFile.print("Recording: ");
        txtFile.println(currentRecording);
        txtFile.print("Recording Mode: ");
        manualCapture ? txtFile.println("Capture") : txtFile.println("Continuous");
        txtFile.println("Sensor: BMP388");
        txtFile.close();

        Serial.println("Text file created and info written: " + txtFileName);
        return true;
    } else {
        Serial.println("Failed to open text file: " + txtFileName);
        return false;
    }
}

void storeCSVData() {
  // Open the CSV file in append mode
  File csvFile = SD.open(currentRecording + "/" + CSV_FILE_NAME + ".csv", FILE_APPEND);
  
  if (csvFile) {
    recordTimestamp = frameCount * REC_LIVE_INTERVAL;

    // Write the currentAltitude to the file
    csvFile.print(frameCount);       // Writing frame value
    csvFile.print(",");
    //csvFile.print((recordTimestamp / 1000) + ":" + (recordTimestamp % 1000));
    if (!manualCapture) {
      csvFile.print(recordTimestamp);
      csvFile.print(",");
    }
    csvFile.print(currentAltitude);  // Writing altitude value
    /*csvFile.print(",");
    csvFile.print(currentPressure);  // Writing pressure value
    csvFile.print(",");
    csvFile.print(currentTemp);*/      // Writing temperature value
    
    csvFile.println();               // Move to the next line after the current data
    
    csvFile.close();                 // Close the file to ensure the data is saved
    Serial.println("Data stored successfully.");
    frameCount++;
  } else {
    Serial.println("Error opening CSV file for writing.");
  }
}

void storeTextData() {
  File txtFile = SD.open(currentRecording + "/" + TEXT_FILE_NAME + ".txt", FILE_APPEND);

  if (txtFile) {
    txtFile.println();
    txtFile.print("Max Altitude: ");
    txtFile.println(maxAltitude);
    if(!manualCapture) {
      txtFile.print("Duration: ");
      txtFile.print(recordTimestamp);
      txtFile.println("mSec");
    }
    txtFile.close();
  } else {
    Serial.println("Error opening text file for writing.");
  }
}

void stopRecording() {
  storeTextData();
  captureActive = false;
  display.clearDisplay();
  display.setCursor(5, 12);
  display.setTextSize(2);
  display.println("REC STOP");
  display.display();

  digitalWrite(SPEAKER_PIN, HIGH);
  delay(100);
  delay(100);
  digitalWrite(SPEAKER_PIN, LOW);

  delay(500);
  display.clearDisplay();
}

void initWebServer(AsyncWebServer &server, String &currentWebAltitude) { // init the web server, heavily modified from source ChatGPT
    // Serve the HTML page
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    // Serve the altitude data
    server.on("/altitude", HTTP_GET, [&](AsyncWebServerRequest *request){
        request->send(200, "text/plain", String(currentWebAltitude));
    });

    // Start the server
    server.begin();
}
