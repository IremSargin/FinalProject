#ifndef FRAMING_H
#define FRAMING_H

#include <stdint.h>
#include <stddef.h>

#define MAX_FRAME_SIZE 4096

// Medya çerçevesi (frame) yapısı
typedef struct {
    uint8_t data[MAX_FRAME_SIZE];
    size_t size;
    uint32_t timestamp;     // Zaman damgası
    uint8_t type;           // 0: audio, 1: video
    uint8_t is_keyframe;    // Video için keyframe işareti
} media_frame_t;

// Framing fonksiyonları
int frame_create(media_frame_t *frame, const uint8_t *raw_data, size_t size,
                 uint32_t timestamp, uint8_t type);
void frame_print_info(const media_frame_t *frame);

#endif // FRAMING_H
