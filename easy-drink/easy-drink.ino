#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Time.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <Encoder.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

namespace AdjustMode {
  enum AdjustMode_t {
    NONE,
    HOURS,
    MINUTES,
    TEMPERATURE
  };  
}

#define CLK_PIN 3
#define DT_PIN 4
#define SWITCH_PIN 2

Encoder encoder(CLK_PIN, DT_PIN);

tmElements_t alarm_time;
int temperature = 90;
int mode = AdjustMode::NONE;

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for serial
  pinMode(SWITCH_PIN, INPUT);
  delay(200);
  // put your setup code here, to run once:
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds
  // Clear the buffer
  display.clearDisplay();
  display.display();
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}

void loop() {
  tmElements_t tm;
  int encoderPos = encoder.read();
  // read the state of the switch into a local variable:
  int reading = digitalRead(SWITCH_PIN);
  bool timeReadResult = false;

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        if (mode == AdjustMode::TEMPERATURE) {
          mode = AdjustMode::NONE;
        } else {
          mode++;
          switch(mode) {
            case AdjustMode::HOURS:
              encoder.write(alarm_time.Hour);
              break;
            case AdjustMode::MINUTES:
              encoder.write(alarm_time.Minute);
              break;
            case AdjustMode::TEMPERATURE:
              encoder.write(temperature);
              break;
            default:
              break;
          }
        }
      }
    }
  }
  
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
  switch(mode) {
    case AdjustMode::HOURS:
      alarm_time.Hour = (encoderPos < 0) ? 0 : encoderPos;
      alarm_time.Hour = (encoderPos > 23) ? 23 : encoderPos;
      break;
    case AdjustMode::MINUTES:
      alarm_time.Minute = (encoderPos < 0) ? 0 : encoderPos;
      alarm_time.Minute = (encoderPos > 59) ? 59 : encoderPos;
      break;
    case AdjustMode::TEMPERATURE:
      temperature = (encoderPos < 0) ? 0 : encoderPos;
      temperature = (encoderPos > 100) ? 100 : encoderPos;
      break;
    case AdjustMode::NONE:
      encoder.write(0);
      break;
    default:
      break;
  }

  if (mode == AdjustMode::NONE) {
    timeReadResult = RTC.read(tm);
  } else {
    timeReadResult = true;
    tm.Hour = alarm_time.Hour;
    tm.Minute = alarm_time.Minute;
    tm.Second = 0;
  }
  
  if (timeReadResult) {
    Serial.print("Ok, Time = ");
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.println();
    
    display.clearDisplay();
    
    display.setTextSize(2);             // Normal 1:1 pixel scale
    display.setTextColor(WHITE);        // Draw white text
    display.setCursor(10,10);             // Start at top-left corner
    display.print(tm.Hour, DEC);
    display.setCursor(31,10);             // Start at top-left corner
    display.print(F(":"));
    display.setCursor(40,10);             // Start at top-left corner
    display.print(tm.Minute, DEC);
//    display.setCursor(61,10);             // Start at top-left corner
//    display.print(F(":"));
//    display.setCursor(70,10);             // Start at top-left corner
//    display.println(tm.Second, DEC);
    display.display();
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    } else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      Serial.println();
    }
    delay(2000);
  }
}
