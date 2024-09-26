/**
* https://randomnerdtutorials.com/altimeter-datalogger-esp32-bmp388/
*
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


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SEALEVELPRESSURE_HPA (1013.25)

#define BUTTON_1_PIN 14
#define BUTTON_2_PIN 12
#define BUTTON_3_PIN 13
#define BUTTON_4_PIN 26
#define SPEAKER_PIN 15

#define REC_BLINK_DELAY 650
#define REC_LIVE_INTERVAL 500

#define CSV_FILE_NAME "data"
#define TEXT_FILE_NAME "metadata"
#define SD_CS_PIN 5  // SD Card CS pin (adjust as per your wiring)
#define RECORDING_SLOTS 200


const char* ssid = "CBU-LANCERS";       // Replace with your Wi-Fi SSID
const char* password = "L@ncerN@tion"; // Replace with your Wi-Fi password

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
        /* Apply SF Pro system font */
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            margin: 0;
            padding: 0;
            background-color: #f5f5f7;
            color: #1d1d1f;
        }
        header {
            background-color: #fff;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
            padding: 20px;
            position: fixed;
            top: 0;
            width: 100%;
            z-index: 1000;
        }
        header nav {
            display: flex;
            justify-content: center;
        }
        header nav a {
            color: #0071e3;
            font-weight: 500;
            text-decoration: none;
            padding: 0 15px;
            line-height: 1.6;
            font-size: 18px;
        }
        header nav a:hover {
            text-decoration: underline;
        }
        main {
            margin-top: 80px; /* To avoid overlap with the header */
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 80vh;
        }
        h1 {
            font-size: 2.5em;
            margin-bottom: 20px;
        }
        #liveReadout {
            font-size: 2em;
            background-color: #fff;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
        }
        footer {
            text-align: center;
            padding: 20px;
            font-size: 14px;
            color: #6e6e73;
        }
    </style>
</head>
<body>
    <header>
        <nav>
            <a href="#home" onclick="showSection('home')">Home</a>
            <a href="#flightLogs" onclick="showSection('flightLogs')">Flight Logs</a>
            <a href="#liveReadout" onclick="showSection('liveReadout')">Live Readout</a>
            <a href="#sdCardStorage" onclick="showSection('sdCardStorage')">SD Card Storage</a>
        </nav>
    </header>

    <main id="homeSection">
        <div id="liveReadout">
            <h1>Live Readout</h1>
            <p>Altitude: <span id="altitude">Waiting for data...</span></p>
            <p>Time: <span id="time">Waiting for data...</span></p>
        </div>
    </main>

    <main id="flightLogsSection" style="display: none;">
        <h1>Flight Logs</h1>
        <p>Coming soon...</p>
    </main>

    <main id="liveReadoutSection" style="display: none;">
        <h1>Live Readout Page</h1>
        <p>This will show live data from your altimeter.</p>
    </main>

    <main id="sdCardStorageSection" style="display: none;">
        <h1>SD Card Storage</h1>
        <p>Access and manage your SD card data here.</p>
    </main>

    <footer>
        <p>Altimeter Dashboard © 2024</p>
    </footer>

    <script>
        // Function to toggle between sections
        function showSection(section) {
            document.getElementById('homeSection').style.display = 'none';
            document.getElementById('flightLogsSection').style.display = 'none';
            document.getElementById('liveReadoutSection').style.display = 'none';
            document.getElementById('sdCardStorageSection').style.display = 'none';

            if (section === 'home') {
                document.getElementById('homeSection').style.display = 'flex';
            } else if (section === 'flightLogs') {
                document.getElementById('flightLogsSection').style.display = 'block';
            } else if (section === 'liveReadout') {
                document.getElementById('liveReadoutSection').style.display = 'block';
            } else if (section === 'sdCardStorage') {
                document.getElementById('sdCardStorageSection').style.display = 'block';
            }
        }

        // Example of updating live readout (you can replace this with real data)
        setInterval(() => {
            const now = new Date();
            document.getElementById('altitude').innerText = Math.floor(Math.random() * 10000) + ' ft';
            document.getElementById('time').innerText = now.toLocaleTimeString();
        }, 1000);

	<script>
        function fetchAltitude() {
            fetch('/altitude')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('altitude').innerText = data + ' m';
                })
                .catch(error => console.error('Error fetching altitude:', error));
        }
        setInterval(fetchAltitude, 500);
    </script>
