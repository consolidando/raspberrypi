#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include "gpio.h"

struct gpiod_line *led;

int main(void)
{
  const char *chipname = "gpiochip0";
  struct gpiod_chip *chip;
  struct gpiod_line *button;
  struct timespec ts = {1, 0};
  struct gpiod_line_event event;
  int ret;

  // Open GPIO chip
  chip = gpiod_chip_open_by_name(chipname);

  // Open GPIO lines
  led = gpiod_chip_get_line(chip, 21);
  button = gpiod_chip_get_line(chip, 6);

  // output
  gpiod_line_request_output(led, "test", GPIOD_LINE_ACTIVE_STATE_HIGH);
  // input
  gpiod_line_request_falling_edge_events(button, "test");

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

    ret = gpiod_line_event_read(button, &event);
    if (ret == 0)
    {
      // AC detected 
      if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
      {
        set_led_state(0);

        do
        {     
          // waits 1 second or cleans bounces   
          gpiod_line_event_wait(button, &ts);
          ret = gpiod_line_get_value(button);
          // AC has stopped
          if (ret == 1)
          {
            set_led_state(1);
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
