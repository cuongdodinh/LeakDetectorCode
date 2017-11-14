#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define BRIDGE_ADDRESS     0x00

#define PACKET_TYPE_ACK                     1
#define PACKET_TYPE_ALARM                   2
#define PACKET_TYPE_ALIVE                   3
#define PACKET_TYPE_SETTINGS                4
#define PACKET_TYPE_ERROR                   5
#define PACKET_TYPE_REGISTER_NODE           6
#define PACKET_TYPE_REBOOT_REPORT           7

#define PACKET_OFFSET_SRC_ADDR             0
#define PACKET_OFFSET_SEQ_NUMBER           1
#define PACKET_OFFSET_PACKET_TYPE          2
#define PACKET_OFFSET_PAYLOAD              3
#define MIN_PACKET_LEN                     PACKET_OFFSET_PAYLOAD
#define BRIDGE_PACKET_OFFSET_PACKET_TYPE   MIN_PACKET_LEN
#define BRIDGE_PACKET_OFFSET_PAYLOAD       BRIDGE_PACKET_OFFSET_PACKET_TYPE + 1

#define PACKET_TYPE_SETTINGS_LEN           MIN_PACKET_LEN + 10
#define PACKET_TYPE_REGISTER_NODE_LEN      MIN_PACKET_LEN + 10

#endif /* PROTOCOL_H_ */
