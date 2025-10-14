#include "framing.h"
#include <string.h>
#include <stdio.h>

// Medya frame'i oluştur (Aşama 1: Çerçeveleme)
int frame_create(media_frame_t *frame, const uint8_t *raw_data, size_t size,
                 uint32_t timestamp, uint8_t type) {
    if (!frame || !raw_data || size == 0 || size > MAX_FRAME_SIZE) {
        return -1;
    }

    memcpy(frame->data, raw_data, size);
    frame->size = size;
    frame->timestamp = timestamp;
    frame->type = type;
    frame->is_keyframe = (type == 1) ? 1 : 0;  // Video için default keyframe

    return 0;
}

// Frame bilgilerini yazdır
void frame_print_info(const media_frame_t *frame) {
    if (!frame) {
        return;
    }

    printf("=== Media Frame Info ===\n");
    printf("Type:       %s\n", frame->type == 0 ? "Audio" : "Video");
    printf("Size:       %zu bytes\n", frame->size);
    printf("Timestamp:  %u\n", frame->timestamp);
    printf("Keyframe:   %s\n", frame->is_keyframe ? "Yes" : "No");
    printf("Data (hex): ");
    for (size_t i = 0; i < (frame->size < 16 ? frame->size : 16); i++) {
        printf("%02X ", frame->data[i]);
    }
    if (frame->size > 16) {
        printf("...");
    }
    printf("\n========================\n");
}
