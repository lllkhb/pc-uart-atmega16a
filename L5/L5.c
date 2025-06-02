#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <util/delay.h>

#define F_CPU_MK 4000000UL
#define BAUD 19200UL
#define UBRR_value ((F_CPU_MK / (16 * BAUD)) - 1)

#define bufRX_SIZE 32
#define bufTX_SIZE 32
#define bufTMP_SIZE 32

#define MODE_LOW 0
#define MODE_HIGH 1
#define MODE_ALL 2

char mode = -1;
char start = 0;
int adc_result = 0;
int ADCarray[10] = {0};
char adc_index = 0;

unsigned char buf[8];
unsigned char bufRX[bufRX_SIZE] = {0};
unsigned char bufTMP[bufTMP_SIZE] = {0};

void UART_Init(void) {
    UBRRL = UBRR_value;
    UBRRH = (UBRR_value >> 8);
    UCSRB = (1 << TXEN) | (1 << RXEN);
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
}

void UART_send(unsigned char value) {
    while (!(UCSRA & (1 << UDRE)));
    UDR = value;
}

void UART_send_str(unsigned char *str) {
    while (*str != '\0') {
        UART_send(*str++);
    }
}

unsigned char UART_receive(void) {
    while (!(UCSRA & (1 << RXC)));
    return UDR;
}

void adc_init() {
    ADMUX |= (1 << REFS0); 
    ADCSRA |= (1 << ADEN);
    SFIOR |= (1 << ADTS1) | (1 << ADTS0); 

ISR(TIMER0_COMP_vect) {
    if (start) {
        ADCSRA |= (1 << ADSC);     
        while (ADCSRA & (1 << ADSC));
        adc_result = ADC;
    }
}

void timer0_init_ctc() {
    TCCR0 |= (1 << WGM01);  
    TCCR0 |= (1 << CS02); 
    OCR0 = 12;          
    TIMSK |= (1 << OCIE0);  
}

int UART_receive_cmd(void) {
    int cnt = 0;
    unsigned char rxbyte;
    do {
        rxbyte = UART_receive();
        bufRX[cnt++] = rxbyte;
    } while (rxbyte != '\n' && cnt < bufRX_SIZE - 1);
    bufRX[cnt - 2] = '\0';
    return cnt;
}

void Command_parser(void) {
    while (1) {
        UART_receive_cmd();
		if (strcmp(bufRX, "start") == 0) {
            start = 1;
            UART_send_str("Mode: start\r\n");
        } else if (strcmp(bufRX, "under500") == 0) {
            mode = MODE_LOW;
            UART_send_str("Mode: under500\r\n");
        } else if (strcmp(bufRX, "over500") == 0) {
            mode = MODE_HIGH;
            UART_send_str("Mode: over500\r\n");
        } else if (strcmp(bufRX, "all") == 0) {
            mode = MODE_ALL;
            UART_send_str("Mode: all\r\n");
        } else if (strcmp(bufRX, "off") == 0) {
            start = 0;
            UART_send_str("Mode: off\r\n");
        } else {
            UART_send_str("Unknown command\r\n");
        }
    }
}

ISR(INT0_vect) {
    if (start && adc_index < 10) {
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));
        ADCarray[adc_index] = ADC;
        
        itoa(ADCarray[adc_index], buf, 10);
        UART_send_str("ADC[");
        UART_send(adc_index + '0');
        UART_send_str("] = ");
        UART_send_str(buf);
        UART_send_str("\r\n");

        adc_index++; 
    }
}





ISR(INT1_vect) {
	if(start){
    if (mode == MODE_ALL){
        for (int i = 0; i < 10; i++) {
            itoa(ADCarray[i], buf, 10);
            UART_send_str("ADC = ");
            UART_send_str(buf);
            UART_send_str("\r\n");
            _delay_ms(5); 
        }
    }
	if(mode == MODE_LOW){
		for (int i = 0; i < 10; i++) {
			if(ADCarray[i] < 500){
				itoa(ADCarray[i], buf, 10);
				UART_send_str("ADC = ");
				UART_send_str(buf);
				UART_send_str("\r\n");
				_delay_ms(5);
			} else{
				i++;
			}				
        }
	}
        if(mode == MODE_HIGH){
			for (int i = 0; i < 10; i++) {
				if(ADCarray[i] >= 500){
				itoa(ADCarray[i], buf, 10);
				UART_send_str("ADC = ");
				UART_send_str(buf);
				UART_send_str("\r\n");
				_delay_ms(5);
			} else{
				i++;
			}	
			}
		}
	}	
}

int main(void) {
    // Порти
    DDRA = 0x00;    
    DDRD = 0b00000010;
    MCUCR &= ~((1 << ISC00) | (1 << ISC01) | (1 << ISC10) | (1 << ISC11));
    MCUCR |= (1 << ISC01) | (1 << ISC00);
    MCUCR |= (1 << ISC11) | (1 << ISC10);
    GICR |= (1 << INT0) | (1 << INT1);

    UART_Init();
    adc_init();
    timer0_init_ctc();

    sei(); 
	UART_send_str("System ready.\r\n");

    Command_parser();
}


