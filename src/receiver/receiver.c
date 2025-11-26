#include "receiver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

// Receiver'ı başlat (socket oluştur ve bind et)
int receiver_init(receiver_state_t *state, uint16_t port) {
    if (!state) {
        return -1;
    }

    memset(state, 0, sizeof(receiver_state_t));
    state->listen_port = port;

    // UDP socket oluştur
    state->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (state->sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Socket adresini ayarla
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Tüm interface'lerden dinle
    server_addr.sin_port = htons(port);

    // Socket'i bind et
    if (bind(state->sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(state->sockfd);
        return -1;
    }

    printf("Receiver initialized on port %u\n", port);
    return 0;
}

// Receiver'ı temizle
void receiver_cleanup(receiver_state_t *state) {
    if (state && state->sockfd >= 0) {
        close(state->sockfd);
        state->sockfd = -1;
    }
}

// IP header'ı parse et
int parse_ip_header(const uint8_t *buffer, ip_header_t *ip_hdr) {
    if (!buffer || !ip_hdr) {
        return -1;
    }

    memcpy(ip_hdr, buffer, sizeof(ip_header_t));

    // IP version kontrolü
    uint8_t version = ip_hdr->version_ihl >> 4;
    if (version != 4) {
        fprintf(stderr, "Invalid IP version: %u\n", version);
        return -1;
    }

    return 0;
}

// UDP header'ı parse et
int parse_udp_header(const uint8_t *buffer, udp_header_t *udp_hdr) {
    if (!buffer || !udp_hdr) {
        return -1;
    }

    memcpy(udp_hdr, buffer, sizeof(udp_header_t));
    return 0;
}

// RTP header'ı parse et
int parse_rtp_header(const uint8_t *buffer, rtp_header_t *rtp_hdr) {
    if (!buffer || !rtp_hdr) {
        return -1;
    }

    memcpy(rtp_hdr, buffer, sizeof(rtp_header_t));

    // RTP version kontrolü
    if (rtp_hdr->version != RTP_VERSION) {
        fprintf(stderr, "Invalid RTP version: %u\n", rtp_hdr->version);
        return -1;
    }

    return 0;
}

// Gelen paketi parse et ve state'e ekle
int receiver_parse_packet(receiver_state_t *state, const uint8_t *buffer, size_t buffer_size) {
    if (!state || !buffer || buffer_size < sizeof(rtp_header_t)) {
        return -1;
    }

    // RTP header'ı parse et (UDP socket'ten RTP verisi gelir, IP/UDP yok)
    rtp_header_t rtp_hdr;
    if (parse_rtp_header(buffer, &rtp_hdr) != 0) {
        return -1;
    }

    uint16_t seq_num = ntohs(rtp_hdr.sequence_number);
    uint32_t timestamp = ntohl(rtp_hdr.timestamp);
    uint8_t marker = rtp_hdr.marker;

    // Payload'ı çıkar
    size_t payload_size = buffer_size - sizeof(rtp_header_t);
    const uint8_t *payload = buffer + sizeof(rtp_header_t);

    // Sequence number ile paket index'i bul (seq 1-based, index 0-based)
    int pkt_index = seq_num - 1;

    if (pkt_index < 0 || pkt_index >= MAX_PACKETS) {
        fprintf(stderr, "Sequence number out of range: %u\n", seq_num);
        return -1;
    }

    // Duplicate kontrolü
    if (state->packets[pkt_index].received) {
        state->duplicate_packets++;
        return 0;  // Duplicate, atla
    }

    // Out-of-order kontrolü
    if (pkt_index < state->packet_count - 1) {
        state->out_of_order_packets++;
    }

    // Paketi kaydet
    state->packets[pkt_index].seq_num = seq_num;
    state->packets[pkt_index].timestamp = timestamp;
    state->packets[pkt_index].payload_size = payload_size;
    memcpy(state->packets[pkt_index].payload, payload, payload_size);
    state->packets[pkt_index].received = 1;

    // İstatistikleri güncelle
    state->total_packets_received++;
    state->total_bytes_received += payload_size;

    // En yüksek paket index'ini güncelle
    if (pkt_index + 1 > state->packet_count) {
        state->packet_count = pkt_index + 1;
    }

    // Marker bit varsa son paket (return 1 = stream bitti)
    if (marker == 1) {
        printf("\n[Last packet received - marker bit set]\n");
        return 1;
    }

    return 0;
}

// Paketleri dinle (timeout saniye bekle)
int receiver_listen(receiver_state_t *state, int timeout_sec) {
    if (!state || state->sockfd < 0) {
        return -1;
    }

    uint8_t buffer[RECV_BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Timeout ayarla
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    setsockopt(state->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    printf("Listening for RTP packets on port %u...\n", state->listen_port);
    printf("(Timeout: %d seconds)\n\n", timeout_sec);

    int packets_received = 0;
    time_t start_time = time(NULL);

    while (1) {
        ssize_t recv_len = recvfrom(state->sockfd, buffer, RECV_BUFFER_SIZE, 0,
                                    (struct sockaddr *)&client_addr, &addr_len);

        if (recv_len < 0) {
            // Timeout veya hata
            break;
        }

        if (recv_len > 0) {
            int parse_result = receiver_parse_packet(state, buffer, recv_len);

            if (parse_result >= 0) {
                packets_received++;

                // İlerleme göster (her 10 pakette bir)
                if (packets_received % 10 == 0) {
                    printf("\rReceived packets: %d", packets_received);
                    fflush(stdout);
                }

                // Marker bit görüldü (son paket), stream bitti
                if (parse_result == 1) {
                    printf("\nStream completed (marker received).\n");
                    break;
                }
            }
        }

        // Çok uzun süre paket gelmezse çık
        if (time(NULL) - start_time > timeout_sec + 5) {
            break;
        }
    }

    printf("\n\nReceiving completed.\n");
    return packets_received;
}

// Alınan paketleri dosyaya yaz
int receiver_save_to_file(receiver_state_t *state, const char *output_file) {
    if (!state || !output_file) {
        return -1;
    }

    FILE *fp = fopen(output_file, "wb");
    if (!fp) {
        perror("Failed to open output file");
        return -1;
    }

    printf("\nReassembling packets...\n");

    size_t total_written = 0;
    int missing_packets = 0;

    // Paketleri sırayla dosyaya yaz
    for (int i = 0; i < state->packet_count; i++) {
        if (!state->packets[i].received) {
            fprintf(stderr, "Warning: Packet %d (seq %d) missing!\n", i + 1, i + 1);
            missing_packets++;
            continue;
        }

        size_t written = fwrite(state->packets[i].payload, 1,
                               state->packets[i].payload_size, fp);
        if (written != state->packets[i].payload_size) {
            fprintf(stderr, "Error writing packet %d\n", i + 1);
            fclose(fp);
            return -1;
        }

        total_written += written;
    }

    fclose(fp);

    printf("File saved: %s\n", output_file);
    printf("Total bytes written: %zu\n", total_written);

    if (missing_packets > 0) {
        printf("Warning: %d packets missing!\n", missing_packets);
        return -1;
    }

    return 0;
}

// Receiver istatistiklerini yazdır
void receiver_print_stats(const receiver_state_t *state) {
    if (!state) {
        return;
    }

    printf("\n========================================\n");
    printf("  Receiver Statistics\n");
    printf("========================================\n");
    printf("Total packets received: %u\n", state->total_packets_received);
    printf("Total bytes received:   %zu\n", state->total_bytes_received);
    printf("Expected packets:       %d\n", state->packet_count);
    printf("Duplicate packets:      %u\n", state->duplicate_packets);
    printf("Out-of-order packets:   %u\n", state->out_of_order_packets);

    // Paket kaybı kontrolü
    int missing = 0;
    for (int i = 0; i < state->packet_count; i++) {
        if (!state->packets[i].received) {
            missing++;
        }
    }

    printf("Missing packets:        %d\n", missing);

    if (missing == 0) {
        printf("\n✓ All packets received successfully!\n");
    } else {
        printf("\n✗ %d packets lost (%.2f%% loss rate)\n",
               missing, (missing * 100.0) / state->packet_count);
    }

    printf("========================================\n");
}
