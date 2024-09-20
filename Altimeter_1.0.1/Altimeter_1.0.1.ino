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

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_1_PIN 14
#define BUTTON_2_PIN 12
#define BUTTON_3_PIN 13
#define BUTTON_4_PIN 26
#define SPEAKER_PIN 15

#define REC_BLINK_DELAY 650

//const int NUM_SCREENS = 4;

enum Menus {
  MENU_0,
  MENU_1,
  MENU_2,
  MENU_3
};
const int NUM_MENUS = 4;

bool menuActive = true;
bool liveCapture = false;
bool manualCapture = false;
bool showRecord = false;
bool captureActive = false;
//int count = 0;


Menus menuId = MENU_0;

Timer recordDot;

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
      display.println("Status");
      break;

    case MENU_3:
      display.println("Settings");
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

  switch(menuId) {
    case MENU_0:
      display.println("Current Altitude");
      display.setTextSize(2);
      display.setCursor(67, 13);
      display.println("0000m");
      display.setTextSize(1.5);
      display.setCursor(10, 20);
      break;
    
    case MENU_1:
      display.println("Max Altitude");
      display.setTextSize(2);
      display.setCursor(67, 13);
      display.println("0000m");
      display.setTextSize(1.5);
      display.setCursor(10, 20);
      break;
    
    case MENU_2:
      display.println("Status");
      break;

    case MENU_3:
      display.println("Settings");
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

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  display.clearDisplay();

  btn1.attach(BUTTON_1_PIN);
  btn2.attach(BUTTON_2_PIN);
  btn3.attach(BUTTON_3_PIN);
  btn4.attach(BUTTON_4_PIN);

  pinMode(SPEAKER_PIN, OUTPUT);
  
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(100);
  digitalWrite(SPEAKER_PIN, LOW);
  delay(100);
  digitalWrite(SPEAKER_PIN, HIGH);
  delay(100);
  digitalWrite(SPEAKER_PIN, LOW);

  //testscrolltext();    // Draw scrolling text

}

void loop() {
  btn1.update(); btn2.update(); btn3.update(); btn4.update();
  checkRecordDot();


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
    }
  }


  if (btn4.isTripped() && !menuActive) {
    if (liveCapture) {  // If recording in live capture mode
      if (captureActive) {  // If displaying recording symbol
        captureActive = false;
      }
      else {
        recordDot.setTimer(REC_BLINK_DELAY);
        captureActive = true;
      }
    } else if (manualCapture) {
      if (captureActive) {
        // idk man
      } else {
        captureActive = true;
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

  Serial.println(liveCapture);

}

void testscrolltext(void) {
  display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(F("scroll"));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}

void checkRecordDot() {
  if (recordDot.isFinished()) {
    showRecord = !showRecord;
    recordDot.setTimer(REC_BLINK_DELAY);
  }
}