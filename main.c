#include <stdio.h>            
#include "pico/stdlib.h"      
#include "hardware/adc.h"     
#include "hardware/pwm.h"     
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "pico/bootrom.h"

// Definições do display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Definições de pinos do joystick, botões e LEDs
#define VRY_PIN 26  
#define VRX_PIN 27
#define SW_PIN 22
#define LED_PIN_RED 13
#define LED_PIN_GREEN 11
#define LED_PIN_BLUE 12
#define PIN_BUTTON_A 5
#define PIN_BUTTON_B 6


#define CALIBRATION_OFFSET 50 // Offset para calibração do joystick


#define DEBOUNCE_TIME_MS 300 // Tempo de debounce em ms

#define SQUARE_SIZE 8 // Tamanho do quadrado

bool green_led_state = false; // Estado inicial do LED verde
bool pwm_led_state = false; // Estado inicial da ativação do LED com PWM
int border_style = 0; // Estilo da borda do display

absolute_time_t last_interrupt_time = {0};
ssd1306_t ssd; 

// Inicializa o PWM para um pino GPIO específico
uint pwm_init_gpio(uint gpio, uint wrap) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice_num, wrap);
    
    pwm_set_enabled(slice_num, true);  
    return slice_num;  
}

// Função de tratamento de interrupção do GPIO
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

  // Ativa ou desativa a funcionalidade do LED com PWM quando o botão A é pressionado
  if (gpio == PIN_BUTTON_A)
  {
    pwm_led_state = !pwm_led_state;
  }
  // Alterna o estado do LED verde e o estilo da borda do display quando o botão do joystick é pressionado
  else if (gpio == SW_PIN)
  {
    green_led_state = !green_led_state;

    if(border_style<=2)
    {
      border_style++;
    }
    else
    {
      border_style = 0;
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
  // Inicializa o ADC e os pinos do joystick
  adc_init(); 
  adc_gpio_init(VRX_PIN); 
  adc_gpio_init(VRY_PIN);
  gpio_init(SW_PIN);
  
  // Inicializa os pinos dos botões e LEDs
  gpio_init(PIN_BUTTON_A);
  gpio_init(PIN_BUTTON_B);
  gpio_init(LED_PIN_GREEN);
  gpio_init(LED_PIN_BLUE);
  gpio_init(LED_PIN_RED);

  // Configura os pinos dos botões e LEDs
  gpio_set_dir(PIN_BUTTON_A, GPIO_IN);
  gpio_set_dir(PIN_BUTTON_B, GPIO_IN);
  gpio_set_dir(SW_PIN, GPIO_IN);
  gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
  gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);
  gpio_set_dir(LED_PIN_RED, GPIO_OUT);
  gpio_pull_up(PIN_BUTTON_A);
  gpio_pull_up(PIN_BUTTON_B);
  gpio_pull_up(SW_PIN);

  // Configura a interrupção dos botões 
  gpio_set_irq_enabled_with_callback(PIN_BUTTON_A, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(PIN_BUTTON_B, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(SW_PIN, GPIO_IRQ_EDGE_FALL, 1, &gpio_irq_handler);
  
  // Configura o pwm para os LEDs
  uint pwm_wrap = 4096;  
  uint pwm_slice_red = pwm_init_gpio(LED_PIN_RED, pwm_wrap);
  uint pwm_slice_blue = pwm_init_gpio(LED_PIN_BLUE, pwm_wrap);
  
  i2c_init(I2C_PORT, 400 * 1000); // Inicializa o barramento I2C
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configura o pino SDA
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configura o pino SCL
  gpio_pull_up(I2C_SDA); // Habilita o pull-up no pino SDA
  gpio_pull_up(I2C_SCL); // Habilita o pull-up no pino SCL
  
  // Inicializa o display OLED
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
  ssd1306_config(&ssd);
  ssd1306_send_data(&ssd);

  // Lê os valores iniciais do joystick para calibração
  adc_select_input(0);  
  uint16_t vry_value = adc_read(); 
  adc_select_input(1);
  uint16_t vrx_value = adc_read();
  
  // Calibração do joystick
  uint16_t vrx_calibration = vrx_value;
  uint16_t vry_calibration = vry_value;

  // Inicializa as variáveis de posição do quadrado
  int square_pos_x = 0;
  int square_pos_y = 0;
  int prev_square_pos_x = 0;
  int prev_square_pos_y = 0;

  while (true)
  {
    // Lê os valores do joystick
    adc_select_input(0);  
    uint16_t vry_value = adc_read(); 
    adc_select_input(1);
    uint16_t vrx_value = adc_read();

    // Mapear valores do joystick para PWM
    int16_t calibrated_vrx_value = vrx_value - vrx_calibration;
    int16_t calibrated_vry_value = vry_value - vry_calibration;
    int16_t mapped_vrx_value = calibrated_vrx_value;
    int16_t mapped_vry_value = calibrated_vry_value;

    // Limita os valores mapeados
    if (mapped_vrx_value < 0) 
    {
      mapped_vrx_value = -mapped_vrx_value;
    }
    if (mapped_vry_value < 0)
    {
      mapped_vry_value = -mapped_vry_value;
    }
    if (mapped_vrx_value < CALIBRATION_OFFSET && mapped_vrx_value > 0)
    {
      mapped_vrx_value = 0;
    }
    if (mapped_vry_value > -CALIBRATION_OFFSET && mapped_vry_value < 0)
    {
      mapped_vry_value = 0;
    }

    // Se o LED com PWM estiver ativado, ajusta o duty cycle dos LEDs vermelho e azul
    if(pwm_led_state)
    {
      pwm_set_gpio_level(LED_PIN_RED, mapped_vrx_value * 2); 
      pwm_set_gpio_level(LED_PIN_BLUE, mapped_vry_value * 2);
    }
    // Se não, desliga os LEDs	
    else
    {
      pwm_set_gpio_level(LED_PIN_RED, 0); 
      pwm_set_gpio_level(LED_PIN_BLUE, 0);
    }
    
    // Atualiza a borda do display de acordo com o estilo selecionado
    switch (border_style)
    {
      case 0:
        ssd1306_rect(&ssd, 3, 3, 122, 60, 1, 0);
        break;
      case 1:
        ssd1306_rect(&ssd, 3, 3, 122, 60, 1, 0);
        ssd1306_rect(&ssd, 4, 4, 120, 58, 1, 0);
        break; 
      case 2:
        ssd1306_rect(&ssd, 3, 3, 122, 60, 1, 0);
        ssd1306_rect(&ssd, 4, 4, 120, 58, 1, 0);
        ssd1306_rect(&ssd, 5, 5, 118, 56, 1, 0);
        break;
      case 3:
        ssd1306_rect(&ssd, 3, 3, 122, 60, 0, 0);
        ssd1306_rect(&ssd, 4, 4, 120, 58, 0, 0);
        ssd1306_rect(&ssd, 5, 5, 118, 56, 0, 0);
        break;
      default:
        break;
      }
    ssd1306_send_data(&ssd);
    // Define a posição do quadrado de acordo com os valores do joystick
    square_pos_x = (WIDTH - SQUARE_SIZE) / 2 + (calibrated_vrx_value * (WIDTH - SQUARE_SIZE) / 4096);
    square_pos_y = (HEIGHT - SQUARE_SIZE) / 2 + (-calibrated_vry_value * (HEIGHT - SQUARE_SIZE) / 4096);  

    // Apaga o quadrado na posição anterior e desenha o quadrado na nova posição
    ssd1306_rect(&ssd, prev_square_pos_y, prev_square_pos_x, SQUARE_SIZE, SQUARE_SIZE, 0, 1);
    ssd1306_rect(&ssd, square_pos_y, square_pos_x, SQUARE_SIZE, SQUARE_SIZE, 1, 1);
    ssd1306_send_data(&ssd);

    // Atualiza a posição anterior do quadrado para a posição atual
    prev_square_pos_x = square_pos_x;
    prev_square_pos_y = square_pos_y;

    sleep_ms(50);
  }

  return 0;  
}
