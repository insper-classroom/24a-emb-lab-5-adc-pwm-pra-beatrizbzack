/*
 * LED blink with FreeRTOS
*/
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

// Implementação do filtro de média móvel
#define WINDOW_SIZE 5

// Estrutura para a média móvel
typedef struct {
    int window[WINDOW_SIZE];
    int window_index;
} MovingAverage;

// Função para inicializar a estrutura da média móvel
void init_moving_average(MovingAverage *ma) {
    for (int i = 0; i < WINDOW_SIZE; i++) {
        ma->window[i] = 0;
    }
    ma->window_index = 0;
}

// Função para calcular a média móvel
int moving_average(MovingAverage *ma, int new_value) {
    // Adiciona o novo valor ao vetor de janela
    ma->window[ma->window_index] = new_value;

    // Atualiza o índice da janela circularmente
    ma->window_index = (ma->window_index + 1) % WINDOW_SIZE;

    // Calcula a média dos valores na janela
    int sum = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        sum += ma->window[i];
    }
    return sum / WINDOW_SIZE;
}

int scaled_value(int raw_value) {
    int scaled_value;
    raw_value -= 2048; // Center the value
    raw_value /= 4; // Scale the value

    if (raw_value < 30 || raw_value > -30) {
        return scaled_value = 0; // Dead zone
    } else {
        return scaled_value = raw_value;
    }
}

// FUNÇÕES PARA A LEITURA DA PORTA ANALÓGICA DE X E Y 
void x_task() {

    adc_t data;
    MovingAverage ma;
    init_moving_average(&ma); 

    adc_init();
    adc_gpio_init(26); // pino do x
    adc_select_input(0);

    printf("x task executando"); 

    while (1) {
        data.axis = 0; // seta que a axis é X 
        u_int16_t val_x = adc_read(); 
        data.val = scaled_value(moving_average(&ma,val_x)); // filtra a média móvel da entrada analógia e converte em valor binário
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        printf("valor de x: %lf", data.val); 
        sleep_ms(100);
    }
}

void y_task() {
    adc_init();
    adc_gpio_init(27); // pino do y
    adc_select_input(1);

    adc_t data;
    MovingAverage ma;
    init_moving_average(&ma);

    printf("y task executando"); 

    while (1) {
        data.axis = 1; // seta que a axis é X 
        u_int16_t val_y = adc_read();
        data.val = scaled_value(moving_average(&ma, val_y)); // filtra a média móvel da entrada analógia e converte em valor binário
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        printf("valor de y: %lf", data.val); 
        sleep_ms(100);
        
    }
}

void uart_task(void *p) {
    adc_t data;
    // printf("iniciando task uart"); 

    if (uxQueueMessagesWaiting(xQueueAdc) > 0) {
        if (xQueueReceive(xQueueAdc, &data, portMAX_DELAY) == pdTRUE) {
            // uart_write_blocking(uart0, data.axis, 1); // envia o valor do eixo
            // uart_write_blocking(uart0, data.val >> 8, 1); // desloca para pegar o byte mais significativo
            // uart_write_blocking(uart0, data.val, 1); 
            // uart_write_blocking(uart0, 0xFF, 1); // -1 => EOP fim do pacote
            printf(data.axis);
            printf(data.val>>8);
            printf(data.val);
            printf(0xFF); 
        }
        
    }
}


int main() {

    stdio_init_all();

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    xTaskCreate(x_task, "x_task", 4096, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
