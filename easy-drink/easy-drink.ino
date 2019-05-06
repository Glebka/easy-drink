#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Time.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
#include <Encoder.h>
#include <stdio.h>
#include "types.h"

static int serial_fputchar(const char ch, FILE *stream) { Serial.write(ch); return ch; }
static FILE *serial_stream = fdevopen(serial_fputchar, NULL);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     5 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Encoder pins definitions
#define CLK_PIN 2
#define DT_PIN 3
#define SWITCH_PIN 4

Encoder encoder(CLK_PIN, DT_PIN);

#define REDRAW_INTERVAL 500

EventQueue_t queue;

void queue_init(EventQueue_t & q) {
  q.start_ = 0;
  q.end_ = 0;
  q.data[0] = Events::NO_EVENT;
}

void queue_push_back(EventQueue_t& q, Events::Events_t e) {
  q.data[q.end_] = e;  
  q.end_ = (q.end_ + 1 >= EVENT_QUEUE_SIZE) ? 0 : q.end_ + 1;
}

Events::Events_t queue_pop_front(EventQueue_t& q) {
 Events::Events_t result = Events::NO_EVENT;
  if (q.start_ != q.end_) {
    result = q.start_;
    q.start_ = (q.start_ + 1 >= EVENT_QUEUE_SIZE) ? 0 : q.start_ + 1;
  }
  return result;
}


StateData stateData;

ViewFunc state2View[] = {
  standby_view  // STANDBY
};

HandlerFunc event2Handler[] = {
  handle_next_tick, // NEXT_TICK
  handle_redraw
};

char textBuf[20];

void init_state() {
  stateData.state = State::STANDBY;
}

void dispatch(Events::Events_t e) {
  queue_push_back(queue, e);  
}

void handle_next_tick() {
  static unsigned long last_tick = 0;  
  if (millis() - last_tick > REDRAW_INTERVAL) {
    last_tick = millis();
    RTC.read(stateData.curr_time);
    dispatch(Events::REDRAW);
  }
  dispatch(Events::NEXT_TICK);
}

void handle_redraw() {  
  state2View[stateData.state]();
}

void standby_view() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10,10);
  sprintf(textBuf, "%02d:%02d", stateData.curr_time.Hour, stateData.curr_time.Minute);
  display.print(textBuf);
  display.display();
}

/**
 * Prints message on the display
 * Clears it before printing
 * 
 * @param msg - null terminated ascii string
 */
void oled_print(const char* msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10,10);
  display.print(msg);
  display.display();
}

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for serial

  pinMode(SWITCH_PIN, INPUT);
  delay(200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  } 

  display.clearDisplay();  
  display.display();

  /**
   *  Checking for real time clock availability
   */  
  byte readResult = RTC.read(stateData.curr_time);
  if (RTC.chipPresent()) {      
      if (!readResult) {        
        oled_print("E: RTC not set");        
        for(;;); // Don't proceed, loop forever
      }      
  } else {
      oled_print("E: RTC is NC");      
      for(;;); // Don't proceed, loop forever
  }

  init_state();
  queue_init(queue);
  dispatch(Events::NEXT_TICK);
}

void loop() {    
  Events::Events_t e = queue_pop_front(queue);
  if (e != Events::NO_EVENT) {
    event2Handler[e]();
  } else {
    dispatch(Events::NEXT_TICK);
  }
  /*tmElements_t tm;
  bool timeReadResult = false;
  if (encoderPos != encoder.read() / 4) {
    encoderPos = encoder.read() / 4;
    Serial.println(encoderPos);
  }*/ 
  /*int encoderPos = encoder.read();
  // read the state of the switch into a local variable:
  int reading = digitalRead(SWITCH_PIN);
  

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
  }*/

  /*if (mode == AdjustMode::NONE) {
    timeReadResult = RTC.read(tm);
  } else {
    timeReadResult = true;
    tm.Hour = alarm_time.Hour;
    tm.Minute = alarm_time.Minute;
    tm.Second = 0;
  }
  
  if (timeReadResult) {
    //Serial.print("Ok, Time = ");
    //print2digits(tm.Hour);
    //Serial.write(':');
    //print2digits(tm.Minute);
    //Serial.write(':');
    //print2digits(tm.Second);
    //Serial.print(", Date (D/M/Y) = ");
    //Serial.print(tm.Day);
    //Serial.write('/');
    //Serial.print(tm.Month);
    //Serial.write('/');
    //Serial.print(tmYearToCalendar(tm.Year));
    //Serial.println();
    
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
  }*/
}
