/*
#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/delay.h>

#define F_CPU_MK 4000000UL
#define BAUD 19200UL
#define UBRR_value ((F_CPU_MK / (BAUD * 16)) - 1) // 2

char buf[8];
int adc_result = 0;


void UART_Init()
{
  UBRRL = UBRR_value;
  UBRRH = (UBRR_value >> 8);
  UCSRB = (1 << TXEN);
  UCSRC |= (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
}

void UART_send(char value){

  while(!(UCSRA&(1<<UDRE)));
  UDR=value;

}

void adc_init(){
	ADMUX |= (1 << REFS0);                   
    ADCSRA |= (1 << ADEN);
	SFIOR |= (1 << ADTS1) | (1 << ADTS0); 
}

ISR(TIMER0_COMP_vect){
	 ADCSRA |= (1 << ADSC);
	 adc_result = ADC;
}

void timer0_init_ctc(){
	TCCR0 |= (1 << WGM01);          
    TCCR0 |= (1 << CS02);           
    OCR0 = 12;                  
    TIMSK |= (1 << OCIE0); 
}

ISR(INT0_vect){
	int i=0;
	itoa(adc_result,buf,10);
	while(buf[i]!='\0'){
		UART_send(buf[i]);
		i++;
	}
	UART_send('\r');
	UART_send('\n'); 
}



int main(void)
{
	DDRD = 0b00000010;
	DDRA = 0b00000000;
	
	MCUCR |= (1 << ISC01) | (1 << ISC00);    
    MCUCR |= (1 << ISC11) | (1 << ISC10);  
    GICR |= (1 << INT0) | (1 << INT1); 
    sei(); 
	
	timer0_init_ctc();
	adc_init();
	UART_Init();


  
  while(1){
    
  }
  
}
*/



//////
/*
#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#define F_CPU_MK 4000000UL
#define BAUD 19200UL
#define UBRR_value ((F_CPU_MK / (16 * BAUD)) - 1)

#define bufRX_SIZE 32
#define bufTX_SIZE 32
#define bufTMP_SIZE 32

volatile int start = 0;
volatile int adc_result = 0;

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
    while (!(UCSRA & (1 << RXC))) {}
    return UDR;
}

void adc_init() {
    ADMUX |= (1 << REFS0);    // AREF = AVcc
    ADCSRA |= (1 << ADEN);    // ADC enable
    SFIOR |= (1 << ADTS1) | (1 << ADTS0); // Timer/Counter0 Compare Match
}

ISR(TIMER0_COMP_vect) {
    if (start) {
        ADCSRA |= (1 << ADSC);        // Start ADC conversion
        while (ADCSRA & (1 << ADSC)); // Wait until conversion complete
        adc_result = ADC;
    }
}

void timer0_init_ctc() {
    TCCR0 |= (1 << WGM01);    // CTC mode
    TCCR0 |= (1 << CS02);     // prescaler 256
    OCR0 = 12;                // 80 Hz
    TIMSK |= (1 << OCIE0);    // interrupt enable
}

int UART_receive_cmd(void) {
    int cnt = 0;
    unsigned char rxbyte;
    do {
        rxbyte = UART_receive();
        bufRX[cnt++] = rxbyte;
    } while (rxbyte != '\n' && cnt < bufRX_SIZE - 1);
    bufRX[cnt - 2] = '\0'; // видаляємо \r
    return cnt;
}

void Command_parser(void) {
    unsigned char *ptr;
    int cmdlen = 0;

    while (1) {
        UART_receive_cmd();
        strcpy(bufTMP, bufRX);
        ptr = strtok(bufTMP, " ");
        cmdlen = 0;

        while (ptr != NULL) {
            cmdlen++;
            ptr = strtok(NULL, " ");
        }

        itoa(cmdlen, buf, 10);
        UART_send_str("cmdlen=");
        UART_send_str(buf);
        UART_send_str("\r\n");

        if (cmdlen == 1) {
            if (strcmp(bufRX, "start") == 0) {
                start = 1;
                UART_send_str("start ok\r\n");
            } else if (strcmp(bufRX, "stop") == 0) {
                start = 0;
                UART_send_str("stop ok\r\n");
            } else if (strcmp(bufRX, "read") == 0) {
                itoa(adc_result, buf, 10);
                UART_send_str("ADC = ");
                UART_send_str(buf);
                UART_send_str("\r\n");
            } else {
                UART_send_str("Unknown\r\n");
            }
        } else if (cmdlen == 2) {
            ptr = strtok(bufRX, " ");
            if (strcmp(ptr, "LED") == 0) {
                ptr = strtok(NULL, " ");
                if (strcmp(ptr, "ON") == 0) {
                    PORTD |= (1 << PD7);
                    UART_send_str("LED ON\r\n");
                } else if (strcmp(ptr, "OFF") == 0) {
                    PORTD &= ~(1 << PD7);
                    UART_send_str("LED OFF\r\n");
                } else {
                    UART_send_str("LED cmd unknown\r\n");
                }
            } else {
                UART_send_str("Unknown\r\n");
            }
        } else {
            UART_send_str("Unknown\r\n");
        }
    }
}

int main(void) {
    DDRD |= (1 << PD7); // LED
    PORTD &= ~(1 << PD7);

    UART_Init();
    adc_init();
    timer0_init_ctc();

    sei(); // Enable interrupts

    Command_parser();
}
*/


