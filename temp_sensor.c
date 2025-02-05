#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

#define BAUD_RATE 9600
#define BAUD_PRESCALE ((F_CPU / (BAUD_RATE * 16UL)) - 1)
#define DHT11_PIN PD6
#define TIMEOUT 255

uint8_t c=0,I_RH,D_RH,I_Temp,D_Temp,CheckSum;

void UART_init(long USART_BAUDRATE)
{
	unsigned int ubrr = F_CPU/16/USART_BAUDRATE-1;
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	
	// Enable receiver and transmitter
	UCSRB = (1<<RXEN)|(1<<TXEN);
	
	// Set frame format: 8data, 1stop bit
	UCSRC = (1<<URSEL)|(1<<UCSZ0)|(1<<UCSZ1);
}

void UART_TxChar(char c)
{
	while (!(UCSRA & (1<<UDRE)));
	UDR = c;
}

void UART_sendString(char *str)
{
	while (*str)
	{
		UART_TxChar(*str++);
	}
}

void Request()
{
	DDRD |= (1<<DHT11_PIN);
	PORTD &= ~(1<<DHT11_PIN);
	_delay_ms(20);
	PORTD |= (1<<DHT11_PIN);
}

uint8_t Response() {
	DDRD &= ~(1 << DHT11_PIN); // Set pin as input
	uint8_t timeout = 0;
	while ((PIND & (1 << DHT11_PIN)) && (++timeout < TIMEOUT)); // Wait for the pin to go low
	if (timeout >= TIMEOUT) return 0; // Timeout error
	timeout = 0;
	while (!(PIND & (1 << DHT11_PIN)) && (++timeout < TIMEOUT)); // Wait for the pin to go high
	if (timeout >= TIMEOUT) return 0; // Timeout error
	timeout = 0;
	while ((PIND & (1 << DHT11_PIN)) && (++timeout < TIMEOUT)); // Wait for the pin to go low
	if (timeout >= TIMEOUT) return 0; // Timeout error
	return 1; // Success
}

uint8_t Receive_data() {
	c = 0;
	for (int q = 0; q < 8; q++) {
		while ((PIND & (1 << DHT11_PIN)) == 0); // Wait for the pin to go high
		_delay_us(30); // Wait for 30 microseconds
		c = (c << 1) | ((PIND & (1 << DHT11_PIN)) ? 1 : 0); // Read the bit
		while (PIND & (1 << DHT11_PIN)); // Wait for the pin to go low
	}
	return c;
}

int main(void)
{
	UART_init(BAUD_RATE);
	_delay_ms(1000); // Startup delay
	UART_sendString("ATmega16 Started\r\n");
	
	while (1)
	{
		Request();
        if (!Response()) {
            UART_sendString("Sensor response timeout\r\n");
            continue; // Skip this iteration
        }
		
		Response();
		I_RH=Receive_data();
		D_RH=Receive_data();
		I_Temp=Receive_data();
		D_Temp=Receive_data();
		CheckSum=Receive_data();
		
		if ((I_RH + D_RH + I_Temp + D_Temp) == CheckSum)
		{
			// Convert temperature to integer (multiplied by 10 to preserve one decimal place)
			int temp = I_Temp * 10 + D_Temp;
			char buffer[20];
			sprintf(buffer, "%d\r\n", temp);
			UART_sendString(buffer);
		}
		
		_delay_ms(2000); // Wait 2 seconds between readings
	}
	return 0;
}