#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"


#define MAX_PACKET_SIZE (128)

/*
	command_task will recieve a command register
	from the laptop server, and will then send data
	through uart to the SJSUOne board
*/
void command_task(void);


/*
	diagnostic_task will receive data from the SJSUOne board
	and route it through wifi to the laptop server to display
*/
void diagnostic_task(void); // esp32 acting as client


/*
	send_uart will take in data from the command task
	and transfer it to the SJSUOne board through uart
*/
void command_send_uart_data(void);

/*
	receive uart from SJSUOne board
	and send to laptop for display
*/
void diag_receive_uart_data(void);

/*
	initializes uart for send and receive
*/
void uart_init(void);


// global dat struct
typedef struct{
    uint8_t length;
    uint8_t opcode;
    union
    {
        uint8_t  bytes[2];
        uint16_t half_word;
    } command;
} __attribute__((packed)) command_packet_S;

typedef struct{

    uint8_t length;
    uint8_t type;
    uint8_t payload[MAX_PACKET_SIZE];

} __attribute__((packed)) diagnostic_packet_S;