/////////////

/*
#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#define F_CPU_MK 4000000UL
#define BAUD 19200UL
#define UBRR_value ((F_CPU_MK / (16 * BAUD)) - 1)

#define bufRX_SIZE 32
#define bufTX_SIZE 32
#define bufTMP_SIZE 32

#define MODE_OFF 0
#define MODE_LOW 1
#define MODE_HIGH 2
#define MODE_ALL 3

char mode = MODE_OFF;
int adc_result = 0;

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
    while (!(UCSRA & (1 << RXC))) {}
    return UDR;
}

void adc_init() {
    ADMUX |= (1 << REFS0);    // AREF = AVcc
    ADCSRA |= (1 << ADEN);    // ADC enable
    SFIOR |= (1 << ADTS1) | (1 << ADTS0); // Timer/Counter0 Compare Match
}

ISR(TIMER0_COMP_vect) {
    if (mode != MODE_OFF) {
        ADCSRA |= (1 << ADSC);        // Start ADC conversion
        while (ADCSRA & (1 << ADSC)); // Wait until conversion complete
        adc_result = ADC;

        // Обробка за режимом
        if ((mode == MODE_LOW && adc_result < 500) ||
            (mode == MODE_HIGH && adc_result >= 500) ||
            (mode == MODE_ALL)) {

            itoa(adc_result, buf, 10);
            UART_send_str("ADC = ");
            UART_send_str(buf);
            UART_send_str("\r\n");
        }
    }
}

void timer0_init_ctc() {
    TCCR0 |= (1 << WGM01);    // CTC mode
    TCCR0 |= (1 << CS02);     // prescaler 256
    OCR0 = 12;                // 80 Hz
    TIMSK |= (1 << OCIE0);    // interrupt enable
}

int UART_receive_cmd(void) {
    int cnt = 0;
    unsigned char rxbyte;
    do {
        rxbyte = UART_receive();
        bufRX[cnt++] = rxbyte;
    } while (rxbyte != '\n' && cnt < bufRX_SIZE - 1);
    bufRX[cnt - 2] = '\0'; // видаляємо \r
    return cnt;
}

void Command_parser(void) {
    while (1) {
        UART_receive_cmd();
        if (strcmp(bufRX, "under500") == 0) {
            mode = MODE_LOW;
            UART_send_str("Mode: under500\r\n");
        } else if (strcmp(bufRX, "over500") == 0) {
            mode = MODE_HIGH;
            UART_send_str("Mode: over500\r\n");
        } else if (strcmp(bufRX, "все підряд") == 0) {
            mode = MODE_ALL;
            UART_send_str("Mode: all\r\n");
        } else if (strcmp(bufRX, "off") == 0) {
            mode = MODE_OFF;
            UART_send_str("Mode off\r\n");
        } else {
            UART_send_str("Unknown command\r\n");
        }
    }
}

int main(void) {
    UART_Init();
    adc_init();
    timer0_init_ctc();

    sei(); // Enable interrupts

    Command_parser();
}
*/


