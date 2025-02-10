# Projeto: Display SSD1306, Botões A/B e LEDs WS2812

Este projeto demonstra a integração entre:
- Um display OLED SSD1306, conectado via I2C
- Dois botões de entrada (Botão A e Botão B)
- LEDs RGB (Verde e Azul)
- Uma matriz de LEDs WS2812 de 5x5, conectada ao GPIO 7
- Comunicação serial via USB para receber caracteres e exibi-los no display

## Funcionalidades Principais
1. **Exibir caracteres no Display:**
   - Ao digitar um caractere no Serial Monitor do VSCode (ou outra interface serial), o caractere recebido é exibido no display SSD1306.
   - Se o caractere for um dígito entre '0' e '9', esse número também é exibido na matriz 5x5 de LEDs WS2812.

2. **Botão A (GPIO 5) controlando LED Verde (GPIO 11):**
   - Pressionar o Botão A alterna o estado do LED verde (ligado/desligado).
   - A operação é registrada de duas formas:
     - Mensagem no Serial Monitor.
     - Mensagem no display SSD1306 exibindo "LED Verde: Ligado" ou "LED Verde: Desligado".

3. **Botão B (GPIO 6) controlando LED Azul (GPIO 13):**
   - Pressionar o Botão B alterna o estado do LED azul (ligado/desligado).
   - A operação também é registrada de duas formas:
     - Mensagem no Serial Monitor.
     - Mensagem no display SSD1306 exibindo "LED Azul: Ligado" ou "LED Azul: Desligado".

4. **Matriz WS2812 (GPIO 7):**
   - Os dígitos de 0 a 9 são exibidos em um formato 5x5 na matriz de LEDs.

## Organização do Código
- **DisplayC.c:** Arquivo principal com a função `main()` e o loop principal, configurações de botões, LEDs, inicialização do display, leitura da porta serial e envio de caracteres para o display. Também contém a interrupção de botões (callback) que, quando um dígito é recebido, atualiza a matriz WS2812.
- **ssd1306.c / ssd1306.h:** Biblioteca para controlar o display SSD1306 via I2C (funções para desenhar pixels, strings, retângulos, etc.).
- **font.h:** Define as fontes para caracteres (maiúsculos, minúsculos, dígitos) a serem desenhados no display.
- **ws2812.pio / ws2812_program:** Programa PIO para controlar os LEDs WS2812, permitindo envio de dados no formato GRB em alta velocidade.

## Como Clonar o Projeto
1. Abra o terminal e navegue até a pasta onde deseja armazenar o projeto.
2. Clone o repositório:
   ```bash
   git clone https://github.com/usuario/I2C_DisplayEmC.git
   cd I2C_DisplayEmC
   ```
3. Abra o Visual Studio Code na pasta do projeto:
   ```bash
   code .
   ```

## Como Executar o Projeto usando a Extensão Raspberry Pi Pico Project no VSCode
1. **Instale a Extensão:** Se ainda não o fez, procure por "Raspberry Pi Pico Project" na aba de extensões do VSCode e instale.
2. **Configure o SDK e Toolchain:** Certifique-se de que o [Pico SDK](https://github.com/raspberrypi/pico-sdk) está instalado e que a extensão do Pico esteja configurada corretamente.
3. **Abra o Projeto:** Com o VSCode aberto na pasta do projeto, a extensão deve detectar o `CMakeLists.txt` e criar automaticamente a estrutura de build.
4. **Selecione o Projeto:** Na barra inferior do VSCode, escolha o kit de compilação e selecione o projeto Pico (Release ou Debug).
5. **Compile/Build:** Use o menu "Pico" ou o atalho para "Build". O VSCode gerará o arquivo `.uf2` na pasta de build.
6. **Gravar no Pico:** Conecte seu Raspberry Pi Pico em modo bootsel (segure BOOTSEL e plugue o cabo USB). O dispositivo aparecerá como uma unidade de disco. Arraste o arquivo `.uf2` gerado para essa unidade.
7. **Teste:** Abra o Serial Monitor do VSCode (Shift+Ctrl+P > Pico: Open Serial Monitor) ou outra interface serial para enviar caracteres e observar as mensagens no display SSD1306 e na matriz WS2812.

## Demonstração no YouTube

Confira a demonstração completa deste projeto no YouTube: [Demonstração do Projeto]( https://youtu.be/BJ7J5YY7K6A?si=E8VwPDCK6p93ivvv )

## Conexões de Hardware
- **Display SSD1306:**
  - SDA -> GPIO 14
  - SCL -> GPIO 15
  - Alimentação (3V3, GND)
- **Matriz WS2812 (5x5):**
  - Pino de dados (DIN) -> GPIO 7
  - Fonte de 3V3 ou 5V (conforme necessidade dos LEDs), GND
- **Botão A:**
  - Ligado ao GPIO 5, com pull-up interno
- **Botão B:**
  - Ligado ao GPIO 6, com pull-up interno
- **LED Verde:**
  - Ligado ao GPIO 11, com resistor adequado
- **LED Azul:**
  - Ligado ao GPIO 13, com resistor adequado

## Observações Finais
- Caso os dígitos apareçam invertidos na matriz WS2812, revise a função `update_matrix_display` para corrigir rotação e/ou espelhamento para números específicos.
- Como as funções I2C não devem ser chamadas na interrupção, este código utiliza flags globais para sinalizar a atualização do display.
- Ajuste o valor de `DEBOUNCE_DELAY_MS` se múltiplos disparos forem detectados ao pressionar os botões.

---
**Autor:** Iago Virgílio

**Licença:** [MIT License](LICENSE)
