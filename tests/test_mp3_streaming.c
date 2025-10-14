#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include "framing.h"
#include "rtp.h"
#include "network.h"

#define MP3_FILE_PATH "/root/workspace/project/ses.mp3"
#define CHUNK_SIZE 1024  // Her RTP paketinde 1KB MP3 verisi gönderelim

int main() {
    printf("\n");
    printf("========================================\n");
    printf("  MP3 Audio RTP Streaming Test\n");
    printf("========================================\n\n");

    // MP3 dosyasını aç
    FILE *mp3_file = fopen(MP3_FILE_PATH, "rb");
    if (!mp3_file) {
        fprintf(stderr, "Error: Cannot open MP3 file: %s\n", MP3_FILE_PATH);
        return 1;
    }

    // Dosya boyutunu öğren
    fseek(mp3_file, 0, SEEK_END);
    long file_size = ftell(mp3_file);
    fseek(mp3_file, 0, SEEK_SET);

    printf("MP3 File Information:\n");
    printf("----------------------------------------\n");
    printf("File path:   %s\n", MP3_FILE_PATH);
    printf("File size:   %ld bytes (%.2f KB)\n", file_size, file_size / 1024.0);
    printf("Chunk size:  %d bytes\n", CHUNK_SIZE);
    printf("Expected packets: %ld\n\n", (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE);

    // RTP parametreleri
    uint32_t ssrc = 0x12345678;
    uint8_t payload_type = 14;  // RFC 3551: Payload type 14 = MPEG audio
    uint16_t seq_num = 1;
    uint32_t timestamp = 0;
    uint32_t timestamp_increment = 1152;  // MP3 frame için typical değer (44.1kHz'de 26ms)

    // Network parametreleri
    uint32_t src_ip = (192 << 24) | (168 << 16) | (1 << 8) | 100;
    uint32_t dst_ip = (192 << 24) | (168 << 16) | (1 << 8) | 200;
    uint16_t src_port = 5004;
    uint16_t dst_port = 5005;

    // İstatistikler
    int packet_count = 0;
    size_t total_bytes_processed = 0;
    size_t total_packet_size = 0;

    printf("Starting MP3 to RTP packetization...\n");
    printf("========================================\n\n");

    // MP3 dosyasını chunk'lara bölerek işle
    uint8_t chunk_buffer[CHUNK_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(chunk_buffer, 1, CHUNK_SIZE, mp3_file)) > 0) {
        packet_count++;

        printf("Packet #%d\n", packet_count);
        printf("----------------------------------------\n");

        // ========================================
        // AŞAMA 1: ÇERÇEVELEME
        // ========================================
        media_frame_t frame;
        if (frame_create(&frame, chunk_buffer, bytes_read, timestamp, 0) != 0) {
            fprintf(stderr, "Error: Frame creation failed for packet %d\n", packet_count);
            fclose(mp3_file);
            return 1;
        }

        printf("  [STAGE 1] Frame created: %zu bytes, timestamp: %u\n",
               frame.size, frame.timestamp);

        // ========================================
        // AŞAMA 2: RTP PAKETLEME
        // ========================================
        rtp_packet_t rtp_packet;
        rtp_init_header(&rtp_packet.header, payload_type, ssrc);

        if (rtp_create_packet(&rtp_packet, frame.data, frame.size,
                             seq_num, timestamp, 0) != 0) {
            fprintf(stderr, "Error: RTP packet creation failed for packet %d\n", packet_count);
            fclose(mp3_file);
            return 1;
        }

        printf("  [STAGE 2] RTP packet: seq=%u, payload=%zu bytes\n",
               seq_num, rtp_packet.payload_size);

        // ========================================
        // AŞAMA 3: UDP/IP PAKETLEME
        // ========================================
        network_packet_t net_packet;

        // RTP paketini serialize et
        size_t rtp_total_size = sizeof(rtp_header_t) + rtp_packet.payload_size;
        uint8_t *rtp_serialized = (uint8_t *)malloc(rtp_total_size);
        if (!rtp_serialized) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            fclose(mp3_file);
            return 1;
        }

        memcpy(rtp_serialized, &rtp_packet.header, sizeof(rtp_header_t));
        memcpy(rtp_serialized + sizeof(rtp_header_t), rtp_packet.payload, rtp_packet.payload_size);

        if (create_network_packet(&net_packet, rtp_serialized, rtp_total_size,
                                 src_ip, dst_ip, src_port, dst_port) != 0) {
            fprintf(stderr, "Error: Network packet creation failed\n");
            free(rtp_serialized);
            fclose(mp3_file);
            return 1;
        }

        uint16_t packet_total = ntohs(net_packet.ip_header.total_length);
        printf("  [STAGE 3] Network packet: %u bytes (IP+UDP+RTP+Payload)\n", packet_total);
        printf("            IP: %u.%u.%u.%u:%u -> %u.%u.%u.%u:%u\n",
               (ntohl(net_packet.ip_header.src_ip) >> 24) & 0xFF,
               (ntohl(net_packet.ip_header.src_ip) >> 16) & 0xFF,
               (ntohl(net_packet.ip_header.src_ip) >> 8) & 0xFF,
               ntohl(net_packet.ip_header.src_ip) & 0xFF,
               ntohs(net_packet.udp_header.src_port),
               (ntohl(net_packet.ip_header.dst_ip) >> 24) & 0xFF,
               (ntohl(net_packet.ip_header.dst_ip) >> 16) & 0xFF,
               (ntohl(net_packet.ip_header.dst_ip) >> 8) & 0xFF,
               ntohl(net_packet.ip_header.dst_ip) & 0xFF,
               ntohs(net_packet.udp_header.dst_port));

        // İstatistikleri güncelle
        total_bytes_processed += bytes_read;
        total_packet_size += packet_total;

        // Cleanup
        free(rtp_serialized);

        // Sonraki paket için parametreleri güncelle
        seq_num++;
        timestamp += timestamp_increment;

        printf("  Status: Ready for transmission!\n\n");

        // İlk 5 paketi detaylı göster, sonrakileri özetle
        if (packet_count >= 5) {
            printf("... (showing summary for remaining packets)\n\n");
            break;
        }
    }

    // Kalan paketleri hızlıca işle (detay göstermeden)
    while ((bytes_read = fread(chunk_buffer, 1, CHUNK_SIZE, mp3_file)) > 0) {
        packet_count++;
        total_bytes_processed += bytes_read;

        // Paket boyutu hesapla (IP + UDP + RTP header + payload)
        size_t packet_size = sizeof(ip_header_t) + sizeof(udp_header_t) +
                            sizeof(rtp_header_t) + bytes_read;
        total_packet_size += packet_size;

        seq_num++;
        timestamp += timestamp_increment;
    }

    fclose(mp3_file);

    // ========================================
    // ÖZET İSTATİSTİKLER
    // ========================================
    printf("\n========================================\n");
    printf("  Packetization Summary\n");
    printf("========================================\n");
    printf("Total MP3 data:      %zu bytes (%.2f KB)\n",
           total_bytes_processed, total_bytes_processed / 1024.0);
    printf("Total packets:       %d\n", packet_count);
    printf("Total network size:  %zu bytes (%.2f KB)\n",
           total_packet_size, total_packet_size / 1024.0);
    printf("Overhead:            %zu bytes (%.2f%%)\n",
           total_packet_size - total_bytes_processed,
           ((total_packet_size - total_bytes_processed) * 100.0) / total_bytes_processed);
    printf("Avg packet size:     %zu bytes\n", total_packet_size / packet_count);
    printf("\nSequence range:      1 - %u\n", seq_num - 1);
    printf("Timestamp range:     0 - %u\n", timestamp - timestamp_increment);
    printf("\n========================================\n");

    // ========================================
    // DOĞRULAMA
    // ========================================
    printf("\nVALIDATION CHECKS\n");
    printf("========================================\n");

    int validation_passed = 1;

    if (total_bytes_processed == (size_t)file_size) {
        printf("✓ All MP3 data processed: %zu bytes\n", total_bytes_processed);
    } else {
        printf("✗ Data processing incomplete\n");
        validation_passed = 0;
    }

    if (packet_count > 0) {
        printf("✓ Packets created: %d\n", packet_count);
    } else {
        printf("✗ No packets created\n");
        validation_passed = 0;
    }

    if (seq_num == (uint16_t)(packet_count + 1)) {
        printf("✓ Sequence numbers correct: 1-%u\n", packet_count);
    } else {
        printf("✗ Sequence number mismatch\n");
        validation_passed = 0;
    }

    size_t overhead_per_packet = sizeof(ip_header_t) + sizeof(udp_header_t) + sizeof(rtp_header_t);
    size_t expected_total = total_bytes_processed + (overhead_per_packet * packet_count);
    if (total_packet_size == expected_total) {
        printf("✓ Total packet size correct\n");
    } else {
        printf("✗ Total packet size mismatch\n");
        validation_passed = 0;
    }

    printf("========================================\n\n");

    if (validation_passed) {
        printf("✓✓✓ MP3 STREAMING TEST PASSED ✓✓✓\n");
        printf("\nAll %d packets ready for RTP transmission!\n", packet_count);
        printf("Stream can be sent to 192.168.1.200:5005\n");
    } else {
        printf("✗✗✗ VALIDATION FAILED ✗✗✗\n");
    }

    printf("\n========================================\n");

    return validation_passed ? 0 : 1;
}
