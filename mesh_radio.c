/*Julian Della Guardia*/

#include <avr/interrupt.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "mesh_radio.h"

void updateWeight(uint8_t address);
uint8_t checkTrusted(uint8_t address);
void saveRemoteNeighborTable(uint8_t address, uint8_t *neighborData, uint8_t dataLength, uint8_t packageIndex);
void sendNextPing();

uint8_t nRF_pipe[5] = NRF_PIPE;

// Structure to store neighbor data
typedef struct {
    uint8_t id;                         // ID (first byte of the data)
    int weight;                         // Weight of the package
    bool inUse;                        // Flag to indicate if the package slot is in use
    bool trusted;                       // Flag to indicate if the package is trusted
    uint8_t owner;                      // Set from what route the ID is the closest
    uint8_t hops;                       // Number of hops to the receiver
} Package;

// Global variables
Package packages[MAX_SENDERS];  // Array to store packages
volatile uint8_t address = 0;

// Setup for NRF communication
void radioInit(uint8_t setAddress)
{   
    // Timer for lowering weights
    TCC0.CTRLB = TC_WGMODE_NORMAL_gc;                                           // Normal mode
    TCC0.CTRLA = TC_CLKSEL_DIV256_gc;                                           // Devide clock by 1024 so the periode isnt so big
    TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc;                                         // Enable overflow interrupt low level
    TCC0.PER = (125 * WEIGHT_LOWER_TIME);                                       // Set lowering weight time

	nrfspiInit();                                                               // initialize SPI
	nrfBegin();                                                                 // initialize NRF module
	nrfSetRetries(NRF_RETRY_SPEED, NRF_NUM_RETRIES);		                    // set retries
	nrfSetPALevel(NRF_POWER_LEVEL);									            // set power mode
	nrfSetDataRate(NRF_DATA_RATE);									            // set data rate				
	nrfSetCRCLength(NRF_CRC_LENGTH);                                            // CRC check
	nrfSetChannel(NRF_CHANNEL);													// set channel
	nrfSetAutoAck(NRF_AUTO_ACK);												// set acknowledge
	nrfEnableDynamicPayloads();													// enable the dynamic payload size
	
	nrfClearInterruptBits();													// clear interrupt bits
	nrfFlushRx();                                                               // Flush fifo's
	nrfFlushTx();

	PORTF.INT0MASK |= PIN6_bm;													// interrupt pin F0
	PORTF.PIN6CTRL = PORT_ISC_FALLING_gc;										// interrupts at falling edge
	PORTF.INTCTRL |= (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc ; 	// interrupts On
	
	// Opening pipes
	nrfOpenWritingPipe((uint8_t *) nRF_pipe);								
	nrfOpenReadingPipe(0, (uint8_t *) nRF_pipe);								
	nrfStartListening();
	nrfPowerUp();

    address = setAddress;                                                      //set device address

    for (int i = 0; i < MAX_SENDERS; i++) {
        packages[i].inUse = 0;
    }
}

void sendDataOverRadio(uint8_t *data, uint8_t dataSize) {
    nrfStopListening();
    cli();
    nrfWrite((uint8_t *) data, dataSize);
    sei();
    nrfStartListening();
}

void sendRadioData(uint8_t target_id, uint8_t* data, uint8_t dataSize){
    
    if(dataSize > MAX_DATA_LENGTH) return;
    
    uint8_t send_data[32] = {0};
    send_data[0] = address;
    send_data[1] = COMMAND_DATA;

    for (int i = 0; i < MAX_SENDERS; i++) {
        // Find what owner the target ID has
        if (packages[i].inUse && packages[i].id == target_id) {
            if(!packages[i].owner) send_data[2] == target_id;
            else send_data[2] == packages[i].owner;
        }
        else return; // If there is no route dont send anything
    }

    // Save data to be send
    for(int i = 0; i < dataSize; i++){
        send_data[i+4] = data[i];
    }
    
    // Send out data
    nrfStopListening();
    cli();
    nrfWrite( (uint8_t *) &send_data, dataSize + 4);
    sei();
    nrfStartListening();
}


// Function to print all packages
void printPackages() {
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (packages[i].inUse) {
            printf("ID: %d, hops: %d, Weight: %d, Trusted: %d, Owner: %d\n", 
                   packages[i].id, packages[i].hops,  
                   packages[i].weight, packages[i].trusted,
                   packages[i].owner); 
        }
    }
}




