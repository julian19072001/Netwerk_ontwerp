/*Julian Della Guardia*/

#include <avr/interrupt.h>
#include <string.h>
#include "mesh_radio.h"

uint8_t nRF_pipe[5] = NRF_PIPE;

// Global variables
Package packages[MAX_PACKAGES];  // Array to store packages
volatile uint8_t address = 0;

// Define a struct with bit-fields
typedef struct {
    unsigned int index : 4;  // 4 bits for the lower nibble
    unsigned int total : 4; // 4 bits for the higher nibble
} IndexSplit;

//interrupt from NFR IC
ISR(PORTF_INT0_vect)
{
	uint8_t tx_ds, max_rt, rx_dr;
	uint8_t packet_length;
    uint8_t received_packet[32] = {0};				//create a place to store received data
	
	nrfWhatHappened(&tx_ds, &max_rt, &rx_dr);

	if ( rx_dr ) {
		packet_length = nrfGetDynamicPayloadSize();	 //get the size of the received data
		nrfRead(received_packet, packet_length);	 //store the received data

        for (int i = 0; i < MAX_PACKAGES; i++) {
            if (packages[i].in_use && packages[i].id == received_packet[0]) {
                // If package exists, just update the payload and weight
                packages[i].target_id = received_packet[1];

                // Use memcpy to copy the entire payload at once
                memcpy(packages[i].payload, &received_packet[2], MAX_DATA_LENGTH);  // Copy the payload
                packages[i].unread_data = 1;
                packages[i].weight += 10;  // Increment weight by 10
                if(packages[i].weight > 100) packages[i].weight = 100;
                return;  // Done, no need to continue
            }
            if (!packages[i].in_use) {
                // If an empty slot is found, store the new package
                packages[i].id = received_packet[0];
                packages[i].target_id = received_packet[1];

                // Use memcpy to copy the entire payload at once
                memcpy(packages[i].payload, &received_packet[2], MAX_DATA_LENGTH);  // Copy the payload
                packages[i].unread_data = 1;
                packages[i].weight = 10;    // Initial weight
                packages[i].in_use = 1;     // Mark as in use
                packages[i].owner = 0;      // Set closted device as itself
                packages[i].hops  = 1;      // Set number of hops to one
                return;
            }
        }
	}
}

//setup for NRF communication
void radio_init(uint8_t set_address)
{   
    //timer for lowering weights
    TCC0.CTRLB = TC_WGMODE_NORMAL_gc;                                           //Normal mode
    TCC0.CTRLA = TC_CLKSEL_DIV256_gc;                                            //Devide clock by 1024 so the periode isnt so big
    TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc;                                         // enable overflow interrupt low level
    TCC0.PER = (125 * WEIGHT_LOWER_TIME);                                                           //Set lowering weight time

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
	nrfOpenWritingPipe((uint8_t *) nRF_pipe);								
	nrfOpenReadingPipe(0, (uint8_t *) nRF_pipe);								
	nrfStartListening();
	nrfPowerUp();

    address = set_address;                                                      //set device address

    for (int i = 0; i < MAX_PACKAGES; i++) {
        packages[i].in_use = 0;
    }
}

void send_radio_data(uint8_t command, uint8_t target_id, uint8_t* data, uint8_t data_size){
    
    if(data_size > MAX_DATA_LENGTH) return;
    
    uint8_t send_data[32] = {0};
    send_data[0] = address;
    send_data[1] = command;
    
    if(command == 3){
        Package* package = get_radio_data_by_id(target_id);
        if(package == NULL) return;

        if(!package->owner) send_data[2] = target_id;
        else send_data[2] = package->owner;
        send_data[3] = target_id;

        for(int i = 0; i < data_size; i++){
            send_data[i+4] = data[i];
        }
    }
    else{
        send_data[2] = target_id;
        for(int i = 0; i < data_size; i++){
            send_data[i+3] = data[i];
        }
    }

    //send out data
    nrfStopListening();
    cli();
    nrfWrite( (uint8_t *) &send_data, data_size + 4);
    sei();
    nrfStartListening();
}

