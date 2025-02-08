#include <stdlib.h>
#include <stdio.h> // Adicionado para leitura de caracteres via UART
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO_OLED 0x3C

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

    printf("Digite um caractere no Serial Monitor para exibir no display...\n");

    while (true) {
        char c = getchar(); // Aguarda entrada de um caractere via Serial Monitor

        // Atualiza o display com o novo caractere
        ssd1306_fill(&ssd, false);  // Limpa o display
        ssd1306_draw_char(&ssd, c, 50, 25); // Exibe o caractere na posição (50, 25)
        ssd1306_send_data(&ssd);

        printf("Caractere recebido: %c\n", c);
    }

    return 0;
}
