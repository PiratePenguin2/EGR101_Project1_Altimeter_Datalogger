/**
* https://randomnerdtutorials.com/altimeter-datalogger-esp32-bmp388/
*
 */
 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_1_PIN 14
#define BUTTON_2_PIN 12
#define BUTTON_3_PIN 3
#define BUTTON_4_PIN 4

int menuId = 0;
bool menuActive = true;

void displayMenu(int id, bool clear=false, bool update=false) {
  menuId = id;

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);

  if (clear) {
    display.clearDisplay();
  }

  switch(menuId) {
    case 0:
      display.println("Current");
      display.setCursor(10, 16);
      display.println("Altitude");
      break;
    
    case 1:
      display.println("Max");
      display.setCursor(10, 16);
      display.println("Altitude");
      break;
    
    case 2:
      display.println("Status");
      break;

    case 3:
      display.println("Settings");
      break;
    
    default:
      display.println("Undefined Menu");
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
    case 0:
      display.println("Current Altitude");
      break;
    
    case 1:
      display.println("Max Altitude");
      display.setTextSize(2);
      display.setCursor(70, 11);
      display.println("000m");
      break;
    
    case 2:
      display.println("Status");
      break;

    case 3:
      display.println("Settings");
      break;
    
    default:
      display.println("Undefined Menu");
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


void setup() {
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  display.clearDisplay();


  //testscrolltext();    // Draw scrolling text

  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUTTON_3_PIN, INPUT_PULLUP);
  pinMode(BUTTON_4_PIN, INPUT_PULLUP);


}

void loop() {

  if (getButton1()) {
    if (menuActive) {
      menuActive = false;
      bool state = true;
      while (state) {state=getButton1();}
      swipeRight();
    } else {
      menuActive = true;
      bool state = true;
      while (state) {state=getButton1();}
      swipeLeft();
    }
  }

  if (menuActive) {
    if (getButton2()) {
      if (menuId >= 3) {
        menuId = 0;
        swipeDown();
      } else {
        menuId += 1;
        swipeDown();
      }
      bool state = true;
      while (state) {state = getButton2();}
    }
    displayMenu(menuId, true, true);
  } else {
    displayScreen(menuId, true, true);
    //do something else, show static page
  }

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

bool getButton1() {
  bool state = digitalRead(BUTTON_1_PIN) == LOW;
  return state ? true : false;
}
bool getButton2() {
  bool state = digitalRead(BUTTON_2_PIN) == LOW;
  return state ? true : false;
}
bool getButton3() {
  bool state = digitalRead(BUTTON_3_PIN) == LOW;
  return state ? true : false;
}
bool getButton4() {
  bool state = digitalRead(BUTTON_4_PIN) == LOW;
  return state ? true : false;
}
