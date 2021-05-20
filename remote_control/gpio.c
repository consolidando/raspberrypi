#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>


struct gpiod_line *led;  

int start_gpio(void)
{
  const char *chipname = "gpiochip0";
  struct gpiod_chip *chip;   
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
    // waits for 10 seconds for button changes
    ret = gpiod_line_event_wait(button, &ts);           
    if (ret == 1) 
    {      
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

void set_led_state(int on)
{
    gpiod_line_set_value(led, on);
}

int get_led_state()
{
  return(gpiod_line_get_value(led));
}
