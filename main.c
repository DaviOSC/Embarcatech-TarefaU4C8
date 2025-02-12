#include <stdio.h>            
#include "pico/stdlib.h"      
#include "hardware/adc.h"     
#include "hardware/pwm.h"     
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pico/bootrom.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define NUM_PIXELS 25
#define BAUD_RATE 115200 // Define a taxa de transmissão

#define VRY_PIN 26  
#define VRX_PIN 27
#define SW_PIN 22
#define LED_PIN_RED 13
#define LED_PIN_GREEN 11
#define LED_PIN_BLUE 12
#define PIN_BUTTON_A 5
#define PIN_BUTTON_B 6

#define CALIBRATION_OFFSET 30
#define DEBOUNCE_TIME_MS 300

#define SQUARE_SIZE 8

bool green_led_state = false;

bool pwm_led_state = false;

int border_style = 0 ;
absolute_time_t last_interrupt_time = {0};
ssd1306_t ssd;



uint pwm_init_gpio(uint gpio, uint wrap) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice_num, wrap);
    
    pwm_set_enabled(slice_num, true);  
    return slice_num;  
}

static void gpio_irq_handler(uint gpio, uint32_t events)
{
  // Obter o tempo atual para o debounce
  absolute_time_t current_time = get_absolute_time();

  if (absolute_time_diff_us(last_interrupt_time, current_time) < DEBOUNCE_TIME_MS * 1000)
  {
    return; // Ignora a interrupção se estiver dentro do tempo de debounce
  }
  else
  {
    last_interrupt_time = current_time;
  }

  // Verifica qual botão foi pressionado e alterna o estado do LED correspondente
  if (gpio == PIN_BUTTON_A)
  {
    if(pwm_led_state)
    {
      pwm_led_state = false;
    }
    else
    {
      pwm_led_state = true;
    }
  }
  else if (gpio == SW_PIN)
  {
    if (green_led_state)
    {
      green_led_state = false;
    }
    else
    {
      green_led_state = true;
    }

    if(border_style<=2)
    {
      border_style++;
    }
    else
    {
      border_style = 0;
    }
    if(border_style == 3)
    {
      ssd1306_rect(&ssd, 3, 3, 122, 60, 0, 0);
      ssd1306_rect(&ssd, 4, 4, 120, 58, 0, 0);
      ssd1306_rect(&ssd, 5, 5, 118, 56, 0, 0);
      ssd1306_send_data(&ssd);
    }
  }
  else if (gpio == PIN_BUTTON_B)
  {
    // Ativar o BOOTSEL
    printf("BOOTSEL ativado.\n");
    reset_usb_boot(0, 0);
  }

  // Atualiza o estado dos LEDs
  gpio_put(LED_PIN_GREEN, green_led_state);
}