// Function to retrieve the package struct by ID, based on the conditions
Package* get_radio_data_by_id(uint8_t id) {
    cli();
    // Iterate through all packages
    for (int i = 0; i < MAX_PACKAGES; i++) {
        // Check if the package is in use and the ID matches
        if (packages[i].in_use && packages[i].target_id == id) {
            // Check if the package is trusted and has unread data
            if (packages[i].trusted && packages[i].unread_data) {
                // Return the package struct if all conditions are met
                packages[i].unread_data = 0;
                return &packages[i];
            } 
            else {
                return NULL;  // Return NULL if conditions are not met
            }
        }
    }
    sei();
    
    // Return NULL if the ID doesn't exist in the packages
    return NULL;
}

// Function to print all packages
void print_packages() {
    for (int i = 0; i < MAX_PACKAGES; i++) {
        if (packages[i].in_use) {
            printf("ID: %d, Payload: %s, Weight: %d, Trusted: %d, Target id: %02X\n", 
                   packages[i].id, packages[i].payload, 
                   packages[i].weight, packages[i].trusted,
                   packages[i].target_id); 
        }
    }
}




//interrupt from timer counter for weight lowering
ISR(TCC0_OVF_vect)
{
    for (int i = 0; i < MAX_PACKAGES; i++) {
        if (packages[i].in_use && !packages[i].owner) {
            // Decrease weight
            if (packages[i].weight > 0) packages[i].weight--;

            // Remove package if weight is 0
            if (packages[i].weight == 0) packages[i].in_use = 0; // Mark as unused

            // Check if weight exceeds threshold for trusted status
            else if (packages[i].weight > WEIGHT_THRESHOLD) packages[i].trusted = 1; // Mark as trusted
            
            else packages[i].trusted = 0; // Reset trusted flag if below threshold
        }
    }
    sendNextPing();
}

// Snapshot structure
typedef struct {
    uint8_t *messages[MAX_PACKAGES]; // Array of pointers to messages
    size_t message_lengths[MAX_PACKAGES]; // Length of each message
    size_t total_messages; // Total number of messages
    size_t current_message; // Index of the next message to send
} Snapshot;

Snapshot snapshot = { .total_messages = 0, .current_message = 0 };

// Create snapshot from packages for neighbord table
void createSnapshot(Package packages[]) {
    snapshot.total_messages = 0;
    snapshot.current_message = 0;

    uint8_t buffer[MAX_DATA_LENGTH]; // Temporary buffer
    size_t buffer_index = 0;

    for (size_t i = 0; i < MAX_PACKAGES; i++) {
        if (packages[i].in_use && packages[i].trusted) {
            // Add id and hops to the buffer
            buffer[buffer_index++] = packages[i].id;
            buffer[buffer_index++] = packages[i].hops;

            // If buffer is full, store the message
            if (buffer_index >= MAX_DATA_LENGTH) {
                snapshot.messages[snapshot.total_messages] = malloc(buffer_index);
                memcpy(snapshot.messages[snapshot.total_messages], buffer, buffer_index);
                snapshot.message_lengths[snapshot.total_messages] = buffer_index;
                snapshot.total_messages++;
                buffer_index = 0;
            }
        }
    }

    // Store any remaining data in the buffer
    if (buffer_index > 0) {
        snapshot.messages[snapshot.total_messages] = malloc(buffer_index);
        memcpy(snapshot.messages[snapshot.total_messages], buffer, buffer_index);
        snapshot.message_lengths[snapshot.total_messages] = buffer_index;
        snapshot.total_messages++;
    }
}

// Send the next message in the snapshot with the index and total number in a separate variable
void sendNextPing() {
    if (snapshot.current_message >= snapshot.total_messages) createSnapshot(packages);

    if (snapshot.total_messages > 0) {
        uint8_t *message = snapshot.messages[snapshot.current_message];
        size_t message_length = snapshot.message_lengths[snapshot.current_message];

        // Pack the index and total number into a separate index byte
        uint8_t index_byte = (snapshot.current_message << 4) | (snapshot.total_messages & 0x0F);

        // Decide what command needs to send
        uint8_t command = 0;
        if(snapshot.current_message+1 == snapshot.total_messages) command = 2;
        else command = 1;

        // Send the message data
        send_radio_data(command, index_byte, message, message_length);
        snapshot.current_message++;
    }
}