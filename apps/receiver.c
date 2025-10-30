#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "receiver.h"

int main(int argc, char *argv[]) {
    printf("\n========================================\n");
    printf("  RTP Receiver Application\n");
    printf("========================================\n\n");

    // Varsayılan konfigürasyon
    uint16_t listen_port = 5005;
    char *output_file = "received.mp3";
    int timeout = 10;  // 10 saniye timeout

    // Komut satırı argümanları (opsiyonel)
    if (argc >= 2) {
        listen_port = atoi(argv[1]);
    }
    if (argc >= 3) {
        output_file = argv[2];
    }
    if (argc >= 4) {
        timeout = atoi(argv[3]);
    }

    printf("Configuration:\n");
    printf("  Listen port:  %u\n", listen_port);
    printf("  Output file:  %s\n", output_file);
    printf("  Timeout:      %d seconds\n\n", timeout);

    // Receiver'ı başlat
    receiver_state_t receiver;
    if (receiver_init(&receiver, listen_port) != 0) {
        fprintf(stderr, "Failed to initialize receiver\n");
        return 1;
    }

    printf("Receiver ready. Waiting for packets...\n");
    printf("(Press Ctrl+C to stop early, or wait for timeout)\n\n");

    // Paketleri dinle
    int packets_received = receiver_listen(&receiver, timeout);
    if (packets_received < 0) {
        fprintf(stderr, "Failed to receive packets\n");
        receiver_cleanup(&receiver);
        return 1;
    }

    if (packets_received == 0) {
        printf("No packets received.\n");
        receiver_cleanup(&receiver);
        return 1;
    }

    // İstatistikleri göster
    receiver_print_stats(&receiver);

    // Dosyayı kaydet
    printf("\nSaving to file...\n");
    if (receiver_save_to_file(&receiver, output_file) != 0) {
        fprintf(stderr, "Failed to save file\n");
        receiver_cleanup(&receiver);
        return 1;
    }

    // Temizlik
    receiver_cleanup(&receiver);

    printf("\n✓ Receiver completed successfully!\n");
    printf("  Output saved to: %s\n", output_file);
    printf("========================================\n\n");

    return 0;
}
