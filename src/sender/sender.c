#include "sender.h"
#include "framing.h"
#include "rtp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

// Sender'ı başlat
int sender_init(sender_state_t *state, const sender_config_t *config) {
    if (!state || !config) {
        return -1;
    }

    memset(state, 0, sizeof(sender_state_t));
    memcpy(&state->config, config, sizeof(sender_config_t));

    // UDP socket oluştur
    state->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (state->sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Başlangıç değerleri
    state->sequence_number = 1;
    state->timestamp = 0;

    printf("Sender initialized\n");
    printf("  Input file: %s\n", config->input_file);
    printf("  Destination: %s:%u\n", config->dest_ip, config->dest_port);
    printf("  Chunk size: %zu bytes\n", config->chunk_size);
    printf("  Payload type: %u\n", config->payload_type);

    return 0;
}

// Sender'ı temizle
void sender_cleanup(sender_state_t *state) {
    if (state && state->sockfd >= 0) {
        close(state->sockfd);
        state->sockfd = -1;
    }
}

// Dosyayı RTP paketleri olarak gönder
int sender_send_file(sender_state_t *state) {
    if (!state || !state->config.input_file) {
        return -1;
    }

    // Dosyayı aç
    FILE *fp = fopen(state->config.input_file, "rb");
    if (!fp) {
        perror("Failed to open input file");
        return -1;
    }

    // Dosya boyutunu öğren
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    printf("\nStarting transmission...\n");
    printf("File size: %ld bytes\n", file_size);
    printf("========================================\n\n");

    // Hedef adres yapısı
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(state->config.dest_port);
    inet_pton(AF_INET, state->config.dest_ip, &dest_addr.sin_addr);

    // Chunk buffer
    uint8_t chunk_buffer[state->config.chunk_size];
    size_t bytes_read;

    // RTP paketi oluştur
    rtp_packet_t rtp_packet;
    rtp_init_header(&rtp_packet.header, state->config.payload_type, state->config.ssrc);

    int timestamp_increment = 1152;  // MP3 için tipik değer

    // Toplam paket sayısını hesapla (son paketi işaretlemek için)
    int total_packets = (file_size + state->config.chunk_size - 1) / state->config.chunk_size;

    while ((bytes_read = fread(chunk_buffer, 1, state->config.chunk_size, fp)) > 0) {
        // Frame oluştur
        media_frame_t frame;
        if (frame_create(&frame, chunk_buffer, bytes_read, state->timestamp, 0) != 0) {
            fprintf(stderr, "Frame creation failed\n");
            fclose(fp);
            return -1;
        }

        // Son paket mi kontrol et
        uint8_t marker = (state->packets_sent + 1 >= (uint32_t)total_packets) ? 1 : 0;

        // RTP paketi oluştur
        if (rtp_create_packet(&rtp_packet, frame.data, frame.size,
                             state->sequence_number, state->timestamp, marker) != 0) {
            fprintf(stderr, "RTP packet creation failed\n");
            fclose(fp);
            return -1;
        }

        // RTP paketini serialize et (header + payload)
        size_t rtp_total_size = sizeof(rtp_header_t) + rtp_packet.payload_size;
        uint8_t *rtp_serialized = (uint8_t *)malloc(rtp_total_size);
        if (!rtp_serialized) {
            fprintf(stderr, "Memory allocation failed\n");
            fclose(fp);
            return -1;
        }

        memcpy(rtp_serialized, &rtp_packet.header, sizeof(rtp_header_t));
        memcpy(rtp_serialized + sizeof(rtp_header_t), rtp_packet.payload, rtp_packet.payload_size);

        // UDP ile gönder (IP header'ı işletim sistemi ekler)
        ssize_t sent = sendto(state->sockfd, rtp_serialized, rtp_total_size, 0,
                             (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        if (sent < 0) {
            perror("Send failed");
            free(rtp_serialized);
            fclose(fp);
            return -1;
        }

        // İstatistikleri güncelle
        state->packets_sent++;
        state->bytes_sent += bytes_read;

        // İlerleme göster
        if (state->packets_sent % 10 == 0) {
            printf("\rSent packets: %u (%.1f%%)",
                   state->packets_sent,
                   (state->bytes_sent * 100.0) / file_size);
            fflush(stdout);
        }

        free(rtp_serialized);

        // Sonraki paket için güncelle
        state->sequence_number++;
        state->timestamp += timestamp_increment;

        // Paketler arası gecikme (opsiyonel)
        if (state->config.delay_ms > 0) {
            usleep(state->config.delay_ms * 1000);
        }
    }

    fclose(fp);

    printf("\n\nTransmission completed!\n");
    return 0;
}

// Sender istatistiklerini yazdır
void sender_print_stats(const sender_state_t *state) {
    if (!state) {
        return;
    }

    printf("\n========================================\n");
    printf("  Sender Statistics\n");
    printf("========================================\n");
    printf("Packets sent:     %u\n", state->packets_sent);
    printf("Bytes sent:       %zu\n", state->bytes_sent);
    printf("Sequence range:   1 - %u\n", state->sequence_number - 1);
    printf("Timestamp range:  0 - %u\n", state->timestamp);
    printf("========================================\n");
}
