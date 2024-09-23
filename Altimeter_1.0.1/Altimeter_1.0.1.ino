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
#define RECORDING_SLOTS 20

enum Menus {
  MENU_0,
  MENU_1,
  MENU_2,
  MENU_3
};
const int NUM_MENUS = 4;

bool menuActive = true;
bool liveCapture = true;
bool manualCapture = false;
bool showRecord = false;
bool captureActive = false;

bool useMeters = false;
double currentAltitude = 0;
double maxAltitude = 0;
//int count = 0;

Adafruit_BMP3XX bmp;

Menus menuId = MENU_0;

Timer recordDot, recordTimer;

Sensor btn1, btn2, btn3, btn4;

void displayMenu(Menus id, bool clear=false, bool update=false) {
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

    case MENU_3:
      display.println("Switch to");
      display.setCursor(10, 16);
      if (useMeters) {
        display.println("Feet");
      } else {
        display.println("Meters");
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

void displayScreen(int id, bool clear=false, bool update=false) {
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
      display.print("9");
      display.print(" / ");
      display.print(RECORDING_SLOTS);
      display.println(" Records Used");
      break;

    case MENU_3:
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
    
    default:
      display.println("Undefined Menu");
      for(;;);
      break;
  }


  if (update) {
    display.display();
  }

}

void swipeDown() {
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

void swipeRight() {
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

void swipeLeft() {
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

void showRecordingState(bool show) {
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

  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    return;
  }

  Serial.println("SD card initialized.");

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  display.clearDisplay();

  // Initialize BMP388 sensor
  if (!bmp.begin_I2C()) {
    Serial.println("Could not find a valid BMP388 sensor, check wiring!");
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

  // Initialize speaker
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(100);
  digitalWrite(SPEAKER_PIN, LOW);
  delay(100);
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(100);
  digitalWrite(SPEAKER_PIN, LOW);

  // Create a new recording folder and files


}

void loop() {
  btn1.update(); btn2.update(); btn3.update(); btn4.update();
  checkRecordDot();

  currentAltitude = static_cast<int>((bmp.readAltitude(SEALEVELPRESSURE_HPA)) + .5);

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
    //do something else, show static page

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
      recordTimer.setTimer(REC_LIVE_INTERVAL);
    }
  }

  if (!bmp.performReading()) {
  Serial.println("Failed to perform reading :(");
  return;
  }
  
  delay(10);
}

void checkRecordDot() {
  if (recordDot.isFinished()) {
    showRecord = !showRecord;
    recordDot.setTimer(REC_BLINK_DELAY);
  }
}


bool createNewRecording(String baseName) {
    for (int i = 1; i <= RECORDING_SLOTS; i++) {
        // Format folder name with leading zeros (001, 002, ...)
        String folderNum = (i < 10) ? "00" + String(i) : "0" + String(i);
        String folderName = baseName + folderNum;

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
