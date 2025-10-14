#include "rtp.h"
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

// RTP header'ı başlat
void rtp_init_header(rtp_header_t *header, uint8_t payload_type, uint32_t ssrc) {
    if (!header) {
        return;
    }

    memset(header, 0, sizeof(rtp_header_t));
    header->version = RTP_VERSION;
    header->padding = 0;
    header->extension = 0;
    header->csrc_count = 0;
    header->marker = 0;
    header->payload_type = payload_type;
    header->sequence_number = 0;
    header->timestamp = 0;
    header->ssrc = ssrc;
}

// RTP paketi oluştur (Aşama 2: RTP Paketleme)
int rtp_create_packet(rtp_packet_t *packet, const uint8_t *data, size_t data_size,
                      uint16_t seq_num, uint32_t timestamp, uint8_t marker) {
    if (!packet || !data || data_size == 0 || data_size > RTP_MAX_PAYLOAD_SIZE) {
        return -1;
    }

    // Sequence number ve timestamp'i network byte order'a çevir
    packet->header.sequence_number = htons(seq_num);
    packet->header.timestamp = htonl(timestamp);
    packet->header.marker = marker;

    // Payload'ı kopyala
    memcpy(packet->payload, data, data_size);
    packet->payload_size = data_size;

    return 0;
}

// RTP header bilgilerini yazdır
void rtp_print_header(const rtp_header_t *header) {
    if (!header) {
        return;
    }

    printf("=== RTP Header ===\n");
    printf("Version:     %u\n", header->version);
    printf("Padding:     %u\n", header->padding);
    printf("Extension:   %u\n", header->extension);
    printf("CSRC Count:  %u\n", header->csrc_count);
    printf("Marker:      %u\n", header->marker);
    printf("Payload Type:%u\n", header->payload_type);
    printf("Seq Number:  %u\n", ntohs(header->sequence_number));
    printf("Timestamp:   %u\n", ntohl(header->timestamp));
    printf("SSRC:        0x%08X\n", ntohl(header->ssrc));
    printf("==================\n");
}