int main() {
    stdio_init_all();
    adc_init(); 
    adc_gpio_init(VRX_PIN); 
    adc_gpio_init(VRY_PIN);
    
    gpio_init(PIN_BUTTON_A);
    gpio_init(PIN_BUTTON_B);
    gpio_init(SW_PIN);
    gpio_init(LED_PIN_GREEN);
    gpio_init(LED_PIN_BLUE);
    gpio_init(LED_PIN_RED);

    gpio_set_dir(PIN_BUTTON_A, GPIO_IN);
    gpio_set_dir(PIN_BUTTON_B, GPIO_IN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);
    gpio_pull_up(PIN_BUTTON_A);
    gpio_pull_up(PIN_BUTTON_B);
    gpio_pull_up(SW_PIN);

    
    gpio_set_irq_enabled_with_callback(PIN_BUTTON_A, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(PIN_BUTTON_B, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(SW_PIN, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
    
    uint pwm_wrap = 4096;  
    uint pwm_slice_red = pwm_init_gpio(LED_PIN_RED, pwm_wrap);
    uint pwm_slice_blue = pwm_init_gpio(LED_PIN_BLUE, pwm_wrap);
    
    i2c_init(I2C_PORT, 400 * 1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    adc_select_input(0);  
    uint16_t vry_value = adc_read(); 
    adc_select_input(1);
    uint16_t vrx_value = adc_read();
    
    uint16_t vrx_calibration = vrx_value;
    uint16_t vry_calibration = vry_value;
    
    uint32_t last_print_time = 0; 

    int square_pos_x = 0;
    int square_pos_y = 0;
    int prev_square_pos_x = 0;
    int prev_square_pos_y = 0;
    while (true)
    {

      adc_select_input(0);  
      uint16_t vry_value = adc_read(); 
      adc_select_input(1);
      uint16_t vrx_value = adc_read();

      // Mapear valores do joystick para PWM
      int16_t calibrated_vrx_value = vrx_value - vrx_calibration;
      int16_t calibrated_vry_value = vry_value - vry_calibration;
      int16_t mapped_vrx_value = calibrated_vrx_value;
      int16_t mapped_vry_value = calibrated_vry_value;

      if (mapped_vrx_value < 0) {
          mapped_vrx_value = -mapped_vrx_value;
      }
      if (mapped_vry_value < 0) {
          mapped_vry_value = -mapped_vry_value;
      }
      if (mapped_vrx_value < CALIBRATION_OFFSET && mapped_vrx_value > 0) {
          mapped_vrx_value = 0;
      }
      if (mapped_vry_value > -CALIBRATION_OFFSET && mapped_vry_value < 0) {
          mapped_vry_value = 0;
      }

      if(pwm_led_state)
      {
        pwm_set_gpio_level(LED_PIN_RED, mapped_vrx_value * 2); 
        pwm_set_gpio_level(LED_PIN_BLUE, mapped_vry_value * 2);
      }
      else
      {
        pwm_set_gpio_level(LED_PIN_RED, 0); 
        pwm_set_gpio_level(LED_PIN_BLUE, 0);
      }
      
      switch (border_style)
      {
      case 0:
          ssd1306_rect(&ssd, 3, 3, 122, 60, 1, 0); // Desenha um retângulo
          ssd1306_send_data(&ssd);
          break;
      case 1:
          ssd1306_rect(&ssd, 3, 3, 122, 60, 1, 0);
          ssd1306_rect(&ssd, 4, 4, 120, 58, 1, 0);
          ssd1306_send_data(&ssd);
          break; 
      case 2:
          ssd1306_rect(&ssd, 3, 3, 122, 60, 1, 0); // Desenha um retângulo
          ssd1306_rect(&ssd, 4, 4, 120, 58, 1, 0);
          ssd1306_rect(&ssd, 5, 5, 118, 56, 1, 0);
          ssd1306_send_data(&ssd);
          break;        
      default:
          break;
      }

        square_pos_x = (WIDTH - SQUARE_SIZE) / 2 + (calibrated_vrx_value * (WIDTH - SQUARE_SIZE) / 4096);
        square_pos_y = ((HEIGHT - SQUARE_SIZE) / 2 + (-calibrated_vry_value * (HEIGHT - SQUARE_SIZE) / 4096));  

        // Limpar a tela e desenhar o quadrado na nova posição
        ssd1306_rect(&ssd, prev_square_pos_y, prev_square_pos_x, SQUARE_SIZE, SQUARE_SIZE, 0, 1);
        ssd1306_rect(&ssd, square_pos_y, square_pos_x, SQUARE_SIZE, SQUARE_SIZE, 1, 1);
        ssd1306_send_data(&ssd);
        prev_square_pos_x = square_pos_x;
        prev_square_pos_y = square_pos_y;

        float duty_cycle_red = (mapped_vrx_value / 1941.0) * 100;  
      float duty_cycle_blue = (mapped_vry_value / 2047.0) * 100;  
      uint32_t current_time = to_ms_since_boot(get_absolute_time());  
      if (current_time - last_print_time >= 1000) {  
          printf("VRX: %u\n", vrx_value);
          printf("VRY: %u\n", vry_value);
          printf("Calibrated VRX: %d\n", calibrated_vrx_value);
          printf("Calibrated VRY: %d\n", calibrated_vry_value);
          printf("Mapped VRX: %d\n", mapped_vrx_value);
          printf("Mapped VRY: %d\n", mapped_vry_value); 
          printf("Square Pos X: %d\n", square_pos_x);
          printf("Square Pos Y: %d\n", square_pos_y);
          printf("Duty Cycle LED Red: %.2f%%\n", duty_cycle_red); 
          printf("Duty Cycle LED Blue: %.2f%%\n", duty_cycle_blue); 
          printf("Border Style: %d\n", border_style);
          last_print_time = current_time;  
      }

      sleep_ms(50);
    }

    return 0;  
}
