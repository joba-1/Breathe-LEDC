/*
 * JoB ESP32 LEDC breathe example
 *
 * Setup LEDC so led fades in and out repeatedly without further intervention.
 */

#include <stdio.h>                   // printf()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"           // for timing
#include "driver/ledc.h"             // for ledc led fading api

#define LED_PIN   27
#define CHANNEL   LEDC_CHANNEL_0
#define MODE      LEDC_HIGH_SPEED_MODE
#define FADE_MS   4000
#define DUTY_BITS 12

#define MAX_DUTY  (1<<DUTY_BITS)

bool fade_inverted = true;           // direction of last fade

void ledc_init() {
  // Setup timer and channel similar to ledc_example_main.c
  ledc_timer_config_t ledc_timer = {
    .duty_resolution = DUTY_BITS,    // resolution of pwm duty
    .freq_hz         = 5000,         // frequency of pwm signal
    .speed_mode      = MODE,         // timer mode
    .timer_num       = LEDC_TIMER_0  // timer index
  };

  // Set configuration of timer0 for high speed channels
  ledc_timer_config(&ledc_timer);

  // Prepare individual configuration for a channel of the LED Controller
  ledc_channel_config_t ledc_channel = {
    .channel    = CHANNEL,
    .duty       = 0,
    .gpio_num   = LED_PIN,
    .speed_mode = MODE,
    .hpoint     = 0,
    .timer_sel  = LEDC_TIMER_0
  };

  // Set configuration of LEDC channel
  ESP_ERROR_CHECK( ledc_channel_config(&ledc_channel) );

  // Initialize fade service (need to share or ledc_isr_register() below will fail).
  ESP_ERROR_CHECK( ledc_fade_func_install(0) );
}

void timing( bool inverted ) {
  static TickType_t started = 0;
  TickType_t now = xTaskGetTickCount();
  printf("%u ms: fading %s\n", (now-started)*portTICK_PERIOD_MS, inverted ? "out" : "in");
  started = now;
}

void ledc_fade() {
  for(;;) {
    fade_inverted = !fade_inverted;
    timing(fade_inverted);
    ESP_ERROR_CHECK( ledc_set_fade_with_time(MODE, CHANNEL, fade_inverted ? 0 : MAX_DUTY, FADE_MS) );
    ESP_ERROR_CHECK( ledc_fade_start(MODE, CHANNEL, LEDC_FADE_WAIT_DONE) );
  }
}

void app_main()
{
  printf("Hello Breathe LEDC!\n");

  ledc_init();

  printf("Start LEDC Breathing\n");

  xTaskCreate(ledc_fade, "ledc_fade", 2*configMINIMAL_STACK_SIZE, NULL, 4, NULL);

  printf("LEDC Task running\n");
}
