#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "ws2812.pio.h" // Biblioteca para controle dos LEDs WS2812

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO_OLED 0x3C

#define WS2812_PIN 7
#define NUM_PIXELS 25

#define GREEN_LED_PIN 11  // Definição do pino do LED verde
#define BUTTON_A_PIN 5    // Definição do pino do botão A

#define DEBOUNCE_DELAY_MS 200  // Tempo de debounce para o botão A

// Estado do LED verde e flag para atualizar o display
volatile bool green_led_state = false;
uint32_t last_debounce_A = 0;
volatile bool update_display_flag = false;

// Objeto global para o display
ssd1306_t ssd = { .width = 128, .height = 64, .pages = 8, .address = ENDERECO_OLED, .i2c_port = I2C_PORT };

// Função de callback da interrupção (apenas sinaliza que a ação ocorreu)
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (gpio == BUTTON_A_PIN) {
        bool level = gpio_get(BUTTON_A_PIN); // true = solto, false = pressionado
        if (!level && (now - last_debounce_A >= DEBOUNCE_DELAY_MS)) {
            last_debounce_A = now;
            green_led_state = !green_led_state;  // Alterna o estado do LED
            gpio_put(GREEN_LED_PIN, green_led_state);

            // Mensagem para o Serial Monitor
            printf("Botão A pressionado. LED Verde: %s\n", green_led_state ? "Ligado" : "Desligado");

            // Apenas sinaliza para atualizar o display (não chama funções I2C aqui)
            update_display_flag = true;
        }
    }
}

// --- A parte do WS2812 permanece inalterada ---

// Definição da matriz de números 5x5 para WS2812
static const bool digit_patterns[10][NUM_PIXELS] = {
    // 0
    { 0,1,1,1,0,  1,0,0,0,1,  1,0,0,0,1,  1,0,0,0,1,  0,1,1,1,0 },
    // 1
    { 0,0,1,0,0,  0,1,1,0,0,  0,0,1,0,0,  0,0,1,0,0,  0,1,1,1,0 },
    // 2
    { 0,1,1,1,0,  1,0,0,0,1,  0,0,0,1,0,  0,0,1,0,0,  1,1,1,1,1 },
    // 3
    { 1,1,1,1,0,  0,0,0,0,1,  0,1,1,1,0,  0,0,0,0,1,  1,1,1,1,0 },
    // 4
    { 0,0,0,1,0,  0,0,1,1,0,  0,1,0,1,0,  1,1,1,1,1,  0,0,0,1,0 },
    // 5
    { 1,1,1,1,1,  1,0,0,0,0,  1,1,1,1,0,  0,0,0,0,1,  1,1,1,1,0 },
    // 6
    { 0,1,1,1,0,  1,0,0,0,0,  1,1,1,1,0,  1,0,0,0,1,  0,1,1,1,0 },
    // 7
    { 1,1,1,1,1,  0,0,0,1,0,  0,0,1,0,0,  0,1,0,0,0,  0,1,0,0,0 },
    // 8
    { 0,1,1,1,0,  1,0,0,0,1,  0,1,1,1,0,  1,0,0,0,1,  0,1,1,1,0 },
    // 9
    { 0,1,1,1,0,  1,0,0,0,1,  0,1,1,1,1,  0,0,0,0,1,  0,1,1,1,0 }
};

static PIO ws2812_pio = pio0;
static uint ws2812_sm = 0;
static uint ws2812_offset;

// Inicializa os LEDs WS2812
void ws2812_init(uint gpio) {
    ws2812_offset = pio_add_program(ws2812_pio, &ws2812_program);
    ws2812_program_init(ws2812_pio, ws2812_sm, ws2812_offset, gpio, 800000, false);
}

// Envia um pixel para a matriz WS2812
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(ws2812_pio, ws2812_sm, pixel_grb << 8u);
}

// Converte componentes R, G, B em um valor de 32 bits no formato GRB
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
}

// Atualiza a matriz WS2812 para exibir um número de 0 a 9
void update_matrix_display(int digit) {
    if (digit < 0 || digit > 9) return;

    uint32_t color = urgb_u32(0, 0, 20); // Define a cor azul para os LEDs ativos

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

int main() {
    stdio_init_all(); // Inicializa a UART para comunicação com o PC

    // Configuração do LED Verde (pino 12)
    gpio_init(GREEN_LED_PIN);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_put(GREEN_LED_PIN, false);  // Inicia desligado

    // Configuração do Botão A
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, gpio_callback);

    // Inicializa o I2C para comunicação com o display OLED
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
        // Atualiza o display OLED se a flag tiver sido setada pela ISR
        if (update_display_flag) {
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "LED Verde:", 20, 10);
            ssd1306_draw_string(&ssd, green_led_state ? "Ligado" : "Desligado", 20, 30);
            ssd1306_send_data(&ssd);
            update_display_flag = false;
        }
    
        // Processa a entrada serial com timeout de 10ms
        int ch = getchar_timeout_us(10000);
        if (ch != PICO_ERROR_TIMEOUT) {
            char c = (char) ch;
            // Atualiza o display OLED com o caractere recebido
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
