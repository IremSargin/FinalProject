#ifndef RECEIVER_H
#define RECEIVER_H

#include <stdint.h>
#include <stddef.h>
#include "rtp.h"
#include "network.h"

#define MAX_PACKETS 1000
#define RECV_BUFFER_SIZE 2048

// Alınan paket bilgisi
typedef struct {
    uint16_t seq_num;           // RTP sequence number
    uint32_t timestamp;         // RTP timestamp
    uint8_t payload[RTP_MAX_PAYLOAD_SIZE];
    size_t payload_size;
    int received;               // Bu paket alındı mı?
} received_packet_t;

// Receiver durumu
typedef struct {
    int sockfd;                 // UDP socket
    uint16_t listen_port;       // Dinleme portu

    // Paket buffer'ı
    received_packet_t packets[MAX_PACKETS];
    int packet_count;

    // İstatistikler
    uint32_t total_packets_received;
    uint32_t duplicate_packets;
    uint32_t out_of_order_packets;
    size_t total_bytes_received;
} receiver_state_t;

// Receiver fonksiyonları
int receiver_init(receiver_state_t *state, uint16_t port);
void receiver_cleanup(receiver_state_t *state);

int receiver_listen(receiver_state_t *state, int timeout_sec);
int receiver_parse_packet(receiver_state_t *state, const uint8_t *buffer, size_t buffer_size);

int receiver_save_to_file(receiver_state_t *state, const char *output_file);
void receiver_print_stats(const receiver_state_t *state);

// Helper fonksiyonlar
int parse_ip_header(const uint8_t *buffer, ip_header_t *ip_hdr);
int parse_udp_header(const uint8_t *buffer, udp_header_t *udp_hdr);
int parse_rtp_header(const uint8_t *buffer, rtp_header_t *rtp_hdr);

#endif // RECEIVER_H
