/**
 * This file uses code extracted from an example in the repository of Ulfius
 */

#include <stdio.h>
#include <string.h>
#include <ulfius.h>
#include "gpio.h"
#include "rest.h"

#define PORT 8080
#define STATIC_FILE  "../static/led/index.html"
#define STATIC_FILE_CHUNK 256


/**
 * main function
 */
int main(void) 
{
  struct _u_instance instance;
 
  // Initialize instance with the port number
  if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK) 
  {
    fprintf(stderr, "Error ulfius_init_instance, abort\n");
    return(1);
  }

  // Endpoint list declaration
  ulfius_add_endpoint_by_val(&instance, "GET", "/led", NULL, 0, &callback_get_led, NULL);
  ulfius_add_endpoint_by_val(&instance, "PUT", "/led", NULL, 0, &callback_put_led_state, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/websocket", NULL, 0, &callback_websocket, NULL);

  // Start the framework
  if (ulfius_start_framework(&instance) == U_OK) 
  {
    printf("Start framework on port %d\n", instance.port);

    // Start gpios
    start_gpio();
  } 
  else 
  {
    fprintf(stderr, "Error starting framework\n");
  }
  printf("End framework\n");

  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);

  return 0;
}



/**
 * Streaming callback function to ease sending large files
 */
static ssize_t callback_static_file_stream(void * cls, uint64_t pos, char * buf, size_t max) 
{
  if (cls != NULL) 
  {
    return fread (buf, 1, max, (FILE *)cls);
  } 
  else 
  {
    return U_STREAM_END;
  }
}

/**
 * Cleanup FILE* structure when streaming is complete
 */
static void callback_static_file_stream_free(void * cls) 
{
  if (cls != NULL) 
  {
    fclose((FILE *)cls);
  }
}

/**
 * Callback function for the web application on /led url call, return /static/index.html
 */
int callback_get_led (const struct _u_request * request, struct _u_response * response, void * user_data) 
{
   size_t length;
  FILE * f;
  char * file_requested, * file_path = STATIC_FILE;

    if (access(file_path, F_OK) != -1) 
    {
      f = fopen (file_path, "rb");
      if (f) 
      {
        fseek (f, 0, SEEK_END);
        length = ftell (f);
        fseek (f, 0, SEEK_SET);
        
        u_map_put(response->map_header, "Content-Type", "text/html");       
        
        if (ulfius_set_stream_response(response, 200, callback_static_file_stream, callback_static_file_stream_free, length, STATIC_FILE_CHUNK, f) != U_OK) 
        {
          y_log_message(Y_LOG_LEVEL_ERROR, "callback_static_file - Error ulfius_set_stream_response");
        }
      }
    }     
    else 
    {

        ulfius_set_string_body_response(response, 404, "File not found");
    }        
  
  return U_CALLBACK_CONTINUE;
}


/**
 * Set led on or off according to the json "state" value
 */
int callback_put_led_state (const struct _u_request * request, struct _u_response * response, void * user_data) 
{
  json_t * json_led = ulfius_get_json_body_request(request, NULL); 
  json_t * json_body = NULL;
  json_int_t state;

  if (json_led != NULL) 
  {    
    state = json_integer_value(json_object_get(json_led,"state")); 

    // turn on /off the led
    set_led_state(state);
    //
    json_body = json_object();
    json_object_set_new(json_body, "state", json_integer(state));
    ulfius_set_json_body_response(response, 200, json_body);
    //
    json_decref(json_led);
    json_decref(json_body);
  }  
  else
  {
    ulfius_set_json_body_response(response, 400, NULL);    
  }

  return U_CALLBACK_CONTINUE;
}



/**
 * websocket_manager_callback
 * check for changes on the led and send them to the server clients
 */
void websocket_manager_callback(const struct _u_request * request,
                               struct _websocket_manager * websocket_manager,
                               void * websocket_manager_user_data) 
{
  int sent_state = -1, state;

  for (;;) 
  {
    state = get_led_state();
    if (state != sent_state)
    {
      sent_state = state;
      json_t * message = json_pack("{si}", "state", state);
      char * json = json_dumps(message, JSON_COMPACT);
      int ret = ulfius_websocket_send_message(websocket_manager, U_WEBSOCKET_OPCODE_TEXT, strlen(json), json);
      o_free(json);
      json_decref(message);
    }

    if (ulfius_websocket_wait_close(websocket_manager, 1000) != U_WEBSOCKET_STATUS_OPEN)
      break;
  }

  y_log_message(Y_LOG_LEVEL_DEBUG, "Closing websocket_manager_callback");
}


int callback_websocket (const struct _u_request * request, struct _u_response * response, void * user_data) 
{  
  int ret;

  if ((ret = ulfius_set_websocket_response(response, NULL, NULL, &websocket_manager_callback, NULL, NULL, NULL, NULL, NULL)) == U_OK) 
  {
    //ulfius_add_websocket_deflate_extension(response);
    // ...
    return U_CALLBACK_CONTINUE;
  } 
  else 
  {
    return U_CALLBACK_ERROR;
  }
}





