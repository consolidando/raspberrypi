#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <jansson.h>

#include "gpio.h"
#include "post.h"
#include "jwt_create.h"

struct gpiod_line *led;

void sendAlarm()
{
  char *jwt;
  char aux[256];

  jwt = jwtCreate("rsa_private.pem", "Project Id", "MyFistDeviceID");

  // notification
  time_t current_time = time(NULL);
  struct tm *local_time = localtime(&current_time);
  sprintf(aux, "Motion detected at %s", asctime(local_time));

  json_t *root = json_object();
  json_object_set_new(root, "title", json_string("Desk Alarm"));
  json_object_set_new(root, "body", json_string(aux));

  // send an alarm
  postJSON("192.168.1.36",
           "8080",
           "resource/alarm/notification",
           json_dumps(root, 0),
           jwt);

  json_decref(root);
}

int main(void)
{
  const char *chipname = "gpiochip0";
  struct gpiod_chip *chip;
  struct gpiod_line *button;
  struct timespec ts = {1, 0};
  struct gpiod_line_event event[3];
  int ret;

  // Open GPIO chip
  chip = gpiod_chip_open_by_name(chipname);

  // Open GPIO lines
  led = gpiod_chip_get_line(chip, 21);
  button = gpiod_chip_get_line(chip, 6);

  // output
  gpiod_line_request_output(led, "test", GPIOD_LINE_ACTIVE_STATE_HIGH);
  // input with pull-up resistor
  gpiod_line_request_falling_edge_events_flags(button, "test", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);

  for (;;)
  {
    for (;;)
    {
      ret = gpiod_line_event_wait(button, &ts);
      if (ret == 1)
      {
        break;
      }
    }

    ret = gpiod_line_event_read_multiple(button, event, 3);
    if (ret >= 0)
    {
      printf("Events %d.\n\n", ret);
      // AC detected - motion detected
      if (event[0].event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
      {
        set_led_state(0);

        sendAlarm();

        do
        {          
          usleep(500000); // 500ms
          
          // AC is still detected ?
          ret = gpiod_line_get_value(button);          
          if (ret == 1)
          {
            set_led_state(1);

            // bounces --> multiple events
            usleep(50000); // 500ms
            gpiod_line_event_wait(button, &ts);
            ret = gpiod_line_event_read_multiple(button, event, 3);
            break;
          }
        } while (1);
      }
    }
  }

  // turn off led;
  gpiod_line_set_value(led, 0);
  // Release lines and chip
  gpiod_line_release(led);
  gpiod_line_release(button);
  gpiod_chip_close(chip);
  //
  return 0;
}

void set_led_state(int on)
{
  gpiod_line_set_value(led, on);
}

int get_led_state()
{
  return (gpiod_line_get_value(led));
}
