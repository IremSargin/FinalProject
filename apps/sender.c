#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sender.h"

int main(int argc, char *argv[]) {
    printf("\n========================================\n");
    printf("  RTP Sender Application\n");
    printf("========================================\n\n");

    // Varsayılan konfigürasyon
    sender_config_t config = {
        .input_file = "/root/workspace/project/ses.mp3",
        .dest_ip = "127.0.0.1",  // localhost
        .dest_port = 5005,
        .src_port = 5004,
        .payload_type = 14,      // MPEG Audio
        .ssrc = 0x12345678,
        .chunk_size = 1024,
        .delay_ms = 2            // 2ms delay (paket kaybını azaltmak için)
    };

    // Komut satırı argümanları (opsiyonel)
    if (argc >= 2) {
        config.input_file = argv[1];
    }
    if (argc >= 3) {
        config.dest_ip = argv[2];
    }
    if (argc >= 4) {
        config.dest_port = atoi(argv[3]);
    }

    // Sender'ı başlat
    sender_state_t sender;
    if (sender_init(&sender, &config) != 0) {
        fprintf(stderr, "Failed to initialize sender\n");
        return 1;
    }

    // Dosyayı gönder
    if (sender_send_file(&sender) != 0) {
        fprintf(stderr, "Failed to send file\n");
        sender_cleanup(&sender);
        return 1;
    }

    // İstatistikleri göster
    sender_print_stats(&sender);

    // Temizlik
    sender_cleanup(&sender);

    printf("\n✓ Sender completed successfully!\n");
    printf("========================================\n\n");

    return 0;
}
