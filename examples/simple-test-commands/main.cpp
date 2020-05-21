#include <assert.h>
#include <errno.h>
#include <iostream>
#include <new>
#include <sstream>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <support/CHIPLogging.h>
#include <inet/InetLayer.h>
#include <core/CHIPError.h>
#include <support/ErrorStr.h>
#include <support/CodeUtils.h>
#include <inet/UDPEndPoint.h>

#include <controller/CHIPDeviceController.h>

extern "C" {
#include "chip-zcl/chip-zcl.h"
#include "gen/gen-command-id.h"
#include "gen/gen-types.h"
#include "gen/gen-cluster-id.h"
}

using namespace ::chip;
using namespace ::chip::Inet;

// Device Manager Callbacks
static void HandleResponse(DeviceController::ChipDeviceController * deviceController, void * appReqState,
                           System::PacketBuffer * buffer, const IPPacketInfo * packet_info)
{
    char src_addr[INET_ADDRSTRLEN];
    char dest_addr[INET_ADDRSTRLEN];
    size_t data_len = buffer->DataLength();

    packet_info->SrcAddress.ToString(src_addr, sizeof(src_addr));
    packet_info->DestAddress.ToString(dest_addr, sizeof(dest_addr));

    // We're not really expecting a response here!
    fprintf(stderr,
            "Unexpected UDP packet received from %s:%u to %s:%u (%zu bytes)\n",
            src_addr, packet_info->SrcPort, dest_addr,
            packet_info->DestPort, data_len);

    System::PacketBuffer::Free(buffer);
}

static void HandleError(DeviceController::ChipDeviceController * deviceController, void * appReqState, CHIP_ERROR error,
                         const IPPacketInfo * pi)
{
    fprintf(stderr, "ERROR: %s\n Got UDP error\n", ErrorStr(error));
}

void ShowUsage(const char* executable)
{
    fprintf(stderr,
            "Usage: \n"
            "  %s device-ip-address device-port on|off|toggle\n",
            executable);
}

bool DetermineAddress(int argc, char* argv[], IPAddress* hostAddr, uint16_t* port)
{
    if (argc < 3) {
        return false;
    }

    if (!IPAddress::FromString(argv[1], *hostAddr))
    {
        fputs("Error: Invalid device IP address", stderr);
        return false;
    }

    std::string port_str(argv[2]);
    std::stringstream ss(port_str);
    ss >> *port;
    if (ss.fail() || !ss.eof())
    {
        fputs("Error: Invalid device port", stderr);
        return false;
    }

    return true;
}

enum Command {
    Off,
    On,
    Toggle,
};

bool DetermineCommand(int argc, char* argv[], Command* command) {
    if (argc < 4) {
        return false;
    }

    if (strcmp(argv[3], "off") == 0) {
        *command = Off;
        return true;
    }

    if (strcmp(argv[3], "on") == 0) {
        *command = On;
        return true;
    }

    if (strcmp(argv[3], "toggle") == 0) {
        *command = Toggle;
        return true;
    }

    fprintf(stderr, "Unknown command: %s\n", argv[3]);
    return false;
}

// =============================================================================
// Main Code
//
// Command line arguments are, in order, target IP, target port, command to send
// =============================================================================

