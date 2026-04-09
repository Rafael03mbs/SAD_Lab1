/* 
 * File:   main.c
 * Author: josea / Antigravity Model
 *
 * Implements Data Acquisition Device logic for PIC24FJ1024GB610
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xc.h>
#include "p24fj1024gb610.h"

// Fcy settings
#define Fos 8000000
#define PreScalar 256
#define IPerS (Fos / 2 / PreScalar)
#define FCY 4000000

#define MAX_SAMPLES 50
#define RX_BUF_SIZE 256

// ----- Global variables for configuration -----
int cfg_Ax = 0;  // AN5
int cfg_Ay = 0;  // AN4
int cfg_Az = 0;  // AN0
int cfg_D6 = 0;  // RD6
int cfg_D7 = 0;  // RD7
int cfg_DB = 0;  // Bidirectional channel RA7
int cfg_DV = 0;  // Virtual channel
int cfg_b  = 0;  // RA7: 0=out, 1=in
int cfg_v  = 0;  // 1 = virtual channel configured
int cfg_n  = 1;  // Number of samples per message
int cfg_p  = 1;  // Sampling period (Seconds)
int cfg_alert_val = -1; // -1 means disabled by default, set via JSON {"alert_thresh":3}

// ----- UART RX Buffer -----
volatile char rx_buffer[RX_BUF_SIZE];
volatile int rx_index = 0;
volatile int message_received = 0;

// ----- Data Buffers -----
unsigned int buf_Ax[MAX_SAMPLES];
unsigned int buf_Ay[MAX_SAMPLES];
unsigned int buf_Az[MAX_SAMPLES];
int buf_D6[MAX_SAMPLES];
int buf_D7[MAX_SAMPLES];
int buf_DB[MAX_SAMPLES];
int buf_DV[MAX_SAMPLES];
volatile int sample_count = 0;

// ----- Timer and flags -----
volatile int time_tick = 0;
volatile int sample_flag = 0;
int alert_triggered = 0;


// Timer1 interrupt: Triggers every 100ms
void __attribute__ ((interrupt, no_auto_psv)) _T1Interrupt(void)
{
    IFS0bits.T1IF = 0;
    
    // Only increment when counting is enabled by period
    if (cfg_p > 0) {
        time_tick++;
        // 10 ticks of 100ms = 1 second. Therefore Period in seconds:
        if (time_tick >= (cfg_p * 10)) { 
            time_tick = 0;
            sample_flag = 1;
        }
    }
}

// UART1 RX interrupt
void __attribute__ ((interrupt, no_auto_psv)) _U1RXInterrupt(void)
{
    IFS0bits.U1RXIF = 0;
    while (U1STAbits.URXDA) {
        char c = U1RXREG;
        if (rx_index < RX_BUF_SIZE - 1) {
            rx_buffer[rx_index++] = c;
            
            // Detect JSON end mark or typical line endings
            if (c == '}' || c == '\n') {
                rx_buffer[rx_index] = '\0';
                message_received = 1;
            }
        } else {
            // Buffer overflow reset
            rx_index = 0;
        }
    }
}


void setupTimer1()
{    
    TMR1 = 0;
    PR1 = IPerS / 10; // 1562 for 100ms interrupts
    IPC0bits.T1IP = 0x5;
    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;
    T1CON = 0x8030; // TON=1, prescale=256
}

void UART1_Init(void) {
    U1BRG = 25; // 9600 bps with FCY=4MHz
    
    __builtin_write_OSCCONL(OSCCON & 0xbf);

    RPOR8bits.RP17R = 0x0003; // Reprogrammable pins for TX
    RPINR18bits.U1RXR = 0x000A; // RX pin
    
    U1MODE = 0x0000;
    U1MODEbits.UARTEN = 1;
    
    U1STA = 0x0000; 
    U1STAbits.UTXEN = 1;
    
    // Enable UART Interrupts
    IPC2bits.U1RXIP = 0x4; // RX Interrupt Priority
    IFS0bits.U1RXIF = 0;
    IEC0bits.U1RXIE = 1; // Enable Rx Interrupt
}

void UART1_Write(char c) {
    while(U1STAbits.UTXBF == 1); // Await for empty TX buffer
    U1TXREG = c;
}

void UART1_WriteString(const char *str) {
    while (*str != '\0') {
        UART1_Write(*str++);
    }
}

void ADC_Init() {
    // Config B0(AN0), B4(AN4), B5(AN5) as Analog
    TRISBbits.TRISB0 = 1; ANSBbits.ANSB0 = 1;       
    TRISBbits.TRISB4 = 1; ANSBbits.ANSB4 = 1;       
    TRISBbits.TRISB5 = 1; ANSBbits.ANSB5 = 1;       
    
    AD1CON1 = 0x0000;         
    AD1CON1bits.SSRC = 0b000; // Manual sample trigger
    
    AD1CON2 = 0x0000;         
    AD1CON3 = 0x0002; // Minimal Tad configurations     
    
    AD1CON1bits.ADON = 1;
}

unsigned int ADC_Read(int channel) {
    AD1CHS = channel;
    AD1CON1bits.SAMP = 1;          
    for(volatile int i=0; i<100; i++); // Charging capacitor time   
    AD1CON1bits.SAMP = 0; // Trigger conversion 
    while (!AD1CON1bits.DONE);      
    AD1CON1bits.DONE = 0;          
    return ADC1BUF0;               
}

// ----- Simple Ad-Hoc JSON Value Extractor -----
int get_json_val(const char *json, const char *key, int default_val) {
    char search_key[20];
    sprintf(search_key, "\"%s\":", key);
    char *pos = strstr(json, search_key);
    
    if (!pos) {
        sprintf(search_key, "'%s':", key); // Support single quotes occasionally sent by terminals
        pos = strstr(json, search_key);
    }
    
    if (pos) {
        pos += strlen(search_key);
        while(*pos == ' ' || *pos == '\t') pos++; // Trim empty whitespaces
        return atoi(pos); // Gets integer from string
    }
    return default_val;
}

void process_json_config(const char *json) {
    // Read actuation (Digital out)
    if (strstr(json, "\"D0\":") || strstr(json, "'D0':")) PORTAbits.RA0 = get_json_val(json, "D0", 0);
    if (strstr(json, "\"D1\":") || strstr(json, "'D1':")) PORTAbits.RA1 = get_json_val(json, "D1", 0);
    if (strstr(json, "\"D2\":") || strstr(json, "'D2':")) PORTAbits.RA2 = get_json_val(json, "D2", 0);
    // Generic actuate like the document says: (Dw could mean anything, we provide direct port assignments)
    
    // Read Configurations
    cfg_Ax = get_json_val(json, "Ax", cfg_Ax);
    cfg_Ay = get_json_val(json, "Ay", cfg_Ay);
    cfg_Az = get_json_val(json, "Az", cfg_Az);
    cfg_D6 = get_json_val(json, "D6", cfg_D6);
    cfg_D7 = get_json_val(json, "D7", cfg_D7);
    cfg_DB = get_json_val(json, "DB", cfg_DB);
    cfg_DV = get_json_val(json, "DV", cfg_DV);
    
    cfg_p = get_json_val(json, "p", cfg_p);
    cfg_n = get_json_val(json, "n", cfg_n);
    if (cfg_n > MAX_SAMPLES) cfg_n = MAX_SAMPLES;
    if (cfg_n < 1) cfg_n = 1;
    
    cfg_v = get_json_val(json, "v", cfg_v);
    cfg_alert_val = get_json_val(json, "alert_thresh", cfg_alert_val);

    // Apply Bidirectional pin mode dynamically
    if (strstr(json, "\"b\":") || strstr(json, "'b':")) {
        cfg_b = get_json_val(json, "b", cfg_b);
        TRISAbits.TRISA7 = cfg_b; // 1 = Entry, 0 = Out
    }
}

// Appends stringly Array to UART output
void print_json_array(const char* label, const unsigned int* arr, const int* arr_int, int is_int, int is_first) {
    char temp[32];
    if (!is_first) UART1_WriteString(", ");
    
    UART1_WriteString("\"");
    UART1_WriteString(label);
    UART1_WriteString("\":[");
    
    for (int i = 0; i < cfg_n; i++) {
        if (is_int) {
            sprintf(temp, "%d%s", arr_int[i], (i == cfg_n - 1) ? "" : ",");
        } else {
            sprintf(temp, "%u%s", arr[i], (i == cfg_n - 1) ? "" : ",");
        }
        UART1_WriteString(temp);
    }
    UART1_WriteString("]");
}

// Flushes sampled values array to Computer
void send_monitoring_message() {
    int first_key = 1;
    UART1_WriteString("{");
    
    if (cfg_Ax) { print_json_array("Ax", buf_Ax, NULL,  0, first_key); first_key=0; }
    if (cfg_Ay) { print_json_array("Ay", buf_Ay, NULL,  0, first_key); first_key=0; }
    if (cfg_Az) { print_json_array("Az", buf_Az, NULL,  0, first_key); first_key=0; }
    
    if (cfg_D6) { print_json_array("D6", NULL, buf_D6,  1, first_key); first_key=0; }
    if (cfg_D7) { print_json_array("D7", NULL, buf_D7,  1, first_key); first_key=0; }
    if (cfg_DB) { print_json_array("DB", NULL, buf_DB,  1, first_key); first_key=0; }
    if (cfg_DV && cfg_v) { print_json_array("DV", NULL, buf_DV, 1, first_key); first_key=0; }
    
    UART1_WriteString("}\r\n");
}


int main(int argc, char** argv) {
    // Hardware Setup
    ANSD = 1;               // Retro-compatibility with template where applicable
    TRISA = 0x0000;         // Conf All PORTA as OUTPUT (D3..D10 LEDs are here)
    PORTA = 0x0000;         // Clean signals
    
    TRISDbits.TRISD6 = 1;   // Conf RD6 as INPUT
    TRISDbits.TRISD7 = 1;   // Conf RD7 as INPUT for our logic needs
    
    TRISAbits.TRISA7 = 0;   // Bidirectional pin initialized as output (b=0 default)
    
    ADC_Init();
    UART1_Init();
    setupTimer1();
    
    // Greeting Message
    UART1_WriteString("---- SAD DAD System Started ----\r\n");
    
    while(1) {
        
        // 1. Check if there's a JSON msg waiting
        if (message_received) {
            
            // Critical Section for extracting config
            IEC0bits.U1RXIE = 0; // Disable briefly
            process_json_config((char*)rx_buffer);
            rx_index = 0;
            message_received = 0;
            IEC0bits.U1RXIE = 1; // Enable again
            
            UART1_WriteString("{\"status\":\"config_updated\"}\r\n");
            
            // Whenever n changes drastically we can reset sampling pointer to avoid garbage send loops:
            if (sample_count >= cfg_n) { sample_count = 0; }
        }
        
        // 2. Perform Polling Sampling if Timer flagged it
        if (sample_flag) {
            sample_flag = 0; // Consume flag
            
            // Read requested analogs
            if (cfg_Ax) buf_Ax[sample_count] = ADC_Read(5); // AN5
            if (cfg_Ay) buf_Ay[sample_count] = ADC_Read(4); // AN4
            if (cfg_Az) buf_Az[sample_count] = ADC_Read(0); // AN0
            
            // Read requested discrete digitals
            if (cfg_D6) buf_D6[sample_count] = PORTDbits.RD6;
            if (cfg_D7) buf_D7[sample_count] = PORTDbits.RD7;
            if (cfg_DB) buf_DB[sample_count] = PORTAbits.RA7;
            
            // Calculate virtual logic independently from sample config
            int v_val = 0;
            if (cfg_v) {
                v_val = (PORTDbits.RD7 << 1) | PORTDbits.RD6;
                // Add to array only if DV is required
                if (cfg_DV) buf_DV[sample_count] = v_val;
            }
            
            // Advance Array
            sample_count++;
            
            // Trigger Network dump once buffer filled
            if (sample_count >= cfg_n) {
                send_monitoring_message();
                sample_count = 0; // Reset for next batch
            }
            
            // 3. Independent Event Alert System for Virtual Channels
            // Alert logic evaluates continuously on every sample interval (to prevent racing on extreme polling values)
            if (cfg_v && cfg_alert_val != -1) {
                if (v_val == cfg_alert_val && !alert_triggered) {
                    char alert_msg[64];
                    sprintf(alert_msg, "{\"alert\":\"threshold_reached\", \"v_val\":%d}\r\n", v_val);
                    UART1_WriteString(alert_msg);
                    alert_triggered = 1;
                } else if (v_val != cfg_alert_val) {
                    alert_triggered = 0; // Recover alert state safely
                }
            }
        }
    }
    
    return (1);
}