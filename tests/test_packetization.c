#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include "framing.h"
#include "rtp.h"
#include "network.h"

#define TEST_DATA_SIZE 256

int main() {
    printf("\n");
    printf("========================================\n");
    printf("  RTP 3-Stage Packetization Test\n");
    printf("========================================\n\n");

    // Test için ham veri oluştur (simulated raw audio/video data)
    uint8_t raw_data[TEST_DATA_SIZE];
    for (int i = 0; i < TEST_DATA_SIZE; i++) {
        raw_data[i] = i & 0xFF;  // 0-255 arası pattern
    }

    printf("Step 0: Raw Data Created\n");
    printf("----------------------------------------\n");
    printf("Size: %d bytes\n", TEST_DATA_SIZE);
    printf("Data (first 16 bytes): ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", raw_data[i]);
    }
    printf("...\n\n");

    // ========================================
    // AŞAMA 1: ÇERÇEVELEME (FRAMING)
    // ========================================
    printf("STAGE 1: FRAMING\n");
    printf("========================================\n");

    media_frame_t frame;
    uint32_t timestamp = (uint32_t)time(NULL);

    if (frame_create(&frame, raw_data, TEST_DATA_SIZE, timestamp, 0) != 0) {
        fprintf(stderr, "Error: Frame creation failed\n");
        return 1;
    }

    frame_print_info(&frame);

    // ========================================
    // AŞAMA 2: RTP PAKETLEME
    // ========================================
    printf("\nSTAGE 2: RTP PACKETIZATION\n");
    printf("========================================\n");

    rtp_packet_t rtp_packet;
    rtp_init_header(&rtp_packet.header, 96, 0x12345678);  // Payload type 96 (dynamic), SSRC

    if (rtp_create_packet(&rtp_packet, frame.data, frame.size,
                         1, timestamp, 0) != 0) {
        fprintf(stderr, "Error: RTP packet creation failed\n");
        return 1;
    }

    rtp_print_header(&rtp_packet.header);
    printf("Payload Size: %zu bytes\n", rtp_packet.payload_size);
    printf("Payload (first 16 bytes): ");
    for (size_t i = 0; i < (rtp_packet.payload_size < 16 ? rtp_packet.payload_size : 16); i++) {
        printf("%02X ", rtp_packet.payload[i]);
    }
    printf("...\n");

    // ========================================
    // AŞAMA 3: UDP/IP PAKETLEME
    // ========================================
    printf("\n\nSTAGE 3: UDP/IP PACKETIZATION\n");
    printf("========================================\n");

    network_packet_t net_packet;

    // RTP paketini serialize et (header + payload)
    size_t rtp_total_size = sizeof(rtp_header_t) + rtp_packet.payload_size;
    uint8_t *rtp_serialized = (uint8_t *)malloc(rtp_total_size);
    if (!rtp_serialized) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }

    memcpy(rtp_serialized, &rtp_packet.header, sizeof(rtp_header_t));
    memcpy(rtp_serialized + sizeof(rtp_header_t), rtp_packet.payload, rtp_packet.payload_size);

    // Network paketi oluştur
    // Örnek IP'ler: 192.168.1.100 -> 192.168.1.200
    uint32_t src_ip = (192 << 24) | (168 << 16) | (1 << 8) | 100;
    uint32_t dst_ip = (192 << 24) | (168 << 16) | (1 << 8) | 200;
    uint16_t src_port = 5004;
    uint16_t dst_port = 5005;

    if (create_network_packet(&net_packet, rtp_serialized, rtp_total_size,
                             src_ip, dst_ip, src_port, dst_port) != 0) {
        fprintf(stderr, "Error: Network packet creation failed\n");
        free(rtp_serialized);
        return 1;
    }

    print_network_packet(&net_packet);

    // ========================================
    // DOĞRULAMA
    // ========================================
    printf("VALIDATION CHECKS\n");
    printf("========================================\n");

    int validation_passed = 1;

    // Check 1: Frame size doğru mu?
    if (frame.size == TEST_DATA_SIZE) {
        printf("✓ Frame size correct: %zu bytes\n", frame.size);
    } else {
        printf("✗ Frame size incorrect\n");
        validation_passed = 0;
    }

    // Check 2: RTP header version doğru mu?
    if (rtp_packet.header.version == RTP_VERSION) {
        printf("✓ RTP version correct: %u\n", rtp_packet.header.version);
    } else {
        printf("✗ RTP version incorrect\n");
        validation_passed = 0;
    }

    // Check 3: RTP payload size doğru mu?
    if (rtp_packet.payload_size == frame.size) {
        printf("✓ RTP payload size matches frame size: %zu bytes\n", rtp_packet.payload_size);
    } else {
        printf("✗ RTP payload size mismatch\n");
        validation_passed = 0;
    }

    // Check 4: IP version doğru mu?
    if ((net_packet.ip_header.version_ihl >> 4) == 4) {
        printf("✓ IP version correct: 4\n");
    } else {
        printf("✗ IP version incorrect\n");
        validation_passed = 0;
    }

    // Check 5: IP protocol UDP mu?
    if (net_packet.ip_header.protocol == 17) {
        printf("✓ IP protocol correct: UDP (17)\n");
    } else {
        printf("✗ IP protocol incorrect\n");
        validation_passed = 0;
    }

    // Check 6: UDP port'lar doğru mu?
    if (ntohs(net_packet.udp_header.src_port) == src_port &&
        ntohs(net_packet.udp_header.dst_port) == dst_port) {
        printf("✓ UDP ports correct: %u -> %u\n", src_port, dst_port);
    } else {
        printf("✗ UDP ports incorrect\n");
        validation_passed = 0;
    }

    // Check 7: Total packet size doğru mu?
    uint16_t expected_ip_total = sizeof(ip_header_t) + sizeof(udp_header_t) + rtp_total_size;
    if (ntohs(net_packet.ip_header.total_length) == expected_ip_total) {
        printf("✓ Total packet size correct: %u bytes\n", expected_ip_total);
    } else {
        printf("✗ Total packet size incorrect\n");
        validation_passed = 0;
    }

    printf("========================================\n\n");

    if (validation_passed) {
        printf("✓✓✓ ALL VALIDATION CHECKS PASSED ✓✓✓\n");
        printf("\nPacket is ready for transmission!\n");
        printf("Total encapsulated size: %u bytes\n", expected_ip_total);
    } else {
        printf("✗✗✗ VALIDATION FAILED ✗✗✗\n");
    }

    printf("\n========================================\n");

    // Cleanup
    free(rtp_serialized);

    return validation_passed ? 0 : 1;
}
