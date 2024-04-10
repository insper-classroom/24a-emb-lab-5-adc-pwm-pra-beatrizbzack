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

const int pinoX = 26;
const int pinoXADC = 0;
const int pinoY = 27;
const int pinoYADC = 1;

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
    raw_value /= 8; // Scale the value

    if (raw_value < 80 && raw_value > -80) {
        return scaled_value = 0; // Dead zone
    } else {
        return scaled_value = raw_value;
    }
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis); 
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb); 
    uart_putc_raw(uart0, -1); 
}

void x_task(void *p) {
    adc_init();
    adc_gpio_init(pinoX); 

    MovingAverage ma;
    init_moving_average(&ma);

    while (1) {
        adc_select_input(pinoXADC);
        int x = moving_average(&ma, adc_read());
        x = scaled_value(x); // escala o valor
        struct adc x_data = {0,x};
        xQueueSend(xQueueAdc, &x_data, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    adc_init();
    adc_gpio_init(pinoY);

    MovingAverage ma;
    init_moving_average(&ma);

    while (1) {
        adc_select_input(pinoYADC);
        int y = moving_average(&ma, adc_read());
        y = scaled_value(y); // escala o valor
        struct adc y_data = {1,y};
        xQueueSend(xQueueAdc, &y_data, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}



void uart_task(void *p) {
    adc_t data;

    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);

        // data.val = (data.val-2047)/8;
        // int zone_limit = 80;
        // if (data.val <=zone_limit && data.val >= -1*(zone_limit)) {
        //     data.val = 0;
        // }
        // printf("Axis: %d, Value: %d\n", data.axis, data.val);
        write_package(data);
    }
}

int main() {
    stdio_init_all();
    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(x_task, "x_task", 256, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 256, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    vTaskStartScheduler();

    while (true);
}