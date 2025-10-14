# RTP 3-Stage Packetization Project

FPGA tabanlı gerçek zamanlı ses/video iletimi için temel paketleme sistemi.

## Proje Yapısı

```
project/
├── include/          # Header dosyaları
│   ├── framing.h    # Çerçeveleme (Stage 1)
│   ├── rtp.h        # RTP paketleme (Stage 2)
│   └── network.h    # UDP/IP paketleme (Stage 3)
├── src/
│   ├── framing/     # Çerçeveleme implementasyonu
│   ├── rtp/         # RTP implementasyonu
│   └── udp_ip/      # Network implementasyonu
├── tests/           # Test programları
├── build/           # Build çıktıları
└── Makefile
```

## Üç Aşamalı Paketleme

### Aşama 1: Çerçeveleme (Framing)
- Ham veriyi (raw audio/video) mantıksal çerçevelere ayırır
- Timestamp ekler
- Medya tipini belirler (audio/video)

### Aşama 2: RTP Paketleme
- RTP header ekler (RFC 3550)
- Sequence number ve timestamp yönetimi
- MTU'ya uygun parçalama desteği
- Payload type ve SSRC tanımlama

### Aşama 3: UDP/IP Paketleme
- UDP header ekler (RFC 768)
- IP header ekler (RFC 791)
- Checksum hesaplama
- Network byte order dönüşümü

## Derleme ve Çalıştırma

```bash
# Derleme
make

# Test çalıştırma
make test

# Temizleme
make clean
```

## Test Çıktısı

Program şu doğrulamaları yapar:
- Frame boyutu kontrolü
- RTP version kontrolü
- RTP payload boyut kontrolü
- IP version kontrolü
- IP protocol kontrolü (UDP)
- UDP port kontrolü
- Toplam paket boyutu kontrolü

Tüm testler başarılı olduğunda paket iletim için hazırdır.

## Teknik Detaylar

- **RTP Version:** 2
- **Max Payload Size:** 1400 bytes (MTU uyumlu)
- **IP Version:** 4
- **Transport Protocol:** UDP (17)
- **Default TTL:** 64

## Gelecek Adımlar

- [ ] Gerçek socket üzerinden iletim
- [ ] Multi-paket fragmentasyon
- [ ] RTCP implementasyonu
- [ ] Gerçek codec entegrasyonu (H.264, AAC)
- [ ] FPGA HDL dönüşümü
