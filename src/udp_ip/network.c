#include "network.h"
#include "rtp.h"
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

// UDP header'ı başlat
void udp_init_header(udp_header_t *header, uint16_t src_port, uint16_t dst_port, uint16_t data_len) {
    if (!header) {
        return;
    }

    header->src_port = htons(src_port);
    header->dst_port = htons(dst_port);
    header->length = htons(sizeof(udp_header_t) + data_len);
    header->checksum = 0;  // Basitleştirme için 0 (UDP'de optional)
}

// IP header'ı başlat
void ip_init_header(ip_header_t *header, uint32_t src_ip, uint32_t dst_ip, uint16_t total_len) {
    if (!header) {
        return;
    }

    memset(header, 0, sizeof(ip_header_t));
    header->version_ihl = 0x45;  // Version 4, IHL 5 (20 bytes)
    header->tos = 0;
    header->total_length = htons(total_len);
    header->identification = htons(0x1234);  // Sabit ID (test için)
    header->flags_offset = 0;
    header->ttl = 64;
    header->protocol = 17;  // UDP
    header->src_ip = htonl(src_ip);
    header->dst_ip = htonl(dst_ip);
    header->checksum = 0;

    // IP header checksum hesapla
    header->checksum = calculate_checksum(header, sizeof(ip_header_t));
}

// Checksum hesaplama (RFC 1071)
uint16_t calculate_checksum(const void *data, size_t len) {
    const uint16_t *buf = (const uint16_t *)data;
    uint32_t sum = 0;
    size_t count = len;

    while (count > 1) {
        sum += *buf++;
        count -= 2;
    }

    // Tek byte kaldıysa ekle
    if (count > 0) {
        sum += *(const uint8_t *)buf;
    }

    // Carry bitlerini ekle
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

// Network paketi oluştur (Aşama 3: UDP/IP Paketleme)
int create_network_packet(network_packet_t *packet, const uint8_t *rtp_data, size_t rtp_size,
                         uint32_t src_ip, uint32_t dst_ip, uint16_t src_port, uint16_t dst_port) {
    if (!packet || !rtp_data || rtp_size == 0) {
        return -1;
    }

    // RTP verisini sakla
    packet->rtp_data = (uint8_t *)rtp_data;
    packet->rtp_data_size = rtp_size;

    // UDP header'ı oluştur
    udp_init_header(&packet->udp_header, src_port, dst_port, rtp_size);

    // IP header'ı oluştur
    uint16_t total_len = sizeof(ip_header_t) + sizeof(udp_header_t) + rtp_size;
    ip_init_header(&packet->ip_header, src_ip, dst_ip, total_len);

    return 0;
}

// Network paketini yazdır
void print_network_packet(const network_packet_t *packet) {
    if (!packet) {
        return;
    }

    printf("\n=== Network Packet Info ===\n");

    printf("\n--- IP Header ---\n");
    printf("Version:     %u\n", packet->ip_header.version_ihl >> 4);
    printf("IHL:         %u\n", packet->ip_header.version_ihl & 0x0F);
    printf("Total Len:   %u bytes\n", ntohs(packet->ip_header.total_length));
    printf("Protocol:    %u (UDP)\n", packet->ip_header.protocol);
    printf("TTL:         %u\n", packet->ip_header.ttl);
    printf("Checksum:    0x%04X\n", ntohs(packet->ip_header.checksum));

    uint32_t src = ntohl(packet->ip_header.src_ip);
    uint32_t dst = ntohl(packet->ip_header.dst_ip);
    printf("Source IP:   %u.%u.%u.%u\n",
           (src >> 24) & 0xFF, (src >> 16) & 0xFF,
           (src >> 8) & 0xFF, src & 0xFF);
    printf("Dest IP:     %u.%u.%u.%u\n",
           (dst >> 24) & 0xFF, (dst >> 16) & 0xFF,
           (dst >> 8) & 0xFF, dst & 0xFF);

    printf("\n--- UDP Header ---\n");
    printf("Source Port: %u\n", ntohs(packet->udp_header.src_port));
    printf("Dest Port:   %u\n", ntohs(packet->udp_header.dst_port));
    printf("Length:      %u bytes\n", ntohs(packet->udp_header.length));
    printf("Checksum:    0x%04X\n", ntohs(packet->udp_header.checksum));

    printf("\n--- RTP Data ---\n");
    printf("RTP Size:    %zu bytes\n", packet->rtp_data_size);
    printf("RTP Data (first 16 bytes): ");
    for (size_t i = 0; i < (packet->rtp_data_size < 16 ? packet->rtp_data_size : 16); i++) {
        printf("%02X ", packet->rtp_data[i]);
    }
    if (packet->rtp_data_size > 16) {
        printf("...");
    }
    printf("\n===========================\n\n");
}
