# Projeto de Controle de LEDs e Display com Joystick

## Descrição do Projeto

Este projeto utiliza um joystick para controlar a intensidade de LEDs RGB e a posição de um quadrado em um display SSD1306. O joystick fornece valores analógicos correspondentes aos eixos X e Y, que são utilizados para:

- Controlar a intensidade luminosa dos LEDs RGB via PWM.
- Representar a posição do joystick no display SSD1306 por meio de um quadrado móvel.

## Funcionalidades

- **Controle de LEDs RGB:**
  - O LED Azul tem seu brilho ajustado conforme o valor do eixo Y do joystick.
  - O LED Vermelho tem seu brilho ajustado conforme o valor do eixo X do joystick.
  - Esses dois LEDs são controlados via PWM para permitir variação suave da intensidade luminosa.
  - O LED Verde é ativado de acordo com o botão do joystick

- **Display SSD1306:**
  - Exibe um quadrado de 8x8 pixels, inicialmente centralizado, que se move proporcionalmente aos valores capturados pelo joystick.
  - Exibe diferentes modelos de borda, controlados por um botão
- **Botões:**
  - O botão do joystick alterna o estado do LED Verde e modifica a borda do display a cada acionamento.
  - O botão A ativa ou desativa os LEDs PWM a cada acionamento.
  - O botão B ativa o BOOTSEL (Funcionalidade Adicional para facilitar o desenvolvimento).
  - O botão do joystick ativa ou desativa o LED Verde e modifica a borda do Display
## Componentes Utilizados

- LED RGB, com os pinos conectados às GPIOs (11, 12 e 13).
- Botão do Joystick conectado à GPIO 22.
- Joystick conectado aos GPIOs 26 e 27.
- Botão A conectado à GPIO 5.
- Botão B conectado à GPIO 6.
- Display SSD1306 conectado via I2C (GPIO 14 e GPIO 15).

## Requisitos do Projeto

1. **Uso de interrupções:** Todas as funcionalidades relacionadas aos botões são implementadas utilizando rotinas de interrupção (IRQ).
2. **Debouncing:** Implementação do tratamento do bouncing dos botões via software.
3. **Utilização do Display 128 x 64:** Demonstração do entendimento do princípio de funcionamento do display e utilização do protocolo I2C.
4. **Organização do código:** O código está bem estruturado e comentado para facilitar o entendimento.

## Como Executar

1. Clone o repositório para o seu ambiente de desenvolvimento.
2. Compile o código utilizando o SDK do Raspberry Pi Pico.
3. Carregue o binário gerado na placa Raspberry Pi Pico.
4. Conecte os componentes conforme descrito na seção de componentes utilizados.
5. Execute o código na placa e observe o comportamento dos LEDs e do display conforme você move o joystick e pressiona os botões.

## Estrutura do Código

### Função `main`

A função `main` é responsável por:

- Inicializar os componentes (ADC, GPIOs, I2C, PWM, e display SSD1306).
- Configurar os pinos dos botões e LEDs.
- Configurar as interrupções dos botões.
- Ler os valores do joystick e mapear esses valores para controlar a intensidade dos LEDs RGB via PWM.
- Atualizar a posição do quadrado no display SSD1306 com base nos valores do joystick.
- Ativar ou desativar os LEDs PWM.



### Função de Interrupção `gpio_irq_handler`

A função `gpio_irq_handler` é responsável por:

- Implementar o debouncing dos botões.
- Alternar o estado do LED Verde e modificar a borda do display quando o botão do joystick é pressionado.
- Ativar o BOOTSEL quando o botão B é pressionado.(Função extra)

## Vídeo de explicação 