////////////

#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>

#define F_CPU_MK 4000000UL
#define BAUD 19200UL
#define UBRR_value ((F_CPU_MK / (16 * BAUD)) - 1)

#define bufRX_SIZE 32
#define bufTX_SIZE 32
#define bufTMP_SIZE 32

#define MODE_OFF 0
#define MODE_LOW 1
#define MODE_HIGH 2
#define MODE_ALL 3

char mode = MODE_OFF;
volatile char flag_measure = 0; // прапорець на запит вимірювання
int adc_result = 0;

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
    while (!(UCSRA & (1 << RXC))) {}
    return UDR;
}

void adc_init() {
    ADMUX |= (1 << REFS0);    // AREF = AVcc
    ADCSRA |= (1 << ADEN);    // ADC enable
}

void timer0_init_ctc() {
    TCCR0 |= (1 << WGM01);    // CTC mode
    TCCR0 |= (1 << CS02);     // prescaler 256
    OCR0 = 12;                // 80 Hz (не використовується тут)
    TIMSK |= (1 << OCIE0);    // interrupt enable (опціонально)
}

void ext_int0_init() {
    GICR |= (1 << INT0);      // enable INT0
    MCUCR |= (1 << ISC01);    // falling edge
    MCUCR &= ~(1 << ISC00);
}

ISR(INT0_vect) {
    flag_measure = 1;
}

void process_adc(void) {
    if (flag_measure && mode != MODE_OFF) {
        flag_measure = 0;

        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC));
        adc_result = ADC;

        if ((mode == MODE_LOW && adc_result < 500) ||
            (mode == MODE_HIGH && adc_result >= 500) ||
            (mode == MODE_ALL)) {

            itoa(adc_result, buf, 10);
            UART_send_str("ADC = ");
            UART_send_str(buf);
            UART_send_str("\r\n");
        }
    }
}

int UART_receive_cmd(void) {
    int cnt = 0;
    unsigned char rxbyte;
    do {
        rxbyte = UART_receive();
        bufRX[cnt++] = rxbyte;
    } while (rxbyte != '\n' && cnt < bufRX_SIZE - 1);
    bufRX[cnt - 2] = '\0'; // видаляємо \r
    return cnt;
}

void Command_parser(void) {
    while (1) {
        process_adc(); // перевірка і виконання вимірювання (по прапорцю)

        if (UCSRA & (1 << RXC)) { // якщо є команда
            UART_receive_cmd();
            if (strcmp(bufRX, "under500") == 0) {
                mode = MODE_LOW;
                UART_send_str("Mode: under500\r\n");
            } else if (strcmp(bufRX, "over500") == 0) {
                mode = MODE_HIGH;
                UART_send_str("Mode: over500\r\n");
            } else if (strcmp(bufRX, "все підряд") == 0) {
                mode = MODE_ALL;
                UART_send_str("Mode: all\r\n");
            } else if (strcmp(bufRX, "off") == 0) {
                mode = MODE_OFF;
                UART_send_str("Mode off\r\n");
            } else {
                UART_send_str("Unknown command\r\n");
            }
        }
    }
}

int main(void) {
    DDRD &= ~(1 << PD2); // INT0 як вхід
    PORTD |= (1 << PD2); // підтяжка до VCC

    UART_Init();
    adc_init();
    ext_int0_init();

    sei(); // Enable interrupts

    Command_parser();
}
