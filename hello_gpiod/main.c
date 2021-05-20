/*
 * This file is part of the example in the
 * Post: http://diy.elmolidelanoguera.com/2021/04/raspberry-pi-4-model-b.html
 * in the
 * Blog: http://diy.elmolidelanoguera.com/
 * 
 * Description:
 * Simple gpiod library example of toggling a LED connected to a GPIO line
 * using a pushbutton connected to another GPIO line. 
 * If the button is not pressed in 10 seconds the program ends.
 */

#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  const char *chipname = "gpiochip0";
  struct gpiod_chip *chip;
  struct gpiod_line *led;    
  struct gpiod_line *button; 
  struct timespec ts = { 10, 0 };
  int ret;
  bool bOn = true;

  // Open GPIO chip
  chip = gpiod_chip_open_by_name(chipname);

  // Open GPIO lines
  led = gpiod_chip_get_line(chip, 21);
  button = gpiod_chip_get_line(chip, 26);

  // output
  gpiod_line_request_output(led, "test", GPIOD_LINE_ACTIVE_STATE_HIGH);
  // input
  gpiod_line_request_rising_edge_events(button, "test");
   
  for(;;) 
  {
    // turn on / off the led
    gpiod_line_set_value(led, bOn);
      
    // waits for 10 seconds for button changes
    ret = gpiod_line_event_wait(button, &ts);           
    if (ret == 1) 
    {
      struct gpiod_line_event event;
      // gets event
      gpiod_line_event_read(button, &event);
      // changes state led
      bOn = !bOn;        
    }    
    else
    {
      // timeout ends
      break;
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
