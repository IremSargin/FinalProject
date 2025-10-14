#ifndef RTP_H
#define RTP_H

#include <stdint.h>
#include <stddef.h>

#define RTP_VERSION 2
#define RTP_MAX_PAYLOAD_SIZE 1400  // MTU'ya uygun boyut

// RTP Header yapısı (RFC 3550)
typedef struct {
    uint8_t version:2;        // Version (2 bit) - V
    uint8_t padding:1;        // Padding (1 bit) - P
    uint8_t extension:1;      // Extension (1 bit) - X
    uint8_t csrc_count:4;     // CSRC count (4 bit) - CC

    uint8_t marker:1;         // Marker (1 bit) - M
    uint8_t payload_type:7;   // Payload type (7 bit) - PT

    uint16_t sequence_number; // Sequence number (16 bit)
    uint32_t timestamp;       // Timestamp (32 bit)
    uint32_t ssrc;            // SSRC identifier (32 bit)
} __attribute__((packed)) rtp_header_t;

// RTP Paketi
typedef struct {
    rtp_header_t header;
    uint8_t payload[RTP_MAX_PAYLOAD_SIZE];
    size_t payload_size;
} rtp_packet_t;

// RTP fonksiyonları
void rtp_init_header(rtp_header_t *header, uint8_t payload_type, uint32_t ssrc);
int rtp_create_packet(rtp_packet_t *packet, const uint8_t *data, size_t data_size,
                      uint16_t seq_num, uint32_t timestamp, uint8_t marker);
void rtp_print_header(const rtp_header_t *header);

#endif // RTP_H
