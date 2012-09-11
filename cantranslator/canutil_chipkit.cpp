#include "canutil.h"
#include "signals.h"
#include "log.h"

/* Private: Initializes message filters on the CAN controller.
 *
 * canMod - a pointer to an initialized CAN module class.
 * filters - an array of filters to initialize.
 */
void configureFilters(CanBus* bus, CanFilter* filters, int filterCount) {
    debug("Configuring %d filters...", filterCount);
    for(int i = 0; i < filterCount; i++) {
        bus->bus->configureFilter((CAN::FILTER) filters[i].number,
                filters[i].value, CAN::SID);
        bus->bus->linkFilterToChannel((CAN::FILTER) filters[i].number,
                (CAN::FILTER_MASK) filters[i].maskNumber,
                (CAN::CHANNEL) filters[i].channel);
        bus->bus->enableFilter((CAN::FILTER) filters[i].number, true);
    }
    debug("Done.");
}

void initializeCan(CanBus* bus) {
    CAN::BIT_CONFIG canBitConfig;

	queue_init(&bus->sendQueue);

    // Switch the CAN module ON and switch it to Configuration mode. Wait till
    // the switch is complete
    bus->bus->enableModule(true);
    bus->bus->setOperatingMode(CAN::CONFIGURATION);
    while(bus->bus->getOperatingMode() != CAN::CONFIGURATION);

    // Configure the CAN Module Clock. The CAN::BIT_CONFIG data structure is
    // used for this purpose. The propagation, phase segment 1 and phase segment
    // 2 are configured to have 3TQ. The CANSetSpeed() function sets the baud.
    canBitConfig.phaseSeg2Tq            = CAN::BIT_3TQ;
    canBitConfig.phaseSeg1Tq            = CAN::BIT_3TQ;
    canBitConfig.propagationSegTq       = CAN::BIT_3TQ;
    canBitConfig.phaseSeg2TimeSelect    = CAN::TRUE;
    canBitConfig.sample3Time            = CAN::TRUE;
    canBitConfig.syncJumpWidth          = CAN::BIT_2TQ;
    bus->bus->setSpeed(&canBitConfig, SYS_FREQ, bus->speed);

    // Assign the buffer area to the CAN module. Note the size of each Channel
    // area. It is 2 (Channels) * 8 (Messages Buffers) 16 (bytes/per message
    // buffer) bytes. Each CAN module should have its own message area.
    bus->bus->assignMemoryBuffer(bus->buffer, BUS_MEMORY_BUFFER_SIZE);

    // Configure channel 0 for TX with 8 byte buffers and with "Remote Transmit
    // Request" disabled, meaning that other nodes can't request for us to
    // transmit data.
    bus->bus->configureChannelForTx(CAN::CHANNEL0, 8, CAN::TX_RTR_DISABLED,
            CAN::LOW_MEDIUM_PRIORITY);

    // Configure channel 1 for RX with 8 byte buffers.
    bus->bus->configureChannelForRx(CAN::CHANNEL1, 8, CAN::RX_FULL_RECEIVE);

    int filterCount;
    CanFilter* filters = initializeFilters(bus->address, &filterCount);
    configureFilters(bus, filters, filterCount);

    // Enable interrupt and events. Enable the receive channel not empty event
    // (channel event) and the receive channel event (module event). The
    // interrrupt peripheral library is used to enable the CAN interrupt to the
    // CPU.
    bus->bus->enableChannelEvent(CAN::CHANNEL1, CAN::RX_CHANNEL_NOT_EMPTY,
            true);
    bus->bus->enableModuleEvent(CAN::RX_EVENT, true);

    bus->bus->setOperatingMode(CAN::NORMAL_OPERATION);
    while(bus->bus->getOperatingMode() != CAN::NORMAL_OPERATION);

    bus->bus->attachInterrupt(bus->interruptHandler);

    debug("Done.");
}
