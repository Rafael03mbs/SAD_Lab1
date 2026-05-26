/*
 * File:   main.c
 * Author: Iuri / Rafael / Tomas
 *
 * Implements Data Acquisition Device logic for PIC24FJ1024GB610.
 * Includes HD44780 LCD control, JSON command parsing, party/credits modes,
 * temperature sensor, LDR on AN1, UART RX interrupt and I2C SD acquisition.
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

#include <libpic30.h>

#define MAX_SAMPLES 50
#define RX_BUF_SIZE 256
#define TIMER_TICKS_PER_SECOND 10

#define I2C_TIMEOUT 20000
#define SD_I2C_ADDR 0x48
#define SD_ACQUIRE_CMD 0xAC
#define SD_READY_STATUS 0x40
#define SD_READ_ERROR 0xFFFF
#define I2C_DIAG_ON_BOOT 0
#define I2C_BUS_RECOVERY_ON_INIT 1

// ----- LCD Pin Definitions -----
#define LCD_DATA  LATE
#define LCD_RS    LATBbits.LATB15
#define LCD_RW    LATDbits.LATD5
#define LCD_E     LATDbits.LATD4

// ----- Global variables for configuration -----
int cfg_Ax = 0;  // AN5 - Potentiometer
int cfg_Ay = 0;  // AN4 - Temperature sensor
int cfg_Az = 0;  // AN1 - LDR
int cfg_SD0 = 0; // Arduino A0 via I2C
int cfg_SD1 = 0; // Arduino A1 via I2C
int cfg_D6 = 0;  // RD6
int cfg_D7 = 0;  // RD7
int cfg_DB = 0;  // Bidirectional channel RA7
int cfg_DV = 0;  // Virtual channel
int cfg_b  = 0;  // RA7: 0=out, 1=in
int cfg_v  = 0;  // 1 = virtual channel configured
int cfg_n  = 1;  // Number of samples per message
int cfg_p  = TIMER_TICKS_PER_SECOND;  // Sampling period in Timer1 ticks
int cfg_alert_val = -1;

// ----- UART RX Buffer -----
volatile char rx_buffer[RX_BUF_SIZE];
volatile int rx_index = 0;
volatile int message_received = 0;

// ----- Data Buffers -----
unsigned int buf_Ax[MAX_SAMPLES];
int buf_Ay[MAX_SAMPLES];
unsigned int buf_Az[MAX_SAMPLES];
unsigned int buf_SD0[MAX_SAMPLES];
unsigned int buf_SD1[MAX_SAMPLES];
int buf_D6[MAX_SAMPLES];
int buf_D7[MAX_SAMPLES];
int buf_DB[MAX_SAMPLES];
int buf_DV[MAX_SAMPLES];
volatile int sample_count = 0;



// ----- Timer and flags -----
volatile int time_tick = 0;
volatile int sample_flag = 0;
int alert_triggered = 0;
int i2c2_ready = 0;

/// ----- I2C Digital Sensor -----
void I2C2_ClearErrors(void) {
    I2C2STATbits.I2COV = 0;
    I2C2STATbits.IWCOL = 0;
    I2C2STATbits.BCL = 0;
}

int I2C2_WaitIdle(void) {
    unsigned int timeout = I2C_TIMEOUT;
    while ((I2C2CONL & 0x001F) || I2C2STATbits.TRSTAT) {
        if (--timeout == 0) return 0;
    }
    return 1;
}

int I2C2_WaitFlagClear(volatile unsigned int *reg, unsigned int mask) {
    unsigned int timeout = I2C_TIMEOUT;
    while ((*reg & mask) != 0) {
        if (--timeout == 0) return 0;
    }
    return 1;
}

// Manual bus recovery: toggle SCL to free a stuck SDA line
void I2C2_BusRecovery(void) {
    I2C2CONLbits.I2CEN = 0;    // Disable I2C module
    TRISAbits.TRISA2 = 0;      // SCL as output
    TRISAbits.TRISA3 = 1;      // SDA as input
    int i;
    for (i = 0; i < 9; i++) {  // Clock up to 9 pulses
        LATAbits.LATA2 = 0;
        __delay_us(10);
        LATAbits.LATA2 = 1;
        __delay_us(10);
        if (PORTAbits.RA3 == 1) break;  // SDA released
    }
    TRISAbits.TRISA2 = 1;      // Return SCL to input (open-drain)
}

void I2C2_Init(void) {
    PMD3bits.I2C2MD = 0;       // Enable power to the I2C2 peripheral
    I2C2CONLbits.I2CEN = 0;    // Disable I2C module for configuration
    i2c2_ready = 0;

#if I2C_BUS_RECOVERY_ON_INIT
    // 9-pulse SCL bus recovery to release stuck SDA lines on reset/power-on
    I2C2_BusRecovery();
#endif

    // Configure SCL2 (RA2) and SDA2 (RA3) as digital inputs with open-drain enabled
    TRISAbits.TRISA2 = 1;
    TRISAbits.TRISA3 = 1;
    ODCAbits.ODCA2 = 1;
    ODCAbits.ODCA3 = 1;

    I2C2CONL = 0x0000;
    I2C2CONH = 0x0000;
    I2C2BRG = 39;              // 100 kHz speed with FCY = 4 MHz 
    I2C2_ClearErrors();
    I2C2STAT = 0x0000;         // Reset all status bits

    I2C2CONLbits.DISSLW = 1;   // Disable slew rate control (standard speed)
    I2C2CONLbits.I2CEN = 1;    // Enable module
    i2c2_ready = 1;
    __delay_ms(5);             // Let the bus stabilize
}

int I2C2_Start(void) {
    if (!I2C2_WaitIdle()) return 0;
    I2C2_ClearErrors();
    I2C2CONLbits.SEN = 1;
    return I2C2_WaitFlagClear((volatile unsigned int *)&I2C2CONL, 0x0001);
}

int I2C2_Stop(void) {
    unsigned int timeout = I2C_TIMEOUT;
    I2C2CONLbits.PEN = 1;
    while (I2C2CONLbits.PEN) {
        if (--timeout == 0) return 0;
    }
    return 1;
}

int I2C2_WriteByte(unsigned char value) {
    unsigned int timeout = I2C_TIMEOUT;
    if (!I2C2_WaitIdle()) return 0;

    I2C2TRN = value;
    while (I2C2STATbits.TBF) {
        if (--timeout == 0) return 0;
    }

    timeout = I2C_TIMEOUT;
    while (I2C2STATbits.TRSTAT) {
        if (--timeout == 0) return 0;
    }

    if (I2C2STATbits.IWCOL || I2C2STATbits.BCL || I2C2STATbits.ACKSTAT) {
        I2C2_ClearErrors();
        return 0;
    }

    return 1;
}

int I2C2_ReadByte(unsigned char *value, int send_nack) {
    unsigned int timeout = I2C_TIMEOUT;

    if (!I2C2_WaitIdle()) return 0;   // Wait for bus idle before enabling receive

    I2C2CONLbits.RCEN = 1;
    while (!I2C2STATbits.RBF) {
        if (--timeout == 0) return 0;
    }

    *value = I2C2RCV;

    if (!I2C2_WaitIdle()) return 0;   // Wait idle before sending ACK/NACK

    I2C2CONLbits.ACKDT = send_nack ? 1 : 0;
    I2C2CONLbits.ACKEN = 1;

    timeout = I2C_TIMEOUT;
    while (I2C2CONLbits.ACKEN) {
        if (--timeout == 0) return 0;
    }

    return 1;
}
int SD_ReadSensorsOnce(unsigned int *sd0, unsigned int *sd1) {
    unsigned char rx[5];

    if (!I2C2_Start()) goto fail;
    if (!I2C2_WriteByte((SD_I2C_ADDR << 1) | 0)) goto fail;
    if (!I2C2_WriteByte(SD_ACQUIRE_CMD)) goto fail;
    if (!I2C2_Stop()) goto fail;

    // Give the Arduino enough time to complete analogRead in its receiveEvent()
    // (analogRead ~110us x 2 channels + loop overhead + I2C ISR latency)
    __delay_ms(5);

    if (!I2C2_Start()) goto fail;
    if (!I2C2_WriteByte((SD_I2C_ADDR << 1) | 1)) goto fail;

    for (int i = 0; i < 5; i++) {
        if (!I2C2_ReadByte(&rx[i], i == 4)) goto fail;
    }

    if (!I2C2_Stop()) goto fail;
    if (rx[0] != SD_READY_STATUS) return 0;

    *sd0 = (((unsigned int)rx[1]) << 8) | rx[2];
    *sd1 = (((unsigned int)rx[3]) << 8) | rx[4];
    return 1;

fail:
    I2C2_ClearErrors();
    I2C2_Stop();
    return 0;
}

int SD_ReadSensors(unsigned int *sd0, unsigned int *sd1) {
    // Retry up to 3 times with bus recovery on persistent failure
    int attempt;

    if (!i2c2_ready) {
        I2C2_Init();
    }

    for (attempt = 0; attempt < 3; attempt++) {
        if (SD_ReadSensorsOnce(sd0, sd1)) return 1;
        // Full re-init between retries: bus recovery + module reset
        I2C2_Init();
        __delay_ms(5);
    }
    return 0;
}

int I2C2_AddressResponds(unsigned char addr) {
    int ack = 0;

    if (I2C2_Start()) {
        ack = I2C2_WriteByte((addr << 1) | 0);
    }

    I2C2_Stop();
    __delay_ms(1);
    return ack;
}

int I2C2_ScanBus(char *result, unsigned int result_size) {
    unsigned char addr;
    int found = 0;
    int truncated = 0;

    if (result_size == 0) return 0;
    result[0] = '\0';

    for (addr = 0x08; addr <= 0x77; addr++) {
        I2C2_ClearErrors();

        if (I2C2_AddressResponds(addr)) {
            char item[8];
            unsigned int used = strlen(result);

            sprintf(item, "%s0x%02X", found ? "," : "", addr);
            if ((used + strlen(item)) < result_size) {
                strcat(result, item);
            } else {
                truncated = 1;
            }
            found++;
        }
    }

    if (found == 0) {
        strncpy(result, "NONE", result_size - 1);
        result[result_size - 1] = '\0';
    } else if (truncated && (strlen(result) + 4) < result_size) {
        strcat(result, ",...");
    }

    return found;
}

// ----- LCD Functions -----
void LCD_Pulse(void) {
    LCD_E = 1;
    __delay_us(50);
    LCD_E = 0;
    __delay_us(50);
}

void LCD_Command(char cmd) {
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_DATA = (LCD_DATA & 0xFF00) | (unsigned char)cmd;
    LCD_Pulse();
    __delay_ms(2);
}

void LCD_Char(char data) {
    LCD_RS = 1;
    LCD_RW = 0;
    LCD_DATA = (LCD_DATA & 0xFF00) | (unsigned char)data;
    LCD_Pulse();
    __delay_us(50);
}

void LCD_Print(const char *str) {
    while (*str != '\0') {
        LCD_Char(*str++);
    }
}

void LCD_SetCursor(int row, int col) {
    int address = (row == 0) ? 0x80 : 0xC0;
    address += col;
    LCD_Command(address);
}

void LCD_Init(void) {
    ANSBbits.ANSB15 = 0;
    TRISBbits.TRISB15 = 0;
    TRISDbits.TRISD4 = 0;
    TRISDbits.TRISD5 = 0;
    TRISE &= 0xFF00;

    LCD_RS = 0;
    LCD_RW = 0;
    LCD_E = 0;

    __delay_ms(50);
    LCD_Command(0x38);
    __delay_ms(5);
    LCD_Command(0x38);
    __delay_us(150);
    LCD_Command(0x38);
    LCD_Command(0x38);
    LCD_Command(0x0C);
    LCD_Command(0x01);
    __delay_ms(2);
    LCD_Command(0x06);
}

void Update_LCD_Data(unsigned int ax, int ay, unsigned int az,
                     int d6, int d7, int db, int dv) {
    char line1[32], line2[32], temp[16];
    int i;

    line1[0] = '\0';
    if (!cfg_Ax && !cfg_Ay && !cfg_Az) {
        strcat(line1, "Sem Analog  ");
    } else {
        if (cfg_Ax) {
            sprintf(temp, "X%u ", ax);
            strcat(line1, temp);
        }
        if (cfg_Ay) {
            sprintf(temp, "Y%dC ", ay);
            strcat(line1, temp);
        }
        if (cfg_Az) {
            sprintf(temp, "Z%u Lux", az);
            strcat(line1, temp);
        }
    }
    for (i = strlen(line1); i < 16; i++) line1[i] = ' ';
    line1[16] = '\0';

    line2[0] = '\0';
    if (alert_triggered) {
        sprintf(line2, "ALERT! V:%d", dv);
    } else if (!cfg_D6 && !cfg_D7 && !cfg_DB && !cfg_DV) {
        strcat(line2, "Sem Digital ");
    } else {
        if (cfg_D6) {
            sprintf(temp, "6:%d ", d6);
            strcat(line2, temp);
        }
        if (cfg_D7) {
            sprintf(temp, "7:%d ", d7);
            strcat(line2, temp);
        }
        if (cfg_DB) {
            sprintf(temp, "B:%d ", db);
            strcat(line2, temp);
        }
        if (cfg_DV) {
            sprintf(temp, "V:%d", dv);
            strcat(line2, temp);
        }
    }
    for (i = strlen(line2); i < 16; i++) line2[i] = ' ';
    line2[16] = '\0';

    LCD_SetCursor(0, 0);
    LCD_Print(line1);
    LCD_SetCursor(1, 0);
    LCD_Print(line2);
}

// ----- Toggleable Party Mode -----
void party_mode(void) {
    LCD_Command(0x01);
    __delay_ms(2);
    LCD_SetCursor(0, 0);
    LCD_Print(" *** PARTY  *** ");
    LCD_SetCursor(1, 0);
    LCD_Print(" *** MODE!! *** ");

    unsigned int old_trisa = TRISA;
    unsigned int old_lata = LATA;

    while (PORTDbits.RD6 == 0 && PORTDbits.RD13 == 0 && PORTDbits.RD7 == 0 && PORTAbits.RA7 == 0) {
        __delay_ms(10);
    }
    __delay_ms(50);

    TRISA = (TRISA & 0xFF00) | 0x008C;  // Keep RA2/RA3 as input (I2C), RA7 as input

    while (1) {
        LATA = (LATA & 0xFF80) | (rand() & 0x007F);
        __delay_ms(100);

        if (PORTDbits.RD6 == 0 && PORTDbits.RD13 == 0 && PORTDbits.RD7 == 0 && PORTAbits.RA7 == 0) {
            while (PORTDbits.RD6 == 0 && PORTDbits.RD13 == 0 && PORTDbits.RD7 == 0 && PORTAbits.RA7 == 0) {
                __delay_ms(10);
            }
            __delay_ms(50);
            break;
        }
    }

    TRISA = old_trisa;
    LATA = old_lata;

    LCD_Command(0x01);
    __delay_ms(2);
}

// ----- Credits Mode -----
void credits_mode(void) {
    LCD_Command(0x01);
    __delay_ms(2);

    unsigned int old_trisa = TRISA;
    unsigned int old_lata = LATA;

    TRISA = (TRISA & 0xFF00) | 0x008C;  // Keep RA2/RA3 as input (I2C), RA7 as input

    const char* nomes[] = {
        "Iuri Mocas",
        "Rafael Silva",
        "Tomas Horta"
    };
    const char* numeros[] = {
        "N: 62907",
        "N: 62966",
        "N: 73563"
    };
    int num_membros = 3;

    LCD_SetCursor(0, 0);
    LCD_Print(" Trabalho de SAD ");
    LCD_SetCursor(1, 0);
    LCD_Print("  Criado por:   ");
    __delay_ms(2000);

    while (PORTDbits.RD13 == 0 || PORTDbits.RD7 == 0) {
        __delay_ms(10);
    }
    __delay_ms(50);

    int i = 0;
    int led_pos = 0;
    int led_dir = 1;

    while (1) {
        LCD_Command(0x01);
        __delay_ms(2);

        LCD_SetCursor(0, 0);
        LCD_Print(nomes[i]);

        LCD_SetCursor(1, 0);
        LCD_Print(numeros[i]);

        int delay_count = 0;
        int exit_flag = 0;
        int led_timer = 0;

        while (delay_count < 200) {
            led_timer++;
            if (led_timer >= 8) {
                led_timer = 0;

                LATA = (LATA & 0xFF80) | (1 << led_pos);

                led_pos += led_dir;
                if (led_pos >= 6) {
                    led_pos = 6;
                    led_dir = -1;
                } else if (led_pos <= 0) {
                    led_pos = 0;
                    led_dir = 1;
                }
            }

            if (PORTDbits.RD6 == 0) {
                exit_flag = 1;
                break;
            }
            __delay_ms(10);
            delay_count++;
        }

        if (exit_flag) {
            while (PORTDbits.RD6 == 0) {
                __delay_ms(10);
            }
            __delay_ms(50);
            break;
        }

        i++;
        if (i >= num_membros) i = 0;
    }

    TRISA = old_trisa;
    LATA = old_lata;

    LCD_Command(0x01);
    __delay_ms(2);
}

// ----- Interrupts -----
void __attribute__ ((interrupt, no_auto_psv)) _T1Interrupt(void) {
    IFS0bits.T1IF = 0;
    if (cfg_p > 0) {
        time_tick++;
        if (time_tick >= cfg_p) {
            time_tick = 0;
            sample_flag = 1;
        }
    }
}

void __attribute__ ((interrupt, no_auto_psv)) _U1RXInterrupt(void) {
    IFS0bits.U1RXIF = 0;

    if (U1STAbits.OERR) {
        U1STAbits.OERR = 0;
    }

    while (U1STAbits.URXDA) {
        char c = U1RXREG;

        if (rx_index == 0 && (c == '\r' || c == '\n' || c == ' ')) {
            continue;
        }

        if (rx_index < RX_BUF_SIZE - 1) {
            rx_buffer[rx_index++] = c;
            if (c == '}' || c == '\n') {
                rx_buffer[rx_index] = '\0';
                message_received = 1;
            }
        } else {
            rx_index = 0;
        }
    }
}

// ----- Peripherals Setup -----
void setupTimer1(void) {
    TMR1 = 0;
    PR1 = IPerS / 10;
    IPC0bits.T1IP = 0x5;
    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;
    T1CON = 0x8030;
}

void UART1_Init(void) {
    U1BRG = 25;
    __builtin_write_OSCCONL(OSCCON & 0xbf);
    RPOR8bits.RP17R = 0x0003;
    RPINR18bits.U1RXR = 0x000A;

    U1MODE = 0x0000;
    U1MODEbits.UARTEN = 1;
    U1STA = 0x0000;
    U1STAbits.UTXEN = 1;

    IPC2bits.U1RXIP = 0x4;
    IFS0bits.U1RXIF = 0;
    IEC0bits.U1RXIE = 1;
}

void UART1_Write(char c) {
    while (U1STAbits.UTXBF == 1);
    U1TXREG = c;
}

void UART1_WriteString(const char *str) {
    while (*str != '\0') {
        UART1_Write(*str++);
    }
}

void ADC_Init(void) {
    ANSAbits.ANSA7 = 0;
    TRISBbits.TRISB0 = 1; ANSBbits.ANSB0 = 1;
    TRISBbits.TRISB1 = 1; ANSBbits.ANSB1 = 1;
    TRISBbits.TRISB4 = 1; ANSBbits.ANSB4 = 1;
    TRISBbits.TRISB5 = 1; ANSBbits.ANSB5 = 1;
    AD1CON1 = 0x0000;
    AD1CON1bits.SSRC = 0b000;
    AD1CON2 = 0x0000;
    AD1CON3 = 0x0002;
    AD1CON1bits.ADON = 1;
}

unsigned int ADC_Read(int channel) {
    AD1CHS = channel;
    AD1CON1bits.SAMP = 1;
    for (volatile int i = 0; i < 100; i++);
    AD1CON1bits.SAMP = 0;
    while (!AD1CON1bits.DONE);
    AD1CON1bits.DONE = 0;
    return ADC1BUF0;
}

// ----- JSON Extractor -----
int get_json_val(const char *json, const char *key, int default_val) {
    const char *pos = json;
    int key_len = strlen(key);

    while ((pos = strstr(pos, key)) != NULL) {
        char before = (pos > json) ? *(pos - 1) : ' ';
        if ((before >= 'A' && before <= 'Z') || (before >= 'a' && before <= 'z')) {
            pos += key_len;
            continue;
        }

        pos += key_len;
        char after = *pos;
        if ((after >= 'A' && after <= 'Z') || (after >= 'a' && after <= 'z')) {
            continue;
        }

        while (*pos != '\0' && *pos != ':') pos++;
        if (*pos == ':') {
            pos++;
            while (*pos != '\0' && (*pos < '0' || *pos > '9') && *pos != '-') pos++;
            return atoi(pos);
        }
    }
    return default_val;
}

void process_json_config(const char *json) {
    int old_sd_enabled = cfg_SD0 || cfg_SD1;

    cfg_b = get_json_val(json, "b", cfg_b);
    TRISAbits.TRISA7 = cfg_b;

    int dw = get_json_val(json, "Dw", -1);
    if (dw != -1) {
        LATAbits.LATA7 = dw;
    }

    int s0 = get_json_val(json, "S0", -1);
    if (s0 != -1) {
        LATAbits.LATA0 = s0 & 1;
    }
    int s1 = get_json_val(json, "S1", -1);
    if (s1 != -1) {
        LATAbits.LATA1 = s1 & 1;
    }

    // RA2/RA3 are reserved for I2C2 when the Arduino SD is connected.
    int s2 = get_json_val(json, "S2", -1);
    if (s2 != -1 && !cfg_SD0 && !cfg_SD1) {
        LATAbits.LATA2 = s2 & 1;
    }

    cfg_v = get_json_val(json, "v", cfg_v);

    cfg_Ax = get_json_val(json, "Ax", cfg_Ax);
    cfg_Ay = get_json_val(json, "Ay", cfg_Ay);
    cfg_Az = get_json_val(json, "Az", cfg_Az);
    cfg_SD0 = get_json_val(json, "SD0", cfg_SD0);
    cfg_SD1 = get_json_val(json, "SD1", cfg_SD1);
    cfg_D6 = get_json_val(json, "D6", cfg_D6);
    cfg_D7 = get_json_val(json, "D7", cfg_D7);
    cfg_DB = get_json_val(json, "DB", cfg_DB);
    cfg_DV = get_json_val(json, "DV", cfg_DV);

    int new_p = get_json_val(json, "p", -1);
    if (new_p >= 0) {
        cfg_p = new_p * TIMER_TICKS_PER_SECOND;
    }
    cfg_alert_val = get_json_val(json, "alert", cfg_alert_val);

    int new_n = get_json_val(json, "n", cfg_n);
    if (new_n > 0 && new_n <= MAX_SAMPLES) {
        cfg_n = new_n;
    }

    if (!old_sd_enabled && (cfg_SD0 || cfg_SD1)) {
        I2C2_Init();
    }
}

void print_json_array(const char* label, const unsigned int* arr, const int* arr_int, int is_int, int is_first) {
    char temp[32];
    if (!is_first) UART1_WriteString(", ");

    UART1_WriteString("\"");
    UART1_WriteString(label);
    UART1_WriteString("\":[");

    for (int i = 0; i < cfg_n; i++) {
        if (is_int) {
            sprintf(temp, "%d%s", arr_int[i], (i == cfg_n - 1) ? "" : ", ");
        } else {
            sprintf(temp, "%u%s", arr[i], (i == cfg_n - 1) ? "" : ", ");
        }
        UART1_WriteString(temp);
    }
    UART1_WriteString("]");
}

void send_monitoring_message(void) {
    int first_key = 1;
    UART1_WriteString("{");

    if (cfg_Ax) { print_json_array("Ax", buf_Ax, NULL, 0, first_key); first_key = 0; }
    if (cfg_Ay) { print_json_array("Ay", NULL, buf_Ay, 1, first_key); first_key = 0; }
    if (cfg_Az) { print_json_array("Az", buf_Az, NULL, 0, first_key); first_key = 0; }
    if (cfg_SD0) { print_json_array("SD0", buf_SD0, NULL, 0, first_key); first_key = 0; }
    if (cfg_SD1) { print_json_array("SD1", buf_SD1, NULL, 0, first_key); first_key = 0; }

    if (cfg_D6) { print_json_array("D6", NULL, buf_D6, 1, first_key); first_key = 0; }
    if (cfg_D7) { print_json_array("D7", NULL, buf_D7, 1, first_key); first_key = 0; }
    if (cfg_DB) { print_json_array("DB", NULL, buf_DB, 1, first_key); first_key = 0; }
    if (cfg_DV && cfg_v) { print_json_array("DV", NULL, buf_DV, 1, first_key); first_key = 0; }

    UART1_WriteString("}\r\n");
}

// ----- Main -----
int main(int argc, char** argv) {
    ANSD = 1;
    // Preserve RA2/RA3 as inputs — they are I2C2 SCL/SDA pins.
    // Driving them low as outputs would lock the I2C bus.
    TRISA = 0x000C;            // RA2=1 (input), RA3=1 (input), rest=output
    PORTA = 0x0000;

    TRISDbits.TRISD6 = 1;
    TRISDbits.TRISD7 = 1;
    TRISDbits.TRISD13 = 1;
    TRISAbits.TRISA7 = 0;

    ADC_Init();
    UART1_Init();
    setupTimer1();

#if I2C_DIAG_ON_BOOT
    I2C2_Init();

    // I2C diagnostic: test the bus and report results via UART
    {
        char diag[128];
        int scl_state = PORTAbits.RA2;
        int sda_state = PORTAbits.RA3;
        int step = 0;  // 0=not started

        // Step 1: START
        int s1 = I2C2_Start();
        if (s1) {
            step = 1;
            // Step 2: Write address (write mode)
            int s2 = I2C2_WriteByte((SD_I2C_ADDR << 1) | 0);
            if (s2) {
                step = 2;
                // Step 3: Write command
                int s3 = I2C2_WriteByte(SD_ACQUIRE_CMD);
                if (s3) step = 3;
            }
        }
        I2C2_Stop();

        sprintf(diag,
            "{\"alert\":\"I2C_DIAG: SCL=%d SDA=%d START=%s ADDR_ACK=%s CMD=%s step=%d CONL=0x%04X STAT=0x%04X\"}\r\n",
            scl_state, sda_state,
            (step >= 1) ? "OK" : "FAIL",
            (step >= 2) ? "ACK" : "NACK",
            (step >= 3) ? "OK" : "FAIL",
            step,
            (unsigned int)I2C2CONL,
            (unsigned int)I2C2STAT);
        UART1_WriteString(diag);

        // Re-init after diagnostic
        I2C2_Init();

        // I2C scan: list every 7-bit address that acknowledges on the bus.
        {
            char scan[96];
            char scan_msg[192];
            int found = I2C2_ScanBus(scan, sizeof(scan));

            sprintf(scan_msg,
                "{\"alert\":\"I2C_SCAN: FOUND=%d ADDR=%s CONL=0x%04X STAT=0x%04X\"}\r\n",
                found,
                scan,
                (unsigned int)I2C2CONL,
                (unsigned int)I2C2STAT);
            UART1_WriteString(scan_msg);
        }

        // Re-init after scan
        I2C2_Init();
    }
#endif

    LCD_Init();
    LCD_Print("A Iniciar Sistema");
    LCD_SetCursor(1, 0);
    LCD_Print("Estado: Ativo");

    UART1_WriteString("---- Sistema Inicializado ----\r\n");

    while (1) {
        ClrWdt();

        unsigned int temp_trisa7 = TRISAbits.TRISA7;
        TRISAbits.TRISA7 = 1;
        __delay_us(5);

        if (PORTDbits.RD6 == 0 && PORTDbits.RD13 == 0 && PORTDbits.RD7 == 0 && PORTAbits.RA7 == 0) {
            party_mode();
            if (cfg_SD0 || cfg_SD1) I2C2_Init();
        }
        TRISAbits.TRISA7 = temp_trisa7;

        if (PORTDbits.RD13 == 0 && PORTDbits.RD7 == 0 && PORTDbits.RD6 != 0) {
            credits_mode();
            if (cfg_SD0 || cfg_SD1) I2C2_Init();
        }

        if (message_received) {
            IEC0bits.U1RXIE = 0;
            char process_buf[RX_BUF_SIZE];
            strcpy(process_buf, (char*)rx_buffer);
            rx_index = 0;
            message_received = 0;
            if (U1STAbits.OERR) U1STAbits.OERR = 0;
            IEC0bits.U1RXIE = 1;

            process_json_config(process_buf);

            UART1_WriteString("{\"status\":\"config_updated\"}\r\n");

            LCD_SetCursor(1, 0);
            LCD_Print("Status: Config'd");

            if (sample_count >= cfg_n) sample_count = 0;
        }

        if (sample_flag) {
            sample_flag = 0;

            unsigned int cur_Ax = 0, cur_Az = 0;
            unsigned int cur_SD0 = 0, cur_SD1 = 0;
            int cur_Ay = 0;
            int cur_D6 = 0, cur_D7 = 0, cur_DB = 0, cur_DV = 0;

            if (cfg_Ax) {
                cur_Ax = ADC_Read(5);
                buf_Ax[sample_count] = cur_Ax;
            }

            if (cfg_Ay) {
                unsigned int raw_Ay = ADC_Read(4);
                float voltage = (raw_Ay / 1023.0) * 3.3;
                cur_Ay = (int)((voltage - 0.5) * 100.0);
                buf_Ay[sample_count] = cur_Ay;
            }

            if (cfg_Az) {
                int valorADC = ADC_Read(1);
                if (valorADC == 0) valorADC = 1;
                if (valorADC > 1023) valorADC = 1023; // Prevent division-by-zero
                
                // For a 5k ohm fixed resistor in voltage divider:
                // R_LDR = 5000 * (1024.0 / valorADC - 1.0)
                // R_LDR_kOhm = 5.0 * (1024.0 / valorADC - 1.0)
                // Lux = 500.0 / R_LDR_kOhm
                // Simplified formula: Lux = (100.0 * valorADC) / (1024.0 - valorADC)
                float valorLux = (100.0 * valorADC) / (1024.0 - valorADC);
                
                cur_Az = (unsigned int)valorLux;
                buf_Az[sample_count] = cur_Az;
            }

            if (cfg_SD0 || cfg_SD1) {
                int sd_ok = SD_ReadSensors(&cur_SD0, &cur_SD1);
                if (!sd_ok) {
                    cur_SD0 = SD_READ_ERROR;
                    cur_SD1 = SD_READ_ERROR;
                }
                if (cfg_SD0) buf_SD0[sample_count] = cur_SD0;
                if (cfg_SD1) buf_SD1[sample_count] = cur_SD1;
            }

            if (cfg_D6 && cfg_v == 0) {
                cur_D6 = PORTDbits.RD6;
                buf_D6[sample_count] = cur_D6;
            }
            if (cfg_D7 && cfg_v == 0) {
                cur_D7 = PORTDbits.RD7;
                buf_D7[sample_count] = cur_D7;
            }

            if (cfg_DB && cfg_b == 1) {
                cur_DB = PORTAbits.RA7;
                buf_DB[sample_count] = cur_DB;
            } else if (cfg_DB && cfg_b == 0) {
                cur_DB = LATAbits.LATA7;
                buf_DB[sample_count] = cur_DB;
            }

            if (cfg_v) {
                cur_DV = (PORTDbits.RD7 << 1) | PORTDbits.RD6;
                if (cfg_DV) buf_DV[sample_count] = cur_DV;
            }

            sample_count++;

            if (sample_count >= cfg_n) {
                send_monitoring_message();
                sample_count = 0;
            }

            if (cfg_v && cfg_alert_val != -1) {
                if (cur_DV == cfg_alert_val && !alert_triggered) {
                    char alert_msg[64];
                    sprintf(alert_msg, "{\"alert\":\"threshold_reached\", \"v_val\":%d}\r\n", cur_DV);
                    UART1_WriteString(alert_msg);
                    alert_triggered = 1;

                    LCD_SetCursor(1, 0);
                    LCD_Print("Status: ALERT!  ");
                } else if (cur_DV != cfg_alert_val) {
                    if (alert_triggered) {
                        alert_triggered = 0;
                        LCD_SetCursor(1, 0);
                        LCD_Print("Status: Active  ");
                    }
                }
            }

            Update_LCD_Data(cur_Ax, cur_Ay, cur_Az, cur_D6, cur_D7, cur_DB, cur_DV);
        }
    }

    return (1);
}
