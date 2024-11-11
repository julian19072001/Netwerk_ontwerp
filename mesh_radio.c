#include <avr/interrupt.h>
#include "mesh_radio.h"
uint8_t pipe[5] = {0x30, 0x47, 0x72, 0x70, 0x45}; 		//pipe address ""
uint8_t received_packet[32] = {0};				//create a place to store received data
volatile uint8_t new_data = 0;                             //place to keep track if there is unprocessed data

//check if received message was already received before
uint8_t check_message_new(uint8_t* payload);

//interrupt from NFR IC
ISR(PORTF_INT0_vect)
{
	uint8_t tx_ds, max_rt, rx_dr;
	uint8_t packet_length;
	
	nrfWhatHappened(&tx_ds, &max_rt, &rx_dr);

	if ( rx_dr ) {
		packet_length = nrfGetDynamicPayloadSize();	 //get the size of the received data
		nrfRead(received_packet, packet_length);	 //store the received data
        
        if(check_message_new(received_packet)){
            new_data = 1;

            nrfStopListening();
            cli();
            nrfWrite( (uint8_t *) &received_packet, packet_length);
            sei();
            nrfStartListening();
        }
	}
}

//check if received message was already received before
uint8_t check_message_new(uint8_t* payload){
    static uint8_t previous_message[256] = {0};

    if(payload[1] == previous_message[payload[0]]) return 0;
    if(payload[0] == DEVICE_ADDRESS) return 0;
    
    previous_message[payload[0]] = payload[1];
    return 1;
}

void send_radio_data(uint8_t data_type, uint8_t* data, uint8_t data_size){
    
    if(data_size > 28) return;

    static uint8_t message_number = 0;
    
    uint8_t send_data[32] = {0};
    send_data[0] = DEVICE_ADDRESS;
    send_data[1] = message_number;
    send_data[2] = data_type;
    send_data[3] = data_size;
    for(int i = 0; i < data_size; i++){
        send_data[i+4] = data[i];
    }

    message_number++;

    //send out data
    nrfStopListening();
    cli();
    nrfWrite( (uint8_t *) &send_data, data_size + 4);
    sei();
    nrfStartListening();
}

//return new data if there is new data
uint8_t get_radio_data(uint8_t* data){
    if(!new_data) return 0;

    for(int i = 0; i < 32; i++) data[i] = received_packet[i];
    new_data = 0;
    
    return 1;
}

//setup for NRF communication
void radio_init(void)
{
	nrfspiInit();                                                               //initialize SPI
	nrfBegin();                                                                 //initialize NRF module
	nrfSetRetries(NRF_RETRY_SPEED, NRF_NUM_RETRIES);		                    //set retries
	nrfSetPALevel(NRF_POWER_LEVEL);									            //set power mode
	nrfSetDataRate(NRF_DATA_RATE);									            //set data rate				
	nrfSetCRCLength(NRF_CRC_LENGTH);                                            //CRC check
	nrfSetChannel(NRF_CHANNEL);													//set channel
	nrfSetAutoAck(NRF_AUTO_ACK);												//set acknowledge
	nrfEnableDynamicPayloads();													//enable the dynamic payload size
	
	nrfClearInterruptBits();													//clear interrupt bits
	nrfFlushRx();                                                               //Flush fifo's
	nrfFlushTx();

	PORTF.INT0MASK |= PIN6_bm;													//interrupt pin F0
	PORTF.PIN6CTRL = PORT_ISC_FALLING_gc;										//interrupts at falling edge
	PORTF.INTCTRL |= (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc ; 	//interrupts On
	
	// Opening pipes
	nrfOpenWritingPipe((uint8_t *) pipe);								
	nrfOpenReadingPipe(0, (uint8_t *) pipe);								
	nrfStartListening();
	nrfPowerUp();
}