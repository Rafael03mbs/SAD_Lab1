/* 
 * File:   main.c
 * Author: josea
 *
 * Created on April 1, 2025, 12:05 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

#include "p24fj1024gb610.h"

//#define Fos 8000000
//#define PreScalar 256
//#define IPerS Fos / 2 / PreScalar
//#define FCY 4000000
//
//// Configuration Bits TIMER1
//#pragma config FNOSC = PRI
//#pragma config POSCMOD = HS
//#pragma config JTAGEN = OFF
//#pragma config FWDTEN = OFF
//
//void setupTimer1()
//{    
//    TMR1 = 0;
//    PR1 = IPerS;
//    IPC0bits.T1IP = 0x7;
//    IFS0bits.T1IF = 0;
//    IEC0bits.T1IE = 1;
//    // 0bx1000 0000 0011 0000
//    T1CON = 0x8030;
//}
//
//void __attribute__ ( ( interrupt, __shadow__ ) ) _T1Interrupt(void)
//{
//    IFS0bits.T1IF = 0;
//    TMR1 = 0;
//} 

void UART1_Init(void) {
    U1BRG = 25;
    
    // 0x8000 liga a UART e define 8 bits de dados, Sem paridade, 1 Stop bit
    U1MODE = 0x0000;
    U1MODEbits.UARTEN = 1;
    
    // 0x0400 liga apenas o Transmissor (UTXEN = 1)
    U1STA = 0x0000; 
    U1STAbits.UTXEN = 1;
}

void UART1_Write(char c) {
    if (U1STAbits.UTXBF == 0){ // Espera que o buffer de transmissão fique vazio
    U1TXREG = c;}             // Envia o caratere
}

void UART1_WriteString(const char *str) {
    while (*str != '\0') {
        UART1_Write(*str++);
    }
}

int main(int argc, char** argv) {
  
    ANSD = 1;              // (Apenas para reter compatibilidade com template)
    TRISA = 0x0000;        // Configura todos os pinos do PORTA como SAÍDA (LEDs D3 a D10 estão nos RA0-RA7)
    TRISDbits.TRISD6 = 1;  // Configura RD6 (Botão S3) como ENTRADA
    
    // Iniciar o estado: Apagar todos os LEDs e ligar apenas o LED 7 (RA7)
    int led_atual = 7;
    PORTA = 0x0000;      // Assegura que começam recetados
    PORTAbits.RA7 = 1;   // Liga apenas o LED no RA7
    
    int last_button_state = 1; // S3 tem resistência pull-up. 1 = Não pressionado.
    
    __builtin_write_OSCCONL(OSCCON & 0xbf);

    RPOR8bits.RP17R = 0x0003;
    
    RPINR18bits.U1RXR = 0x000A;
    
    UART1_Init();
    UART1_Write("1");
    
   /* while(1){
        
        int current_button_state = PORTDbits.RD6;
        // Detetar a "Edge" ou clique no botão (passagem de High/1 para Low/0)
        if (last_button_state == 1 && current_button_state == 0) {
            
            //UART1_Write("1");
            U1TXREG = '1';
            // "Debounce" básico. Atraso p/ ignorar falsos cliques causados pelo contacto de metal
            for(volatile long delay = 0; delay < 20000; delay++);
            
            // Confirmar que o botão ainda está "low"
            if (PORTDbits.RD6 == 0) {
                
                // Mudar para o próximo LED, descendo do 7 para o 0
                led_atual--;
                
                // Se chegou abaixo de 0, faz loop e regressa ao LED 7
                if (led_atual < 0) {
                    led_atual = 7;
                }
                
                // Atualizar o Output diretamente nos pinos da Placa
                // Primeiro desliga todos para evitar que dois fiquem ligados
                PORTA = 0x0000; 
                
                // Liga o pino diretamente através do seu nome
                switch(led_atual) {
                    case 7: PORTAbits.RA7 = 1; break;
                    case 6: PORTAbits.RA6 = 1; break;
                    case 5: PORTAbits.RA5 = 1; break;
                    case 4: PORTAbits.RA4 = 1; break;
                    case 3: PORTAbits.RA3 = 1; break;
                    case 2: PORTAbits.RA2 = 1; break;
                    case 1: PORTAbits.RA1 = 1; break;
                    case 0: PORTAbits.RA0 = 1; break;
                }
                
                // Esperar que o utilizador SE SOLTE do botão antes de contar como próximo clique
                // Isto evita transições descontroladas caso o dedo fique no botão
                while (PORTDbits.RD6 == 0) {
                    // Espera ativa (bloqueia aqui até largares S3)
                }
                
                // Evita bouncing (ruído) ao libertar o botão
                for(volatile long delay = 0; delay < 20000; delay++);
            }
        }
        
        // Atualiza a memória de last_state para o próximo ciclo
        last_button_state = PORTDbits.RD6;
    }
    */
    return (1);
}