//##### Start of code for receiving packages #############################################################################################################

// Interrupt from NFR IC
ISR(PORTF_INT0_vect)
{
	uint8_t txDs, maxRt, rxDr;
	uint8_t packetLength;
    uint8_t received_packet[32] = {0};				    // Create a place to store received data
	
	nrfWhatHappened(&txDs, &maxRt, &rxDr);

	if ( rxDr ) {
		packetLength = nrfGetDynamicPayloadSize();	    // Get the size of the received data
		nrfRead(received_packet, packetLength);	        // Store the received data

        cli();

        updateWeight(received_packet[0]);
        if(!checkTrusted(received_packet[0])) return;

        // Check what command has been send with data
        switch (received_packet[1]) {
        case COMMAND_PING:
        case COMMAND_PING_END:
            saveRemoteNeighborTable(received_packet[0], &received_packet[3], packetLength - 2, received_packet[2]);
            break;
        
        default:
            break;
        }

        sei();
	}
}

// Updata weights of direct neighbors
void updateWeight(uint8_t updateAddress){
    // Look if package id is already stored and if so update weight
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (packages[i].inUse && packages[i].id == updateAddress) {
            // If package exists, just update the weight
            packages[i].weight += 10;  // Increment weight by 10
            packages[i].owner = 0;      // Set closted device as itself
            packages[i].hops  = 1;      // Set number of hops to one
            if(packages[i].weight > WEIGHT_MAX) packages[i].weight = WEIGHT_MAX;
            return;  // Done, no need to continue
        }
    }

    // If the package isnt in the neighbor table yet put it in.
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (!packages[i].inUse) {
            // If an empty slot is found, store the new package
            packages[i].id = updateAddress;
            packages[i].weight = 10;    // Initial weight
            packages[i].inUse = 1;     // Mark as in use
            packages[i].owner = 0;      // Set closted device as itself
            packages[i].hops  = 1;      // Set number of hops to one
            return;
        }
    }
}

// Check if address is in trusted list
uint8_t checkTrusted(uint8_t checkAddress){
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (packages[i].id == checkAddress) {
            if(packages[i].trusted) return 1;
            return 0;
        }
    }

    return 0;
}
//##### End of code for receiving packages ################################################################################################################



//##### Start of code for receiving neighbor tables #######################################################################################################

// Structure to store the sender's state
typedef struct {
    uint8_t currentMessageNumber;
    uint8_t totalMessageCount;
    uint8_t data[MAX_DATA_LENGTH];
    uint16_t dataLength;
} SenderState;

// Array to store sender states
SenderState senders[MAX_SENDERS];

// Slot occupancy array for managing sender slots
bool slotOccupied[MAX_SENDERS];

// Array to map addresses to senders
uint8_t addressMapping[MAX_SENDERS];

// Function to reset the sender's state
void resetSenderState(SenderState *sender) {
    sender->currentMessageNumber = 0;
    sender->totalMessageCount = 0;
    sender->dataLength = 0;
}

// Function to drop a sender's address from the mapping and reset its state
void dropSender(uint8_t dropAddress) {
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (slotOccupied[i] && addressMapping[i] == dropAddress) {
            // Reset the sender's state
            resetSenderState(&senders[i]);
            // Mark the slot as unoccupied
            slotOccupied[i] = false;
            return;
        }
    }
}

