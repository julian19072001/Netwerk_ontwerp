/*Julian Della Guardia*/

#define F_CPU 32000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "clock.h"
#include "serialF0.h"
#include "mesh_radio.h"

#define DEVICE_ADDRESS  0x02                           //address of device (each address can only be used once in a network)

void set_color(int *red, int *green, int *blue, uint8_t *string);
void     init_adc(void);
int16_t  read_adc(void);
void init_pwm(void);

int main(void)
{
	init_clock();
    init_stream(F_CPU);
	radio_init(DEVICE_ADDRESS);
	init_adc();
	init_pwm();

	PMIC.CTRL |= PMIC_LOLVLEN_bm;			//turn on low level interrupts	

	sei();

	uint8_t received_data[32] = {0};
	uint8_t send_data[2];

	int green = 0;
	int red = 0;
	int blue = 0;

	uint16_t adc_value = 0;
	
	while(1)
	{	
		adc_value = read_adc() + 10;
		send_data[0] = (uint8_t)(adc_value >> 8);
		send_data[1] = (uint8_t)adc_value; 
		send_radio_data(2, send_data, 2);

		if(get_radio_data(received_data)){
			switch(received_data[0])
			{
			case 0x01:
				uint8_t *color_received = received_data + 4;

				printf("%s\n", color_received);

				set_color(&red, &green, &blue, color_received);
			}
		}
		
		TCC0.CCA = blue * adc_value;													//set the color on onboard led the same as the tft number color
		TCF0.CCA = green * adc_value;
		TCF0.CCB = red * adc_value;

		_delay_ms(100);
	}
}

void set_color(int *red, int *green, int *blue, uint8_t *string){
	if (strcmp((char *)string, "red") == 0) {
		*green = 0;
		*red = 1;
		*blue = 0;
	} else if (strcmp((char *)string, "green") == 0) {
		*green = 1;
		*red = 0;
		*blue = 0;
	} else if (strcmp((char *)string, "blue") == 0) {
		*green = 0;
		*red = 0;
		*blue = 1;
	} else if (strcmp((char *)string, "cyan") == 0) {
		*green = 1;
		*red = 0;
		*blue = 1;
	} else if (strcmp((char *)string, "magenta") == 0) {
		*green = 0;
		*red = 1;
		*blue = 1;
	} else if (strcmp((char *)string, "yellow") == 0) {
		*green = 1;
		*red = 1;
		*blue = 0;
	} else if (strcmp((char *)string, "white") == 0) {
		*green = 1;
		*red = 1;
		*blue = 1;
	} else {
		*green = 0;
		*red = 0;
		*blue = 0;
	}
}

void init_adc(void)
{
  PORTA.DIRCLR     = PIN0_bm;                              // 
  ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc |               // PA0 to + channel 0
                     ADC_CH_MUXNEG_GND_MODE3_gc;           // GND to - channel 0
  ADCA.CH0.CTRL    = ADC_CH_INPUTMODE_DIFF_gc;             // channel 0 differential
  ADCA.REFCTRL     = ADC_REFSEL_INTVCC_gc;
  ADCA.CTRLB       = ADC_RESOLUTION_12BIT_gc |
                     ADC_CONMODE_bm;                       // signed conversion
  ADCA.PRESCALER   = ADC_PRESCALER_DIV16_gc;
  ADCA.CTRLA       = ADC_ENABLE_bm;
}

int16_t read_adc(void)                                     // return a signed
{
  int16_t res;                                             // is also signed

  ADCA.CH0.CTRL |= ADC_CH_START_bm;
  while ( !(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm) ) ;
  res = ADCA.CH0.RES;
  ADCA.CH0.INTFLAGS |= ADC_CH_CHIF_bm;

  return res;
}

void init_pwm(void){
	PORTE.DIRCLR = PIN1_bm;															//configure sensor pins
	PORTE.DIRSET = PIN0_bm;
	PORTE.OUTCLR = PIN0_bm;
	
	PORTC_DIRSET = PIN0_bm;															//configure RGB LED pins
	PORTF.DIRSET = PIN0_bm | PIN1_bm;
	
	TCC0.CTRLB   = TC0_CCAEN_bm | TC_WGMODE_SINGLESLOPE_gc;							//create clock for PWM for the green and red channel
	TCC0.CTRLA   = TC_CLKSEL_DIV2_gc;
	TCC0.PER     = 7999;
	
	TCF0.CTRLB   = TC0_CCAEN_bm | TC0_CCBEN_bm | TC_WGMODE_SINGLESLOPE_gc;			//create clock for PWM for the green and red channel
	TCF0.CTRLA   = TC_CLKSEL_DIV2_gc;
	TCF0.PER     = 7999;
}