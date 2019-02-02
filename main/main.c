/*
 * JoB ESP32 LEDC breathe example
 *
 * Setup LEDC so led fades in, calls interrupt to fade out and so on without further intervention.
 */

#include <stdio.h>           // printf()
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h" // for xSemaphore* stuff (wating for fade end)
#include "driver/ledc.h"     // for ledc led fading api


#define FADE_MS    4000
#define MAX_DUTY   4000
#define TIMEOUT_MS 5000


static SemaphoreHandle_t fading = NULL; // to wait for fade end
static bool fade_inverted       = true; // direction of last fade


// ISR routine for changing fade direction of LEDC demo
static void IRAM_ATTR fade_ended_isr( void *dummy ) {
  xSemaphoreGiveFromISR(fading, NULL);
}


void app_main()
{
  printf("Hello Breathe LEDC!\n");

  // Setup timer and channel similar to ledc_example_main.c
  ledc_timer_config_t ledc_timer = {
    .duty_resolution = LEDC_TIMER_12_BIT,    // resolution of pwm duty 0..4095
    .freq_hz         = 5000,                 // frequency of pwm signal
    .speed_mode      = LEDC_HIGH_SPEED_MODE, // timer mode
    .timer_num       = LEDC_TIMER_0          // timer index
  };

  // Set configuration of timer0 for high speed channels
  ledc_timer_config(&ledc_timer);

  // Prepare individual configuration for a channel of the LED Controller 
  ledc_channel_config_t ledc_channel = {
    .channel    = LEDC_CHANNEL_0,
    .duty       = 0,
    .gpio_num   = 27,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .hpoint     = 0,
    .timer_sel  = LEDC_TIMER_0
  };

  // Set configuration of LEDC channel
  ESP_ERROR_CHECK( ledc_channel_config(&ledc_channel) );

  // Initialize fade service (need to share or ledc_isr_register() below will fail).
  ESP_ERROR_CHECK( ledc_fade_func_install(ESP_INTR_FLAG_IRAM|ESP_INTR_FLAG_SHARED) );

  // Set ISR routine to change fade direction 
  ESP_ERROR_CHECK( ledc_isr_register(fade_ended_isr, &ledc_channel, ESP_INTR_FLAG_IRAM|ESP_INTR_FLAG_SHARED, NULL) );

  // Taken initially and while fade is ongoing
  fading = xSemaphoreCreateBinary();

  printf("Start LEDC Breathing\n");
  xSemaphoreGive(fading);

  for(;;) {
    if( !xSemaphoreTake(fading, TIMEOUT_MS/portTICK_PERIOD_MS) ) {
      printf("Fade did not end in %us. Forcing now\n", TIMEOUT_MS/1000);
      xSemaphoreGive(fading); // Necessary? Or is it given after the timeout anyways?
    }
    fade_inverted = !fade_inverted;
    ESP_ERROR_CHECK( ledc_set_fade_with_time(ledc_channel.speed_mode, ledc_channel.channel, fade_inverted ? 0 : MAX_DUTY, FADE_MS) );
    ESP_ERROR_CHECK( ledc_fade_start(ledc_channel.speed_mode, ledc_channel.channel, LEDC_FADE_NO_WAIT) );
  }
}

