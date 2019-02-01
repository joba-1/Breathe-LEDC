/*
 * JoB ESP32 LEDC breathe example
 *
 * Setup LEDC so led fades in, calls interrupt to fade out and so on without further intervention.
 */

#include <stdio.h>          // printf()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  // for xTask* stuff
#include "freertos/queue.h" // for xQueue* stuff
#include "driver/ledc.h"    // for ledc led fading (nothing to do with ulp)


static xQueueHandle evt_queue = NULL;
static bool fade_inverted = false;


// ISR routine for changing fade direction of LEDC demo
static void IRAM_ATTR toggle_fade_isr( void *dummy ) {
  xQueueSendFromISR(evt_queue, &fade_inverted, NULL);  
  fade_inverted = !fade_inverted;
}


// Handle LEDC in task (got wdt if logs enabled if not: didnt help)
void handle_ledc( void *arg ) {
  // setup timer and channel similar to ledc_example_main.c
  ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_13_BIT,    // resolution of pwm duty
    .freq_hz         = 5000,                 // frequency of pwm signal
    .speed_mode      = LEDC_HIGH_SPEED_MODE, // timer mode
    .timer_num       = LEDC_TIMER_0          // timer index
  };

  // set configuration of timer0 for high speed channels
  ledc_timer_config(&ledc_timer);

  // Prepare individual configuration for a channel of the LED Controller 
  ledc_channel_config_t ledc_channel = {
    .channel    = LEDC_CHANNEL_0,
    .duty       = 0,
    .gpio_num   = 27,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .hpoint     = 0,
    .timer_sel  = LEDC_TIMER_0,
    .intr_type  = LEDC_INTR_FADE_END // not in ledc_example_main.c, but I guess is required
  };
  ESP_ERROR_CHECK( ledc_channel_config(&ledc_channel) );

  // Initialize fade service.
  ESP_ERROR_CHECK( ledc_fade_func_install(0) );

  // Set ISR routine to change fade direction -> aborts with ESP_ERR_NOT_FOUND!? 
  // Read the sources: only parameter flags can induce this error message:
  // ledc_isr_register(.., flags, ...) tried all permutations with ESP_INTR_FLAG_IRAM and ESP_INTR_FLAG_SHARED
  // -> esp_intr_alloc(source=ETS_LEDC_INTR_SOURCE, flags, ...)
  // -> esp_intr_alloc_intrstatus(source=ETS_LEDC_INTR_SOURCE, flags, ...) = ESP_ERR_NOT_FOUND
  // -> get_available_int(flags, cpu=xPortGetCoreID(), force=-1, source=ETS_LEDC_INTR_SOURCE) = -1

  // ESP_ERROR_CHECK( ledc_isr_register(toggle_fade_isr, &ledc_channel, 0, NULL) );

  // Start fading
  evt_queue = xQueueCreate(1, sizeof(uint32_t)); // never more than one event
  xQueueSend(evt_queue, &fade_inverted, 0);  
  printf("Started LEDC\n");

  bool inverted;
  for(;;) {
    if( xQueueReceive(evt_queue, &inverted, portMAX_DELAY)) {
      printf("Fade %s\n", inverted ? "out" : "in");
      ESP_ERROR_CHECK( ledc_set_fade_with_time(ledc_channel.speed_mode, ledc_channel.channel, inverted ? 0 : 4000, 3000) );
      ESP_ERROR_CHECK( ledc_fade_start(ledc_channel.speed_mode, ledc_channel.channel, LEDC_FADE_NO_WAIT) );
    }
  }
}


void app_main()
{
  printf("Hello Breathe LEDC!\n");
  // Test pinning to core 0 because no irq on core 1 (does not help...):
  xTaskCreatePinnedToCore(handle_ledc, "ledc_task", 4*configMINIMAL_STACK_SIZE, NULL, 4, NULL, 0);
}

