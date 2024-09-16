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


#define BUTTON_1_PIN 1
#define BUTTON_2_PIN 2
#define BUTTON_3_PIN 3
#define BUTTON_4_PIN 4

int id = 0;
bool menuActive = false;

void setup() {
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display();
  display.clearDisplay();

  testscrolltext();    // Draw scrolling text

  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUTTON_3_PIN, INPUT_PULLUP);
  pinMode(BUTTON_4_PIN, INPUT_PULLUP);


}

void loop() {

  if (getButton1()) {
    if (menuActive) {
      menuActive = false;
      while (getButton1())
    } else {
      menyActive = true;
      while (getButton1());
    }
  }

  if (menuActive) {
    if (getButton2()) {
      if (id >= 3) {}
        id = 0;
      } else {
        id += 1;
      }
    } 
    while (getButton2()) {}
    displayMenu(id, true, true);
  } else {
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


void displayMenu(int id, bool clear=false, bool update=false) {
  if (clear) {
    display.clearDisplay();
  }

  if (id == 0) {
    display.println("Menu 0");
  } else if (id == 1) {
    display.println("Menu 1");
  } else if (id == 2) {
    display.println("Menu 2");
  } else if (id == 3) {
    display.println("Menu 3");
  }

  if (update) {
    display.display();
  }

}

bool getButton1() {
  bool state;
  if (digitalRead(BUTTON_1_PIN) == HIGH) {
    state = true;
  } else {
    state = false;
  }
  return state;
}
bool getButton2() {
  bool state;
  if (digitalRead(BUTTON_2_PIN) == HIGH) {
    state = true;
  } else {
    state = false;
  }
  return state;
}
bool getButton3() {
  bool state;
  if (digitalRead(BUTTON_3_PIN) == HIGH) {
    state = true;
  } else {
    state = false;
  }
  return state;
}
bool getButton4() {
  bool state;
  if (digitalRead(BUTTON_4_PIN) == HIGH) {
    state = true;
  } else {
    state = false;
  }
  return state;
}



