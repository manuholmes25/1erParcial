#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h> // Para usar la función atoi
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "inc/hw_memmap.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "inc/hw_ints.h"

char data[100];

uint32_t FS = 120000000 * 1;  
int cont = 0;
int cont2 = 0;
float distancia = 0.0;

// Prototipo de la función del manejador de interrupciones del temporizador
void Timer0IntHandler(void);

// Función para enviar un string por UART0
void UARTSendString(const char *str) {
    while (*str) {
        UARTCharPut(UART0_BASE, *str++);
    }
}

int main(void)
{
    // Configuración del reloj y perifericos
    SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);
    
    // Habilitar periféricos necesarios
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    
    // Esperar a que los periféricos estén listos
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC));
    
    // Configuración de pines GPIO
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_7);    // Configurar salida para PC7
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);    // Configurar salida para PN1
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    // Configuración de UART
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    
    // Configurar el reloj UART con PIOSC (16 MHz)
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTConfigSetExpClk(UART0_BASE, 16000000, 9600,
        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

    // Configuración del puerto UART para lectura de datos
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Configuración del temporizador (Timer0)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
    
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);  // Configuración de temporizador periódico
    TimerLoadSet(TIMER0_BASE, TIMER_A, FS);  // Inicializar temporizador con frecuencia base
    IntEnable(INT_TIMER0A);  // Habilitar interrupciones del temporizador
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);  // Habilitar la interrupción de tiempo de espera del temporizador
    IntMasterEnable();  // Habilitar interrupciones globales

    // Bucle principal
    while (1)
    {
        if (UARTCharsAvail(UART0_BASE)) {
            memset(data, 0, sizeof(data));  // Limpiar el buffer de datos
            uint32_t i = 0;
            char c;

            // Leer los caracteres recibidos por UART
            while (UARTCharsAvail(UART0_BASE)) {
                c = UARTCharGet(UART0_BASE);
                if (c == '\n' || i >= sizeof(data) - 1)
                    break;
                data[i++] = c;
            }

            data[i] = '\0';  // Terminar la cadena con '\0'

            // Enviar de vuelta el dato recibido por UART
            UARTSendString(data);  
            UARTSendString("\n");

            if (strcmp(data, "A") == 0) {
                GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
            }
            if (strcmp(data, "B") == 0) {
                cont = 0;
                TimerEnable(TIMER0_BASE, TIMER_A);  // Habilitar el temporizador
                UARTSendString("Comando B recibido, temporizador activado\n");  // Depuración
            }else{
              GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
            }
        }
    }
}

// Manejador de interrupciones del temporizador
void Timer0IntHandler(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);  // Limpiar la interrupción

    cont++;  // Incrementar el contador
    if (cont < 5) {
        // Encender las salidas para simular actividad
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);  
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
    }
    
    if (cont == 5) {
        // Cambiar la frecuencia del temporizador a 0.5 segundos
        FS = 120000000 * 0.5;  
        TimerLoadSet(TIMER0_BASE, TIMER_A, FS);
        UARTSendString("Frecuencia del temporizador ajustada a 0.5 segundos\n");  // Depuración
    }

    if (cont > 5) {
        if (cont % 2 == 0) {
            // Apagar las salidas en ciclos
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);  
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);
        } else {
            GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x00);  // Apagar PN1
            GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0x00);  // Apagar PC7
        }
    }

    if (cont > 15) {  // Después de 10 ciclos (10 segundos)
        // Detener el temporizador después de 10 segundos
        TimerDisable(TIMER0_BASE, TIMER_A);  
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x00);  // Apagar PN1
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0x00);  // Apagar PC7
        cont = 0;  // Resetear el contador
        UARTSendString("Temporizador detenido después de 10 segundos\n");  // Depuración
    }
}
