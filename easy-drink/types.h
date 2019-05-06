#ifndef FILE_FOO_SEEN
#define FILE_FOO_SEEN

#include <TimeLib.h>

namespace AdjustMode {
  enum AdjustMode_t {
    NONE,
    HOURS,
    MINUTES,
    TEMPERATURE
  };  
}

namespace Events {
  enum Events_t {    
    NEXT_TICK,
    REDRAW,
    ENCODER_PRESS,
    ENCODER_INC,
    ENCODER_DEC,
    ALARM,
    NO_EVENT
  };
}

#define EVENT_QUEUE_SIZE 10

struct EventQueue_t {
  Events::Events_t data[EVENT_QUEUE_SIZE];
  int start_;
  int end_;
};

namespace State {
  enum State_t {
    STANDBY,
    TIME_HOUR,
    TIME_MINUTE,
    ALARM_HOUR,
    ALARM_MINUTE,
    HEATING,
    BREWING,
    DRINK_DONE    
  };
}

struct StateData {
  tmElements_t curr_time;
  tmElements_t alarm_time;
  int temperature;
  State::State_t state;
};

typedef void (*ViewFunc) ();
typedef void (*HandlerFunc) ();

#endif /* !FILE_FOO_SEEN */