</body>
</html>

)rawliteral";


enum Menus {
  MENU_0,
  MENU_1,
  MENU_2,
  MENU_3, 
  MENU_4
};
const int NUM_MENUS = 5;

bool menuActive = false;
bool liveCapture = true;
bool manualCapture = false;
bool showRecord = false;
bool captureActive = false;

bool useMeters = false;
double currentAltitude = 0.0;
double maxAltitude = 0.0;
int frameCount = 0;

String currentRecording;

Adafruit_BMP3XX bmp;

Menus menuId = MENU_0;

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
    case MENU_0:
      display.println("Current");
      display.setCursor(10, 16);
      display.println("Altitude");
      break;
    
    case MENU_1:
      display.println("Max");
      display.setCursor(10, 16);
      display.println("Altitude");
      break;
    
    case MENU_2:
      display.println("Storage");
      break;

    case MENU_4:
      display.println("Switch to");
      display.setCursor(10, 16);
      if (useMeters) {
        display.println("Feet");
      } else {
        display.println("Meters");
      }
      break;

    case MENU_3:
      display.println("Wifi");
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
    
    case MENU_0:
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
    
    case MENU_1:
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
    
    case MENU_2:
      display.println("Storage");
      display.setCursor(0, 10);
      display.print(countRECDirectories(SD));
      display.print(" / ");
      display.print(RECORDING_SLOTS);
      display.println(" Records Used");
      break;

    case MENU_4:
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

    case MENU_3:
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
  initWebServer(server, currentAltitude);

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

  // avoid miscalibration during first cycles
  if (currentAltitude > maxAltitude && currentAltitude < maxAltitude + 300) {
    maxAltitude = currentAltitude;
  }


  if (btn1.isTripped()) { // Toggles between the display screen and the menu
    if (menuActive) {
      menuActive = false;
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
        menuId = MENU_0;
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
        captureActive = false;
        frameCount = 0;
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

      } else if (liveCapture) {
        captureActive = false;
        frameCount = 0;
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
    }
  }


  if (btn4.isTripped() && !menuActive) {
    if (liveCapture) {  // If recording in live capture mode
      if (captureActive) {  // If displaying recording symbol
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

      storeData();

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
      storeData();
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

void checkRecordDot() {
  if (recordDot.isFinished()) {
    showRecord = !showRecord;
    recordDot.setTimer(REC_BLINK_DELAY);
  }
}

void storeData() {
  // Open the CSV file in append mode
  File csvFile = SD.open(currentRecording + "/DATA.csv", FILE_APPEND);
  
  if (csvFile) {
    frameCount++;
    // Write the currentAltitude to the file
    csvFile.print(frameCount);       // Writing frame value
    csvFile.print(",");              // Add comma separator for CSV format
    csvFile.print(currentAltitude);  // Writing altitude value
    
    csvFile.println();               // Move to the next line after the current data
    
    csvFile.close();                 // Close the file to ensure the data is saved
    Serial.println("Data stored successfully.");
  } else {
    Serial.println("Error opening file for writing.");
  }
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
        csvFile.println("Number,Altitude");  // CSV header
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

// Function to initialize the web server
void initWebServer(AsyncWebServer &server, double &currentAltitude) {
    // Serve the HTML page
    server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    // Serve the altitude data
    server.on("/altitude", HTTP_GET, [&](AsyncWebServerRequest *request){
        request->send(200, "text/plain", String(currentAltitude));
    });

    // Start the server
    server.begin();
}
