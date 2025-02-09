#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "ws2812.pio.h" // Biblioteca para controle dos LEDs WS2812

// Definições de interfaces e endereços
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO_OLED 0x3C

// Definições para WS2812
#define WS2812_PIN 7
#define NUM_PIXELS 25

// Definições para os LEDs RGB e botões
#define GREEN_LED_PIN 11   // LED Verde (controlado pelo Botão A)
#define BLUE_LED_PIN 12    // LED Azul (controlado pelo Botão B)
#define BUTTON_A_PIN 5     // Botão A
#define BUTTON_B_PIN 6     // Botão B

// Tempo de debounce para os botões (em ms)
#define DEBOUNCE_DELAY_MS 200

// Variáveis globais para o estado dos LEDs
volatile bool green_led_state = false;
volatile bool blue_led_state = false;
uint32_t last_debounce_A = 0;
uint32_t last_debounce_B = 0;

// Flag global para atualizar o display OLED
volatile bool update_display_flag = false;

// Objeto global para o display OLED
ssd1306_t ssd = { 
    .width = 128, 
    .height = 64, 
    .pages = 8, 
    .address = ENDERECO_OLED, 
    .i2c_port = I2C_PORT 
};

// --------------------
// Funções para WS2812
// --------------------

// Padrões para os dígitos (matriz 5x5)
static const bool digit_patterns[10][NUM_PIXELS] = {
    { 0,1,1,1,0,  1,0,0,0,1,  1,0,0,0,1,  1,0,0,0,1,  0,1,1,1,0 }, // 0
    { 0,0,1,0,0,  0,1,1,0,0,  0,0,1,0,0,  0,0,1,0,0,  0,1,1,1,0 }, // 1
    { 0,1,1,1,0,  1,0,0,0,1,  0,0,0,1,0,  0,0,1,0,0,  1,1,1,1,1 }, // 2
    { 1,1,1,1,0,  0,0,0,0,1,  0,1,1,1,0,  0,0,0,0,1,  1,1,1,1,0 }, // 3
    { 0,0,0,1,0,  0,0,1,1,0,  0,1,0,1,0,  1,1,1,1,1,  0,0,0,1,0 }, // 4
    { 1,1,1,1,1,  1,0,0,0,0,  1,1,1,1,0,  0,0,0,0,1,  1,1,1,1,0 }, // 5
    { 0,1,1,1,0,  1,0,0,0,0,  1,1,1,1,0,  1,0,0,0,1,  0,1,1,1,0 }, // 6
    { 1,1,1,1,1,  0,0,0,1,0,  0,0,1,0,0,  0,1,0,0,0,  0,0,0,0,1 }, // 7
    { 0,1,1,1,0,  1,0,0,0,1,  0,1,1,1,0,  1,0,0,0,1,  0,1,1,1,0 }, // 8
    { 0,1,1,1,0,  1,0,0,0,1,  0,1,1,1,1,  0,0,0,0,1,  0,1,1,1,0 }  // 9
};

static PIO ws2812_pio = pio0;
static uint ws2812_sm = 0;
static uint ws2812_offset;

// Inicializa a matriz WS2812
void ws2812_init(uint gpio) {
    ws2812_offset = pio_add_program(ws2812_pio, &ws2812_program);
    ws2812_program_init(ws2812_pio, ws2812_sm, ws2812_offset, gpio, 800000, false);
}

// Envia um pixel para a matriz WS2812
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(ws2812_pio, ws2812_sm, pixel_grb << 8u);
}

// Converte componentes R, G, B para o formato GRB
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}

// Atualiza a matriz WS2812 para exibir um número de 0 a 9
void update_matrix_display(int digit) {
    if (digit < 0 || digit > 9) return;

    uint32_t color = urgb_u32(0, 0, 20); // Cor azul para os LEDs

    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 5; col++) {
            int index = (4 - row) * 5 + col;  // Inversão vertical

            // Corrige a inversão horizontal para os números 2 e 4
            if (digit == 2 || digit == 4) {
                index = (4 - row) * 5 + (4 - col);
            }

            if (digit_patterns[digit][index])
                put_pixel(color);
            else
                put_pixel(0);
        }
    }
}

// --------------------
// Função de callback para botões (ISR)
// --------------------
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    // Botão A: alterna o LED Verde
    if (gpio == BUTTON_A_PIN) {
        bool level = gpio_get(BUTTON_A_PIN); // true = solto, false = pressionado
        if (!level && (now - last_debounce_A >= DEBOUNCE_DELAY_MS)) {
            last_debounce_A = now;
            green_led_state = !green_led_state;
            gpio_put(GREEN_LED_PIN, green_led_state);
            printf("Botão A pressionado. LED Verde: %s\n", green_led_state ? "Ligado" : "Desligado");
            update_display_flag = true;
        }
    }
    // Botão B: alterna o LED Azul
    else if (gpio == BUTTON_B_PIN) {
        bool level = gpio_get(BUTTON_B_PIN); // true = solto, false = pressionado
        if (!level && (now - last_debounce_B >= DEBOUNCE_DELAY_MS)) {
            last_debounce_B = now;
            blue_led_state = !blue_led_state;
            gpio_put(BLUE_LED_PIN, blue_led_state);
            printf("Botão B pressionado. LED Azul: %s\n", blue_led_state ? "Ligado" : "Desligado");
            update_display_flag = true;
        }
    }
}

// --------------------
// Função principal
// --------------------
int main() {
    stdio_init_all(); // Inicializa a UART para comunicação

    // Inicializa o LED Verde (pino 11)
    gpio_init(GREEN_LED_PIN);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_put(GREEN_LED_PIN, false);

    // Inicializa o LED Azul (pino 13)
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    gpio_put(BLUE_LED_PIN, false);

    // Inicializa os botões A e B
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    // Registra a ISR para ambos os botões (a callback é registrada para o primeiro, 
    // e os outros são habilitados para a mesma callback)
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, gpio_callback);
    gpio_set_irq_enabled(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true);

    // Inicializa o I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_OLED, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializa a matriz de LEDs WS2812
    ws2812_init(WS2812_PIN);

    printf("Digite um caractere no Serial Monitor para exibir no display...\n");

    while (true) {
        // Se a flag de atualização do display estiver setada, atualiza ambos os estados:
        if (update_display_flag) {
            ssd1306_fill(&ssd, false);
            // Exibe mensagens para os dois LEDs
            ssd1306_draw_string(&ssd, "LED Verde:", 0, 0);
            ssd1306_draw_string(&ssd, green_led_state ? "Ligado" : "Desligado", 0, 10);
            ssd1306_draw_string(&ssd, "LED Azul:", 0, 30);
            ssd1306_draw_string(&ssd, blue_led_state ? "Ligado" : "Desligado", 0, 40);
            ssd1306_send_data(&ssd);
            update_display_flag = false;
        }
    
        // Processa a entrada serial com timeout de 10ms
        int ch = getchar_timeout_us(10000);
        if (ch != PICO_ERROR_TIMEOUT) {
            char c = (char) ch;
            ssd1306_fill(&ssd, false);
            ssd1306_draw_char(&ssd, c, 50, 25);
            ssd1306_send_data(&ssd);
            printf("Caractere recebido: %c\n", c);
    
            // Se for um número de 0 a 9, atualiza a matriz WS2812
            if (c >= '0' && c <= '9') {
                int digit = c - '0';
                update_matrix_display(digit);
            }
        }
        tight_loop_contents();
    }
    
    return 0;
}
