#ifndef SENDER_H
#define SENDER_H

#include <stdint.h>
#include <stddef.h>

// Sender konfigürasyonu
typedef struct {
    char *input_file;           // Gönderilecek dosya
    char *dest_ip;              // Hedef IP (örn: "127.0.0.1")
    uint16_t dest_port;         // Hedef port
    uint16_t src_port;          // Kaynak port

    uint8_t payload_type;       // RTP payload type (14=MP3)
    uint32_t ssrc;              // RTP SSRC
    size_t chunk_size;          // Her pakette kaç byte gönderilecek

    int delay_ms;               // Paketler arası gecikme (ms)
} sender_config_t;

// Sender durumu
typedef struct {
    int sockfd;                 // UDP socket
    sender_config_t config;

    // İstatistikler
    uint32_t packets_sent;
    size_t bytes_sent;
    uint16_t sequence_number;
    uint32_t timestamp;
} sender_state_t;

// Sender fonksiyonları
int sender_init(sender_state_t *state, const sender_config_t *config);
void sender_cleanup(sender_state_t *state);

int sender_send_file(sender_state_t *state);
void sender_print_stats(const sender_state_t *state);

#endif // SENDER_H