// Function to process the completed data
void processData(uint8_t *data, uint16_t dataLength, uint8_t owner) {
    // Remove old values from table
    for (int i = 0; i < MAX_SENDERS; i++) {
        if(packages[i].inUse && packages[i].owner == owner) {
            packages[i].inUse = 0;
        }
    }
    
    // Parse the data into own neighbor table
    for (uint16_t i = 0; i < dataLength; i += 2) {
        uint8_t id = data[i];
        uint8_t hops = data[i + 1];

        if (id == 0x00) break; // End of valid data, stop parsing
        if (id == address) break;

        // Store the ID and hops pair
        for (int i = 0; i < MAX_SENDERS; i++) {
            if (packages[i].inUse && packages[i].id == id) {
                // If package exists, just update the hops if the hops are lower
                if(packages[i].hops > hops) {
                    packages[i].owner = owner;
                    packages[i].hops = hops + 1;
                }
                return;  // Done, no need to continue
            }
        }

        // If the package isnt in the neighbor table yet put it in.
        for (int i = 0; i < MAX_SENDERS; i++) {
            if (!packages[i].inUse) {
                // If an empty slot is found, store the new package
                packages[i].id = id;
                packages[i].inUse = 1;              // Mark as in use
                packages[i].trusted = 1;            // Set trusted as true since that should be handeld by sender
                packages[i].owner = owner;          // Set closted device as itself
                packages[i].hops  = hops + 1;       // Set number of hops to one
                return;
            }
        }
    }
}

// Function to handle incoming packages
void saveRemoteNeighborTable(uint8_t address, uint8_t *neighborData, uint8_t dataLength, uint8_t packageIndex) {
    int senderIndex = -1;

    // Search for the sender's address in the address mapping
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (slotOccupied[i] && addressMapping[i] == address) {
            senderIndex = i;
            break;
        }
    }

    if (senderIndex == -1) {
        // New sender, find an empty slot
        for (int i = 0; i < MAX_SENDERS; i++) {
            if (!slotOccupied[i]) {
                addressMapping[i] = address;
                senderIndex = i;
                slotOccupied[i] = true;
                resetSenderState(&senders[i]);
                break;
            }
        }
    }

    if (senderIndex == -1) return; // No available space for new senders

    SenderState *sender = &senders[senderIndex];
    uint8_t currentMessageNumber = packageIndex >> 4; // First 4 bits are message number
    uint8_t totalMessageCount = packageIndex & 0x0F; // Last 4 bits are total message count

    // Check if this is the first package, and if so, initialize sender state
    if (sender->currentMessageNumber == 0) {
        sender->currentMessageNumber = currentMessageNumber;
        sender->totalMessageCount = totalMessageCount;
    }

    // If the package number or total count doesn't match, ignore it
    if (currentMessageNumber != sender->currentMessageNumber || totalMessageCount != sender->totalMessageCount) return;

    // Add the data to the sender's state
    for (uint8_t i = 0; i < dataLength; i++) {
        if (sender->dataLength < MAX_DATA_LENGTH) {
            sender->data[sender->dataLength++] = neighborData[i];
        }
    }

    // Check if we have received all expected packages
    if (sender->dataLength >= MAX_DATA_LENGTH) {
        // Process completed data once the package is full
        processData(sender->data, sender->dataLength, address);

        // Reset the sender's state after processing
        resetSenderState(sender);
    }
}
//##### End of code for receiving neighbor tables #######################################################################################################



//##### Start of code for lowering weights and pings ######################################################################################################

// Interrupt from timer counter for weight lowering and sending pings
ISR(TCC0_OVF_vect)
{
    for (int i = 0; i < MAX_SENDERS; i++) {
        if (packages[i].inUse && !packages[i].owner) {
            // Decrease weight
            if (packages[i].weight > 0) packages[i].weight--;

            // Remove package if weight is 0
            if (packages[i].weight == 0) {
                packages[i].inUse = 0; // Mark as unused
                dropSender(packages[i].id);    // Drop ID from the neighbor table handling list
            }

            // Check if weight exceeds threshold for trusted status
            else if (packages[i].weight > WEIGHT_THRESHOLD) packages[i].trusted = 1; // Mark as trusted
            
            else {
                packages[i].trusted = 0; // Reset trusted flag if below threshold
            
                // Remove all childeren if sender isnt trusted
                for (int j = 0; j < MAX_SENDERS; j++) {
                    if (packages[j].inUse && packages[j].owner == packages[i].id) {
                        packages[j].inUse = 0;
                    }
                }
            }
        }
    }
    sendNextPing();
}

