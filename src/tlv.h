#ifndef _TLV_H_
#define _TLV_H_

#include <stdint.h>

#include "sal.h"

#define TLV_HEADER_LENGTH 4 //Type (2) and Length(2)
#define TLV_MAX_VALUE_LENGTH (0xFFFF - TLV_HEADER_LENGTH)

typedef enum {
    TLV_TYPE_HEADER = 1,
    TLV_TYPE_FILE_NAME,
    TLV_TYPE_FILE_SIZE,
    TLV_TYPE_CHECKSUM_SHA512,
    TLV_TYPE_FILE_CONTENT,
    TLV_TYPE_ACK,
    TLV_TYPE_NACK
} tlv_type;

typedef struct Stlv {
    uint16_t type;
    uint16_t length;
    uint8_t* buffer;
    struct Stlv* sub_tlv;
    struct Stlv* next;
} tlv_t;

tlv_t new_tlv(const uint16_t type, const uint16_t length);
void tlv_release_tlvs();
bool parse_tlv(uint8_t* buffer, tlv_t* tlv);
uint16_t get_tlv_type(const tlv_t* tlv);
uint16_t get_tlv_length(const tlv_t* tlv);
void fill_tlv_length(tlv_t* tlv);
void set_sub_tlv_list(tlv_t* tlv, tlv_t* sub_tlv);
void set_next_tlv(tlv_t* tlv, tlv_t* next_tlv);
void set_tlv_value_raw(tlv_t* tlv, const uint8_t* src);
void set_tlv_value_long(tlv_t* tlv, const long value);
uint8_t* get_tlv_value_raw(tlv_t* tlv);
long get_tlv_value_long(tlv_t* tlv);

bool send_tlv_data(sal_socket_t socket, const tlv_t* tlv);
bool receive_tlv_data(sal_socket_t socket, tlv_t* tlv);

#endif /* _TLV_H_ */
