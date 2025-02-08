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

// Função para enviar um pixel para a matriz WS2812
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
            int index = (4 - row) * 5 + col;  // Inversão apenas vertical (correto para maioria)

            // Corrige a inversão horizontal somente para os números 2 e 4
            if (digit == 2 || digit == 4) {
                index = (4 - row) * 5 + (4 - col);
            }

            if (digit_patterns[digit][index])
                put_pixel(color);  // LED aceso
            else
                put_pixel(0);      // LED apagado
        }
    }
}

int main() {
    stdio_init_all(); // Inicializa a UART para comunicação com o PC

    // Inicializa o I2C para comunicação com o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_OLED, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd);

    // Inicializa a matriz de LEDs WS2812
    ws2812_init(WS2812_PIN);

    printf("Digite um caractere no Serial Monitor para exibir no display...\n");

    while (true) {
        char c = getchar(); // Aguarda entrada de um caractere via Serial Monitor

        // Atualiza o display com o novo caractere
        ssd1306_fill(&ssd, false);  // Limpa o display
        ssd1306_draw_char(&ssd, c, 50, 25); // Exibe o caractere na posição (50, 25)
        ssd1306_send_data(&ssd);

        printf("Caractere recebido: %c\n", c);

        // Se for um número de 0 a 9, exibe na matriz WS2812
        if (c >= '0' && c <= '9') {
            int digit = c - '0'; // Converte de char para número inteiro
            update_matrix_display(digit);
        }
    }

    return 0;
}