// Snapshot structure
typedef struct {
    uint8_t *messages[MAX_SENDERS]; // Array of pointers to messages
    size_t messageLengths[MAX_SENDERS]; // Length of each message
    size_t totalMessages; // Total number of messages
    size_t currentMessage; // Index of the next message to send
} Snapshot;

Snapshot snapshot = { .totalMessages = 0, .currentMessage = 0 };

// Create snapshot from packages for neighbord table
void createSnapshot(Package packages[]) {
    snapshot.totalMessages = 0;
    snapshot.currentMessage = 0;

    uint8_t buffer[MAX_DATA_LENGTH]; // Temporary buffer
    size_t bufferIndex = 0;

    for (size_t i = 0; i < MAX_SENDERS; i++) {
        if (packages[i].inUse && packages[i].trusted) {
            // Add id and hops to the buffer
            buffer[bufferIndex++] = packages[i].id;
            buffer[bufferIndex++] = packages[i].hops;

            // If buffer is full, store the message
            if (bufferIndex >= MAX_DATA_LENGTH) {
                snapshot.messages[snapshot.totalMessages] = malloc(bufferIndex);
                memcpy(snapshot.messages[snapshot.totalMessages], buffer, bufferIndex);
                snapshot.messageLengths[snapshot.totalMessages] = bufferIndex;
                snapshot.totalMessages++;
                bufferIndex = 0;
            }
        }
    }

    // Store any remaining data in the buffer
    if (bufferIndex > 0) {
        snapshot.messages[snapshot.totalMessages] = malloc(bufferIndex);
        memcpy(snapshot.messages[snapshot.totalMessages], buffer, bufferIndex);
        snapshot.messageLengths[snapshot.totalMessages] = bufferIndex;
        snapshot.totalMessages++;
    }
}

// Send the next message in the snapshot with the index and total number in a separate variable
void sendNextPing() {
    if (snapshot.currentMessage >= snapshot.totalMessages) createSnapshot(packages);

    if (snapshot.totalMessages > 0) {
        uint8_t *message = snapshot.messages[snapshot.currentMessage];
        size_t messageLength = snapshot.messageLengths[snapshot.currentMessage];

        // Pack the index and total number into a separate index byte
        uint8_t indexByte = (snapshot.currentMessage << 4) | (snapshot.totalMessages & 0x0F);

        // Decide what command needs to send
        uint8_t command = 0;
        if(snapshot.currentMessage + 1 == snapshot.totalMessages) command = COMMAND_PING_END;
        else command = COMMAND_PING;

        // Send the message data
        uint8_t sendData[32] = {0};
        sendData[0] = address;
        sendData[1] = command;
        sendData[2] = indexByte;
        for(int i = 0; i < messageLength; i++){
            sendData[i+3] = message[i];
        }

        // Send out data
        nrfStopListening();
        cli();
        nrfWrite( (uint8_t *) &sendData, messageLength + 4);
        sei();
        nrfStartListening();

        snapshot.currentMessage++;
    }
    // If there are no neighbors just send empty ping
    else{
        uint8_t sendData[32] = {0};
        sendData[0] = address;
        sendData[1] = COMMAND_PING_END;
        sendData[2] = 0x01;

        // Send out data
        nrfStopListening();
        cli();
        nrfWrite( (uint8_t *) &sendData, 4);
        sei();
        nrfStartListening();
    }
}
//##### end of code for lowering weights and pings ######################################################################################################