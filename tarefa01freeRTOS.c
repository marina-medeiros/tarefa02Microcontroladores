#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "hardware/adc.h"
#include "joystick.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "include/wifi_conn.h"      // Funções personalizadas de conexão WiFi
#include "include/mqtt_comm.h"      // Funções personalizadas para MQTT
#include "include/xor_cipher.h"     // Funções de cifra XOR

const uint led_pin_red = 12;

void setup(){
    gpio_init(led_pin_red);
    gpio_set_dir(led_pin_red, GPIO_OUT);
    setup_joystick();
}

//LED
void vBlinkTask(){
    for (;;){
        gpio_put(led_pin_red, 1); // LED pin 12 
        vTaskDelay(50);
        gpio_put(led_pin_red, 0);
        vTaskDelay(950);
        //printf("Blinking\n");
    }
}

// SENSOR
void vTemperatureTask(){
    while (true){
        adc_set_temp_sensor_enabled(true);
        adc_select_input(4);
        // converte o valor para abranger 12 bits no máximo
        const float conversionFactor = 3.3f / (1 << 12);
        float adc = (float)adc_read() * conversionFactor;
        float tempC = 27.0f - (adc - 0.706f) / 0.001721f; // Calculo para a temperatura aproximada mostrado no datasheet do RP2040
        printf("Temperatura: %0.2f\n", tempC);
        vTaskDelay(2500);
    }
}

// JOYSTICK
void vJoystickTask(){
    int last_direction = 5;
    uint16_t vry_value, vrx_value;

    while (1){
        joystick_read_axis(&vry_value, &vrx_value);

        int direction = get_direction(vry_value, vrx_value);

        if(direction != last_direction){
            switch (direction) {
            case 4:
                printf("Direita\n");
                vTaskDelay(50);
                break;
            case 3:
                printf("Esquerda\n");
                vTaskDelay(50);
                break;
            case 2:
                printf("Cima\n");
                vTaskDelay(50);
                break;
            case 1:
                printf("Baixo\n");
                vTaskDelay(50);
                break;
            case 5:
                vTaskDelay(50);
                break;
            }
        }

        last_direction = direction;
    }
    
}

void wifiCommunication(){
    // Conecta à rede WiFi
    // Parâmetros: Nome da rede (SSID) e senha
    connect_to_wifi("SSID da rede", "Senha da rede");

    // Configura o cliente MQTT
    // Parâmetros: ID do cliente, IP do broker, usuário, senha
    mqtt_setup("bitdog1", "IP do broker", "aluno", "senha123");

    // Mensagem original a ser enviada
    const char *mensagem = "26.5";
    // Buffer para mensagem criptografada (16 bytes)
    uint8_t criptografada[16];
    // Criptografa a mensagem usando XOR com chave 42
    xor_encrypt((uint8_t *)mensagem, criptografada, strlen(mensagem), 42);

    // Loop principal do programa
    while (true) {
        // Publica a mensagem original (não criptografada)
        mqtt_comm_publish("escola/sala1/temperatura", mensagem, strlen(mensagem));
        
        // Alternativa: Publica a mensagem criptografada (atualmente comentada)
        // mqtt_comm_publish("escola/sala1/temperatura", criptografada, strlen(mensagem));
        
        // Aguarda 5 segundos antes da próxima publicação
        sleep_ms(5000);
    }
}

int main(){
    stdio_init_all();
    adc_init();
    setup();
    
    xTaskCreate(vBlinkTask, "Blink Task", 128, NULL, 1, NULL);
    xTaskCreate(vTemperatureTask, "Temperature Task", 256, NULL, 1, NULL);
    xTaskCreate(vJoystickTask, "Joystick Task", 256, NULL, 1, NULL);
    vTaskStartScheduler();
}
