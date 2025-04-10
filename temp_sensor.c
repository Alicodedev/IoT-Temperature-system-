#define F_CPU 16000000UL // Set freq of 16Mhz
#include <avr/io.h> 
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>

#define BAUD_RATE 9600 // baud rate set for UART
#define BAUD_PRESCALE ((F_CPU / (BAUD_RATE * 16UL)) - 1) // Calculate baud rate prescaler
#define SENSOR_PIN 7 // DHT 11 input pin 

// DHT 11 global variables to store sensor data
uint8_t dht_byte = 0;        // buffer variable for holding bits
uint8_t humidity_int;       
uint8_t humidity_dec;        
uint8_t temp_int;            
uint8_t temp_dec;            
uint8_t checksum_value;      // Checksum for verifying data 

void UART_init(long USART_BAUDRATE) // UART initialization 
{
	unsigned int ubrr = F_CPU/16/USART_BAUDRATE-1; // calculate baud rate register value
	
	UBRRH = (unsigned char)(ubrr>>8); // set uart baud rate registers
	UBRRL = (unsigned char)ubrr;
	
	// enable receiver and transmitter registers 
	UCSRB = (1<<RXEN)|(1<<TXEN);
	
	// set frame format: 8 data bits, 1 stop bit
	UCSRC = (1<<URSEL)|(1<<UCSZ0)|(1<<UCSZ1);
}

void UART_TxChar(char c)// Single character over uart
{
	while (!(UCSRA & (1<<UDRE)));// wait for transmitter buffer to empty
	UDR = c;// store data into buffer for transmission
}

void UART_sendString(char *str)// Send string over uart
{
	while (*str) // transmit individual character until null terminator
	{
		UART_TxChar(*str++);// call function to transmit 
	}
}

void DHT_StartSignal() // request signal sent to DHT 11 
{
	DDRD |= (1<<SENSOR_PIN); // set as output
	PORTD &= ~(1<<SENSOR_PIN);// pin pulled low
	_delay_ms(20);// delay of 20 ms
	PORTD |= (1<<SENSOR_PIN);// pin pulled high
}

void DHT_Response() // waits for sensor to detect response after request
{
	DDRD &= ~(1<<SENSOR_PIN); // set as input
	while(PIND & (1<<SENSOR_PIN));// wait when pin goes low
	while((PIND & (1<<SENSOR_PIN))==0);// wait when pin goes high (80us)
	while(PIND & (1<<SENSOR_PIN));// wait when pin goes low (80us)
}

uint8_t DHT_ReadByte() // receive 8 bit from DHT 11
{
	dht_byte = 0;
	for (int q=0; q<8; q++) // iterate through each bit
	{
		while((PIND & (1<<SENSOR_PIN)) == 0);// wait when pin goes high
		_delay_us(30);
		if(PIND & (1<<SENSOR_PIN)) // checks if pin is high resulting in '1' else '0'
		dht_byte = (dht_byte<<1)|(0x01);
		else
		dht_byte = (dht_byte<<1);
		while(PIND & (1<<SENSOR_PIN));
	}
	return dht_byte; // return assembled byte
}


// ### MAIN FUNCTION ###
int main(void)
{
	UART_init(BAUD_RATE);// UART initialization
	_delay_ms(1000); // Startup delay
	UART_sendString("ATmega16 Started\r\n"); // confirm UART is working
	
	while (1)
	{
		// read data from sensor
		DHT_StartSignal(); // send DHT 11 sensor request
		DHT_Response(); // wait for response 
		
		// 5 bytes of data received from DHT 11
		humidity_int=DHT_ReadByte(); // integer humidity 
		humidity_dec=DHT_ReadByte(); // decimal humidity
		temp_int=DHT_ReadByte();// integer temp
		temp_dec=DHT_ReadByte(); // decimal temp
		checksum_value=DHT_ReadByte();// check sum to verify first 4 bytes 
		
		if ((humidity_int + humidity_dec + temp_int + temp_dec) == checksum_value)
		{
			// Convert temperature to integer (multiplied by 10 to hold decimal place)
			int temp = temp_int * 10 + temp_dec;
			char buffer[20];
			sprintf(buffer, "%d\r\n", temp); // format temp for transmission
			UART_sendString(buffer); // temperature data send via uart
		}
		
		_delay_ms(2000); // Wait 2 seconds between readings
	}
	return 0;
}
