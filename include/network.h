#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>

// UDP Header yapısı (RFC 768)
typedef struct {
    uint16_t src_port;      // Source port (16 bit)
    uint16_t dst_port;      // Destination port (16 bit)
    uint16_t length;        // Length (16 bit) - header + data
    uint16_t checksum;      // Checksum (16 bit)
} __attribute__((packed)) udp_header_t;

// IP Header yapısı (RFC 791 - IPv4, simplified)
typedef struct {
    uint8_t version_ihl;    // Version (4 bit) + IHL (4 bit)
    uint8_t tos;            // Type of Service (8 bit)
    uint16_t total_length;  // Total Length (16 bit)
    uint16_t identification;// Identification (16 bit)
    uint16_t flags_offset;  // Flags (3 bit) + Fragment Offset (13 bit)
    uint8_t ttl;            // Time to Live (8 bit)
    uint8_t protocol;       // Protocol (8 bit) - 17 for UDP
    uint16_t checksum;      // Header Checksum (16 bit)
    uint32_t src_ip;        // Source IP Address (32 bit)
    uint32_t dst_ip;        // Destination IP Address (32 bit)
} __attribute__((packed)) ip_header_t;

// Tam paket yapısı (IP + UDP + RTP + payload)
typedef struct {
    ip_header_t ip_header;
    udp_header_t udp_header;
    uint8_t *rtp_data;      // RTP paketi (header + payload)
    size_t rtp_data_size;
} network_packet_t;

// Network fonksiyonları
void udp_init_header(udp_header_t *header, uint16_t src_port, uint16_t dst_port, uint16_t data_len);
void ip_init_header(ip_header_t *header, uint32_t src_ip, uint32_t dst_ip, uint16_t total_len);
uint16_t calculate_checksum(const void *data, size_t len);
int create_network_packet(network_packet_t *packet, const uint8_t *rtp_data, size_t rtp_size,
                         uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port);
void print_network_packet(const network_packet_t *packet);

#endif // NETWORK_H
