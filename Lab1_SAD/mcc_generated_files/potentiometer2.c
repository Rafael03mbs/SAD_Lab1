/**
  POTENTIOMETER2 Generated Driver API Source File

  @Company
    Microchip Technology Inc.

  @File Name
    potentiometer2.c

  @Summary
    This is the generated source file for the POTENTIOMETER2 driver using Board Support Library

  @Description
    This source file provides APIs for driver for POTENTIOMETER2.
    Generation Information :
        Product Revision  :  Board Support Library - 2.0.0
        Device            :  PIC24FJ1024GB610
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.50
        MPLAB 	          :  MPLAB X v5.40
*/
/*
    (c) 2016 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

#include <xc.h>
#include "potentiometer2.h"
#include "adc1.h"

#define POT_ADC_REF_VOLTAGE          (double)3.3     
#define POT_VIN        0
#define POT_ADC_RESOLUTION           (double)1024
uint16_t POTENTIOMETER2_Get(void){
    
    volatile uint16_t i=0;
    uint16_t value = 0;
    // No Pin is selected for Wiper function
    return value;
}


double POTENTIOMETER2_VoltageGet(void)
{
    double voltage=0;
    uint16_t adcValue = 0;
    // Get the ADC value
    adcValue = POTENTIOMETER2_Get();
    /*
    voltage Value = (Actual ADC Value / ADC Resolution) * ADC Ref Voltage  
    */
    voltage = ((adcValue /POT_ADC_RESOLUTION)* POT_ADC_REF_VOLTAGE);
    return voltage;
}

/**
 End of File
 */