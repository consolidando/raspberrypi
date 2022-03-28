#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <jansson.h>

#include "rest.h"
#include "jwt_create.h"

struct gpiod_line *lamp;

char* notificationJSONCreate()
{
   // current time
  time_t current_time = time(NULL);
  struct tm *local_time = localtime(&current_time);
   char aux[100];
  char *dump;

  // json notification  
  json_t *root = json_object();

  json_object_set_new(root, "title", json_string("Desk Alarm"));
  sprintf(aux, "Motion detected at %s", asctime(local_time));
  json_object_set_new(root, "body", json_string(aux));

  dump = json_dumps(root, 0);

  json_decref(root);

  return(dump);

}

// it sends an alarm to a server which forwards it to the mobile subscriber
void sendAlarm()
{
  char *jwt;
 
  // creates authentification token
  jwt = jwtCreate("rsa_private.pem", "Project Id", "MyFistDeviceID");

  // send an alarm
  restPostJSON("192.168.1.36",
           "8080",
           "resource/alarm/notification",
           notificationJSONCreate(),
           jwt);

  
}

// lamp is on when GPIO is low level
void turn_on_lamp()
{  
  gpiod_line_set_value(lamp, 0);
}

// lamp is off when GPIO is high level
void turn_off_lamp()
{
  gpiod_line_set_value(lamp, 1);
}

int main(void)
{
  const char *chipname = "gpiochip0";
  struct gpiod_chip *chip;
  struct gpiod_line *motion_sensor;
  struct timespec ts = {1, 0};
  struct gpiod_line_event event[3];
  int ret;

  // Open GPIO chip
  chip = gpiod_chip_open_by_name(chipname);

  // Open GPIO lines
  lamp = gpiod_chip_get_line(chip, 21);
  motion_sensor = gpiod_chip_get_line(chip, 6);

  // output in state high by default
  gpiod_line_request_output(lamp, "test", GPIOD_LINE_ACTIVE_STATE_HIGH);
  // input with pull-up resistor
  gpiod_line_request_falling_edge_events_flags(motion_sensor, "test", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);

  for (;;)
  {
    for (;;)
    {
      ret = gpiod_line_event_wait(motion_sensor, &ts);
      if (ret == 1)
      {
        break;
      }
    }

    ret = gpiod_line_event_read(motion_sensor, &event[0]);
    if (ret == 0)
    {      
      // AC detected - motion detected
      if (event[0].event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
      {
        turn_on_lamp();

        sendAlarm();

        do
        {          
          usleep(500000); // 500ms
          
          // AC is still detected ?
          ret = gpiod_line_get_value(motion_sensor);          
          if (ret == 1)
          {
            turn_off_lamp();

            // bounces --> multiple events
            usleep(50000); // 50ms
            gpiod_line_event_wait(motion_sensor, &ts);
            ret = gpiod_line_event_read_multiple(motion_sensor, event, 3);
            break;
          }
        } while (1);
      }
    }
  }

  // turn off lamp;
  gpiod_line_set_value(lamp, 0);
  // Release lines and chip
  gpiod_line_release(lamp);
  gpiod_line_release(motion_sensor);
  gpiod_chip_close(chip);
  //
  return 0;
}




