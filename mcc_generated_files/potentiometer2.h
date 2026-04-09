/**
  POTENTIOMETER2 Generated Driver API Header File

  @Company
    Microchip Technology Inc.

  @File Name
    potentiometer2.h

  @Summary
    This is the generated header file for the POTENTIOMETER2 driver using Board Support Library

  @Description
    This header file provides APIs for driver for POTENTIOMETER2.
    Generation Information :
        Product Revision  :  Board Support Library - 2.0.0
        Device            :  PIC24FJ1024GB610
        Driver Version    :  0.95
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

#ifndef POTENTIOMETER2_H
#define POTENTIOMETER2_H

#include <stdint.h>


/**
    Section: Function 
*/
/**
  @Summary
    This method returns the ADC value of the potentiometer

  @Description
    This is the generated for potentiometer driver, This method 
    can be used to get the potentiometer ADC channel values. The ADC values
    are converted to 16-bit format.

  @Preconditions
   ADC should be loaded and the potentiometer pin has to be selected as an Analog input pin.

  @Returns
    Potentiometer ADC value in 16-bit number format.

  @Param
    None.


*/

uint16_t POTENTIOMETER2_Get(void);
#endif