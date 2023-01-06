#include <stddef.h> //NULL
#include <arpa/inet.h> //htons
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "tlv.h"
#include "sal.h"

#define TLV_BUFFER_LEN (TLV_HEADER_LENGTH + TLV_MAX_VALUE_LENGTH)

/**
 * @brief The internal buffer for TLVs.
 **/
static uint16_t tlv_buffer_offset = 0;

/**
 * @brief Gets the internal TLV buffer.
 *
 * @returns the internal TLV buffer
 **/
static uint8_t* get_tlv_buffer() {
    static uint8_t tlv_buffer[TLV_BUFFER_LEN] = {0};
    return tlv_buffer;
}

/**
 * @brief Gets the internal TLV buffer offset.
 *
 * @returns the internal TLV buffer offset
 **/
static uint16_t get_tlv_buffer_offset() {
    return tlv_buffer_offset;
}

/**
 * @brief Increases the internal TLV buffer offset.
 *
 * @param inc The increment value
 *
 * @return No return
 **/
static void increase_sending_tlv_buffer_offset(uint16_t inc) {
    tlv_buffer_offset += inc;
}

/**
 * @brief Resets the internal TLV buffer offset.
 *
 * @return No return
 **/
static void reset_sending_tlv_buffer_offset() {
    tlv_buffer_offset = 0;
}

/**
 * @brief Gets the TLV type.
 *
 * @param tlv The given TLV
 *
 * @return The TLV type.
 **/
uint16_t get_tlv_type(const tlv_t* tlv) {
    return tlv->type;
}

/**
 * @brief Gets the TLV length.
 *
 * @param tlv The given TLV
 *
 * @return The TLV length (length of Value field).
 **/
uint16_t get_tlv_length(const tlv_t* tlv) {
    return tlv->length;
}

/**
 * @brief Set the TLV header on internal buffer and increment its offset.
 *
 * @param type The TLV type
 * @param length The TLV length
 *
 * @return The new TLV.
 **/
tlv_t new_tlv(const uint16_t type, const uint16_t length) {
    uint16_t offset = get_tlv_buffer_offset();
    uint8_t* buffer = get_tlv_buffer() + offset;
    buffer[0] = (type >> 8) & 0xFF;
    buffer[1] = (type >> 0) & 0xFF;
    buffer[2] = (length >> 8) & 0xFF;
    buffer[3] = (length >> 0) & 0xFF;
    buffer += TLV_HEADER_LENGTH;
    tlv_t tlv = {
        .type = type,
        .length = length,
        .buffer = buffer,
        .next = NULL,
        .sub_tlv = NULL
    };
    increase_sending_tlv_buffer_offset(TLV_HEADER_LENGTH + length);
    return tlv;
}

/**
 * @brief Parses a TLV from a given buffer.
 *
 * @param buffer The buffer to be parsed
 * @param[out] tlv The parsed TLV
 *
 * @return The new TLV.
 **/
bool parse_tlv(uint8_t* buffer, tlv_t* tlv) {
    tlv->type = ((uint16_t)buffer[0] << 8) + buffer[1];
    tlv->length = ((uint16_t)buffer[2] << 8) + buffer[3];
    tlv->buffer = buffer + TLV_HEADER_LENGTH;
    tlv->next = NULL;
    tlv->sub_tlv = NULL;
    return true;
}

/**
 * @brief Resets the TLV internal buffer and offset.
 *
 * @return No return
 **/
void tlv_release_tlvs() {
    reset_sending_tlv_buffer_offset();
    bzero(get_tlv_buffer(), TLV_BUFFER_LEN);
}

/**
 * @brief Sets the TLV length based on its sub-TLV list.
 *
 * @param[inout] tlv The given TLV
 *
 * @return No return
 **/
void fill_tlv_length(tlv_t* tlv) {
    assert(tlv);
    uint16_t length = 0;
    tlv_t* sub_tlv = tlv->sub_tlv;
    while (sub_tlv) {
        length += TLV_HEADER_LENGTH + get_tlv_length(sub_tlv);
        sub_tlv = sub_tlv->next;
    }
    tlv->length = length;
    *(tlv->buffer - 2) = (length >> 8) & 0xFF;
    *(tlv->buffer - 1) = (length >> 0) & 0xFF;
}

