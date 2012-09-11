#include "canwrite.h"
#include "log.h"

/* Private: Write a CAN message with the given data and node ID to the bus.
 *
 * The CAN module has an 8 message buffer and sends messages in FIFO order. If
 * the buffer is full, this function will return false and the message will not
 * be sent.
 *
 * bus - The CAN bus to send the message on.
 * request - the CanMessage requested to send.
 *
 * Returns true if the message was sent successfully.
 */
bool sendCanMessage(CanBus* bus, CanMessage request) {
    CAN::TxMessageBuffer* message = bus->bus->getTxMessageBuffer(CAN::CHANNEL0);
    if (message != NULL) {
        message->messageWord[0] = 0;
        message->messageWord[1] = 0;
        message->messageWord[2] = 0;
        message->messageWord[3] = 0;

        message->msgSID.SID = request.destination;
        message->msgEID.IDE = 0;
        message->msgEID.DLC = 8;
        memset(message->data, 0, 8);
        for(int i = 0; i < 8; i++) {
            memcpy(&message->data[i], &((uint8_t*)request.data)[7 - i], 8);
        }
        debug("Sending message 0x");
        for(int i = 0; i < 8; i++) {
            debug("%x", message->data[i]);
        }
        debug(" to 0x%X", request.destination);

        // Mark message as ready to be processed
        bus->bus->updateChannel(CAN::CHANNEL0);
        bus->bus->flushTxChannel(CAN::CHANNEL0);
        return true;
    } else {
        debug("Unable to get TX message area");
    }
    return false;
}

void processCanWriteQueue(CanBus* bus) {
    while(!queue_empty(&bus->sendQueue)) {
        sendCanMessage(bus, QUEUE_POP(CanMessage, &bus->sendQueue));
    }
}