int main(int argc, char* argv[])
{
    IPAddress host_addr;
    uint16_t port;
    Command command;
    if (!DetermineAddress(argc, argv, &host_addr, &port) ||
        !DetermineCommand(argc, argv, &command)) {
        ShowUsage(argv[0]);
        return -1;
    }

#ifdef ZCL_APS
    // A unicast ZCL command message for a standard command consists of:
    // APS header:
    //   1 byte frame control
    //   1 byte destination endpoint
    //   2 bytes cluster identifier
    //   2 bytes profile identifier
    //   1 byte source endpoint
    //   1 byte APS counter
    // ZCL Frame:
    //   1 byte frame control
    //   1 byte transaction sequence number
    //   1 byte command identifier.
    //   Command payload.
    //
    // In our case we do not have a command payload, so our message will be 11
    // bytes long.
    static const size_t payload_len = 11;
    System::PacketBuffer* buffer = System::PacketBuffer::NewWithAvailableSize(payload_len);
    uint8_t* cur = buffer->Start();

    // APS frame bits

    // This is a "data frame" from the point of view of APS, we're doing
    // unicast, we don't want an ack, and there is no security or extended
    // header involved.
    cur[0] = 0x00;
    // Just pick 1 as the destination endpoint for now.
    cur[1] = 0x01;
    // Next two bytes are the cluster identifier, in little-endian order.  We're
    // doing the on/off cluster, which has id 6.
    cur[2] = 0x06;
    cur[3] = 0x00;
    // Two byte profile identifier, in little-endian order.  Pick 0?
    cur[4] = 0x00;
    cur[5] = 0x00;
    // Just pick 1 as the source endpoint for now.
    cur[6] = 0x01;
    // APS counter.  This should increment for each new transmission, but we
    // plan to only send one packet, so not much to do there.
    cur[7] = 0x00;

    // ZCL frame bits

    // This is a cluster-specific command so low two bits are 0b01.  The command
    // is standard, so does not need a manufacturer code, and we're sending
    // client to server, so all the remaining bits are 0.
    cur[8] = 0x01;
    // Transaction sequence number.  Just pick something.
    cur[9] = 0x01;
    // Our actual command.
    switch (command) {
    case Off:
        cur[10] = 0x00;
        break;
    case On:
        cur[10] = 0x01;
        break;
    case Toggle:
        cur[10] = 0x02;
        break;
    }

    buffer->SetDataLength(payload_len);
#else
    ChipZclCommandId_t zclCommand;
    switch (command) {
    case Off:
        zclCommand = CHIP_ZCL_CLUSTER_ON_OFF_SERVER_COMMAND_OFF;
        break;
    case On:
        zclCommand = CHIP_ZCL_CLUSTER_ON_OFF_SERVER_COMMAND_ON;
        break;
    case Toggle:
        zclCommand = CHIP_ZCL_CLUSTER_ON_OFF_SERVER_COMMAND_TOGGLE;
        break;
    }
    // Make sure our buffer is big enough, but this will need a better setup!
    ChipZclRawBuffer_t* raw_buffer = chipZclBufferAlloc(1024);
    ChipZclCommandContext_t ctx = {
                                   1, // endpointId
                                   CHIP_ZCL_CLUSTER_ON_OFF, // clusterId
                                   true, // clusteSpecific
                                   false, // mfgSpecific
                                   0, // mfgCode
                                   zclCommand, // commandId
                                   ZCL_DIRECTION_CLIENT_TO_SERVER, // direction
                                   0,
                                   nullptr,
                                   nullptr
    };
    chipZclEncodeZclHeader(raw_buffer, &ctx);
    chipZclBufferFlip(raw_buffer);
    const size_t data_len = chipZclBufferUsedLength(raw_buffer);
    System::PacketBuffer* buffer = System::PacketBuffer::NewWithAvailableSize(data_len);
    fprintf(stderr, "SENDING: %zu ", data_len);
    for (size_t i = 0; i < data_len; ++i) {
        fprintf(stderr, "%d ", chipZclBufferPointer(raw_buffer)[i]);
    }
    fprintf(stderr, "\n");
    memcpy(buffer->Start(), chipZclBufferPointer(raw_buffer), data_len);
    chipZclBufferFree(raw_buffer);
    buffer->SetDataLength(data_len);
#endif

    DeviceController::ChipDeviceController * controller = new DeviceController::ChipDeviceController();
    controller->Init();

    controller->ConnectDevice(1, host_addr, NULL, HandleResponse, HandleError, port);

    controller->SendMessage(NULL, buffer);
    controller->ServiceEvents();

    return 0;
}

extern "C" {
void chipZclPostAttributeChangeCallback(uint8_t endpoint, ChipZclClusterId clusterId, ChipZclAttributeId attributeId, uint8_t mask,
                                        uint16_t manufacturerCode, uint8_t type, uint8_t size, uint8_t * value)
{
}
}