/**
 * @brief Sets the sub-TLV list on a parent TLV.
 *
 * @param[inout] tlv The parent TLV
 * @param[in] sub_tlv The given sub-TLV list
 *
 * @return No return
 **/
void set_sub_tlv_list(tlv_t* tlv, tlv_t* sub_tlv) {
    assert(tlv && get_tlv_length(tlv) == 0);
    tlv->sub_tlv = sub_tlv;
    fill_tlv_length(tlv);
}

/**
 * @brief Sets the sibling TLV for a given TLV.
 *
 * @param[inout] tlv The parent TLV
 * @param[in] sub_tlv The given sibling TLV
 *
 * @return No return
 **/
void set_next_tlv(tlv_t* tlv, tlv_t* next_tlv) {
    tlv->next = next_tlv;
}

/**
 * @brief Sets the TLV value buffer. The amount of copied bytes is defined by
 * TLV's length.
 *
 * @param tlv The given TLV
 * @param src The source data
 *
 * @return No return
 **/
void set_tlv_value_raw(tlv_t* tlv, const uint8_t* src) {
    assert(tlv && get_tlv_length(tlv));
    memcpy(tlv->buffer, src, get_tlv_length(tlv));
}

/**
 * @brief Sets the TLV value buffer from a long.
 *
 * @param tlv The given TLV
 * @param value The source value
 *
 * @return No return
 **/
void set_tlv_value_long(tlv_t* tlv, const long value) {
    assert(tlv && get_tlv_length(tlv) == sizeof(value));
    for (int i = 0; i < sizeof(value); ++i) {
        tlv->buffer[i] = (value >> ((sizeof(value) - i - 1) * 8)) & 0xFF;
    }
}

/**
 * @brief Gets the pointer to the TLV value buffer.
 *
 * @param tlv The given TLV
 *
 * @return pointer to TLV buffer
 **/
uint8_t* get_tlv_value_raw(tlv_t* tlv) {
    assert(tlv);
    return tlv->buffer;
}

/**
 * @brief Gets the value encoded on the TLV buffer.
 *
 * @param tlv The given TLV
 *
 * @return the value encoded on TLV buffer
 **/
long get_tlv_value_long(tlv_t* tlv) {
    long value = 0;
    assert(tlv && get_tlv_length(tlv) <= sizeof(value));
    for (int i = 0; i < sizeof(value); ++i) {
        value += (long)tlv->buffer[i] << ((sizeof(value) - i - 1) * 8);
    }
    return value;
}

/**
 * @brief Sends the TLV data though the given socket.
 *
 * @param socket The socket to be used
 * @param tlv The given TLV
 *
 * @return true if data was sent successfully
 * @return false otherwise
 **/
bool send_tlv_data(sal_socket_t socket, const tlv_t* tlv) {
    return sal_send_msg(
        socket,
        tlv->buffer - TLV_HEADER_LENGTH,
        TLV_HEADER_LENGTH + (int)get_tlv_length(tlv)) == SAL_OK;
}

/**
 * @brief Fills the TLV data from the given socket.
 *
 * @param socket The socket to be used
 * @param[out] tlv The given TLV
 *
 * @return true if TLV was retrieved successfully
 * @return false otherwise
 **/
bool receive_tlv_data(sal_socket_t socket, tlv_t* tlv) {
    uint8_t header_buffer[TLV_HEADER_LENGTH] = {0};
    if (sal_receive_msg(socket, header_buffer, sizeof(header_buffer)) != SAL_OK) {
        return false;
    }
    uint16_t type = ((uint16_t)header_buffer[0] << 8) + header_buffer[1];
    uint16_t length = ((uint16_t)header_buffer[2] << 8) + header_buffer[3];
    *tlv = new_tlv(type, length);
    if (sal_receive_msg(socket, tlv->buffer, length) != SAL_OK) {
        return false;
    }
    return true;
}
