#if defined(__AVR_ATmega328PB__)
    #define LED_DDR			DDRB
    #define LED_PORT		PORTB
    #define LED_PIN			PINB
    #define LED				PINB5

    #define ESP_DDR			DDRE
    #define ESP_PORT		PORTE
    #define ESP_PIN			PINE
    #define ESP_ONOFF		PINE1
    #define ESP_BOOT_PIN	PINE3

    // Ports for soft UART
    #ifdef SOFT_UART
    #	define UART_PORT	PORTD
    #	define UART_PIN		PIND
    #	define UART_DDR		DDRD
    #	define UART_TX_BIT	1
    #	define UART_RX_BIT	0
    #endif

#endif