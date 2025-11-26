# RTP 3-Stage Packetization Project

FPGA tabanlı gerçek zamanlı ses/video iletimi için **üretim seviyesinde** çalışan RTP streaming sistemi.

**✅ Gerçek network üzerinden test edildi!**
**✅ İki DigitalOcean VM arasında başarıyla çalıştı!**
**✅ 72 paket, 0 kayıp, byte-perfect transfer!**

---

## İçindekiler

- [Proje Yapısı](#proje-yapısı)
- [Üç Aşamalı Paketleme](#üç-aşamalı-paketleme)
- [Derleme ve Çalıştırma](#derleme-ve-çalıştırma)
- [Kullanım Senaryoları](#kullanım-senaryoları)
- [Network Üzerinden Test](#network-üzerinden-test)
- [Gerçek Test Sonuçları](#gerçek-test-sonuçları)
- [Teknik Detaylar](#teknik-detaylar)

---

## Proje Yapısı

```
project/
├── include/              # Header dosyaları
│   ├── framing.h        # Aşama 1: Çerçeveleme
│   ├── rtp.h            # Aşama 2: RTP paketleme
│   ├── network.h        # Aşama 3: UDP/IP paketleme
│   ├── sender.h         # Sender modülü
│   └── receiver.h       # Receiver modülü
├── src/
│   ├── framing/         # Çerçeveleme implementasyonu
│   ├── rtp/             # RTP implementasyonu
│   ├── udp_ip/          # Network implementasyonu
│   ├── sender/          # Sender implementasyonu
│   └── receiver/        # Receiver implementasyonu
├── apps/
│   ├── sender.c         # Sender uygulaması
│   └── receiver.c       # Receiver uygulaması
├── tests/
│   ├── test_packetization.c    # Basit test (256 byte)
│   └── test_mp3_streaming.c    # MP3 streaming testi
├── docs/
│   └── PACKET_VALIDATION.md    # Detaylı doğrulama kılavuzu
├── build/               # Build çıktıları
├── Makefile
└── README.md
```

---

## Üç Aşamalı Paketleme

### Aşama 1: Çerçeveleme (Framing)
**Ne yapar?**
- Ham audio/video verisini mantıksal çerçevelere (frame) böler
- Her çerçeveye zaman damgası (timestamp) ekler
- Medya tipini belirler (audio=0, video=1)

**Örnek:**
```c
media_frame_t frame;
frame_create(&frame, raw_data, 1024, timestamp, 0);
// frame.data = 1024 bytes MP3
// frame.timestamp = 0, 1152, 2304, ...
```

### Aşama 2: RTP Paketleme
**Ne yapar?**
- Her frame'e RTP header ekler (12 byte)
- Sequence number: 1, 2, 3, ... (paket sırası)
- Timestamp: Ses/video senkronizasyonu için
- Marker bit: Son paket işaretlemesi (receiver'ın durması için)
- SSRC: Stream identifier

**RTP Header yapısı:**
```
┌────────────────────────────────────────┐
│ Version (2) | Marker | Payload Type   │  2 bytes
├────────────────────────────────────────┤
│ Sequence Number (1, 2, 3, ...)         │  2 bytes
├────────────────────────────────────────┤
│ Timestamp                               │  4 bytes
├────────────────────────────────────────┤
│ SSRC (0x12345678)                       │  4 bytes
└────────────────────────────────────────┘
Toplam: 12 bytes
```

### Aşama 3: UDP/IP Paketleme
**Ne yapar?**
- UDP header ekler (8 byte): port bilgileri
- IP header ekler (20 byte): IP adresleri, checksum
- Network byte order'a çevirir (big-endian)

**Tam Paket Yapısı:**
```
┌─────────────────────────────────┐
│  IP Header        (20 bytes)    │ ← 192.168.x.x veya Public IP
├─────────────────────────────────┤
│  UDP Header       (8 bytes)     │ ← Port: 5004 → 5005
├─────────────────────────────────┤
│  RTP Header       (12 bytes)    │ ← Seq, Timestamp, Marker
├─────────────────────────────────┤
│  Payload          (1024 bytes)  │ ← Gerçek ses/video verisi
└─────────────────────────────────┘
Toplam: 1064 bytes (overhead: 40 bytes = 3.91%)
```

---

## Derleme ve Çalıştırma

### Tüm Projeyi Derle
```bash
make clean
make all
```

Bu komut şunları derler:
- `build/test_packetization` - Basit test programı
- `build/test_mp3_streaming` - MP3 paketleme testi
- `build/sender` - RTP sender uygulaması
- `build/receiver` - RTP receiver uygulaması

### Testleri Çalıştır

**Basit Test (256 byte dummy data):**
```bash
make test
```

**MP3 Paketleme Testi:**
```bash
make test-mp3
```

**Localhost Loopback Testi (otomatik):**
```bash
make stream
```
Bu komut:
1. Receiver'ı background'da başlatır
2. Sender'ı başlatır
3. Dosyaları karşılaştırır (md5sum)
4. Sonucu gösterir

### Manuel Kullanım

**İki Terminal ile Localhost Testi:**

Terminal 1 (Receiver):
```bash
./build/receiver 5005 received.mp3 60
```

Terminal 2 (Sender):
```bash
./build/sender ses.mp3 127.0.0.1 5005
```

**Parametreler:**

Receiver:
```bash
./build/receiver [port] [output_file] [timeout_seconds]
```

Sender:
```bash
./build/sender [input_file] [dest_ip] [dest_port]
```

---

## Kullanım Senaryoları

### Senaryo 1: Localhost Testi
```bash
# Terminal 1
./build/receiver 5005 output.mp3 60

# Terminal 2
./build/sender ses.mp3 127.0.0.1 5005

# Doğrulama
md5sum ses.mp3 output.mp3
```

### Senaryo 2: Aynı LAN İçinde
```bash
# PC1 (Receiver - 192.168.1.100)
./build/receiver 5005 received.mp3 120

# PC2 (Sender)
./build/sender video.mp3 192.168.1.100 5005
```

### Senaryo 3: İnternet Üzerinden (İki VM)
```bash
# VM1 (Receiver - Public IP: 165.22.69.82)
sudo ufw allow 5005/udp
./build/receiver 5005 received.mp3 600

# VM2 (Sender)
./build/sender ses.mp3 165.22.69.82 5005
```

---

## Network Üzerinden Test

### İki DigitalOcean VM Arası İletim

**Gereksinimler:**
1. İki ayrı VM (Ubuntu 20.04+)
2. Firewall'da UDP 5005 portu açık
3. Projenin her iki VM'de derlenmiş olması

**Adım 1: VM1'de Receiver Hazırlığı**
```bash
# Firewall'da portu aç
sudo ufw allow 5005/udp
sudo ufw status

# Dinlemeye başla
cd /root/workspace/project
./build/receiver 5005 received.mp3 600
```

**Adım 2: VM2'den Gönderim**
```bash
cd /root/workspace/project
./build/sender ses.mp3 <VM1_PUBLIC_IP> 5005
```

**Adım 3: Doğrulama**
```bash
# VM1'de (Receiver)
md5sum received.mp3

# VM2'de (Sender)
md5sum ses.mp3

# Hash'ler aynı olmalı!
```

### Firewall Ayarları

**DigitalOcean Cloud Firewall:**
```
Inbound Rules:
- Type: Custom
- Protocol: UDP
- Port: 5005
- Sources: Specific IP (veya All IPv4 test için)
```

**VM Üzerinde ufw:**
```bash
# Belirli IP'den izin ver
sudo ufw allow from <SENDER_IP> to any port 5005 proto udp

# Veya herkesten izin ver (sadece test için)
sudo ufw allow 5005/udp

# Kuralları kontrol et
sudo ufw status numbered
```

### Troubleshooting

**Sorun 1: Paket gelmiyor**
```bash
# Receiver'da port dinleniyor mu?
sudo netstat -tuln | grep 5005

# Paketler geliyor mu?
sudo tcpdump -i eth0 udp port 5005 -n

# Firewall açık mı?
sudo ufw status
```

**Sorun 2: Paket kaybı yüksek**
```bash
# Network kalitesini test et
ping -c 100 <DEST_IP>

# Sender'da delay artır (apps/sender.c)
.delay_ms = 5  // 2ms → 5ms
```

**Sorun 3: Receiver kapanmıyor**
- Sender'ın marker bit göndermediği anlamına gelir
- Yeniden derleyin: `make clean && make all`

---

## Gerçek Test Sonuçları

### Test Ortamı
- **Platform:** DigitalOcean Droplets
- **VM1 (Receiver):** Ubuntu 22.04, 165.22.69.82
- **VM2 (Sender):** Ubuntu 22.04, farklı region
- **Network:** Public internet üzerinden
- **Dosya:** ses.mp3 (71.88 KB / 73,605 bytes)

### Test Sonuçları

**Sender İstatistikleri:**
```
Packets sent:     72
Bytes sent:       73,605
Sequence range:   1 - 72
Timestamp range:  0 - 82,944
Transfer time:    ~150ms
```

**Receiver İstatistikleri:**
```
Total packets received: 72
Total bytes received:   73,605
Expected packets:       72
Duplicate packets:      0
Out-of-order packets:   0
Missing packets:        0

✓ All packets received successfully!
```

**Dosya Bütünlüğü:**
```bash
MD5 (ses.mp3):      115d71a823da5054f0a7f502b7775cd7
MD5 (received.mp3): 115d71a823da5054f0a7f502b7775cd7
✓ IDENTICAL - Byte-perfect transfer!
```

### Başarı Metrikleri
- ✅ **Paket kaybı:** %0 (0/72)
- ✅ **Veri bütünlüğü:** %100 (byte-perfect)
- ✅ **Overhead:** 3.91% (2,880 bytes / 73,605 bytes)
- ✅ **Marker bit:** Çalışıyor (receiver otomatik kapanıyor)
- ✅ **Sequence integrity:** Tüm paketler sıralı
- ✅ **Latency:** ~150ms (72 paket, 2ms delay)

---

## Teknik Detaylar

### RFC Standartları
- **RFC 3550:** RTP (Real-time Transport Protocol)
- **RFC 3551:** RTP Payload Types (14 = MPA/MPEG Audio)
- **RFC 768:** UDP (User Datagram Protocol)
- **RFC 791:** IP (Internet Protocol v4)

### Protokol Parametreleri
| Parametre              | Değer                  | Açıklama                    |
|------------------------|------------------------|-----------------------------|
| RTP Version            | 2                      | RFC 3550 standardı          |
| Payload Type           | 14                     | MPEG Audio (RFC 3551)       |
| Chunk Size             | 1024 bytes             | Paket başına payload        |
| Max Payload Size       | 1400 bytes             | MTU'ya uygun                |
| IP Version             | 4                      | IPv4                        |
| Transport Protocol     | 17 (UDP)               | Connectionless              |
| TTL                    | 64                     | Standart değer              |
| Timestamp Increment    | 1152                   | 44.1kHz'de ~26ms            |
| Inter-packet Delay     | 2ms                    | Paket kaybını önlemek için  |
| Marker Bit             | Son pakette 1          | Stream son kontrolü         |

### Network Adresleme
| Parametre       | Varsayılan         | Açıklama                    |
|-----------------|--------------------|-----------------------------|
| Listen Port     | 5005               | Receiver dinleme portu      |
| Source Port     | 5004               | Sender kaynak portu         |
| SSRC            | 0x12345678         | Stream identifier           |
| Default IP      | 127.0.0.1          | Localhost (test için)       |

### Performans
```
Dosya Boyutu:         71.88 KB (73,605 bytes)
Paket Sayısı:         72
Network Boyutu:       74.69 KB (76,485 bytes)
Overhead:             2.88 KB (2,880 bytes)
Overhead Oranı:       3.91%
Verimlilik:           96.09%
Ortalama Paket:       1,062 bytes
Transfer Süresi:      ~150ms (2ms delay × 72 paket)
Throughput:           ~4.7 Mbps
```

### Marker Bit Özelliği
```c
// Sender (src/sender/sender.c)
uint8_t marker = (son_paket) ? 1 : 0;
rtp_create_packet(..., marker);

// Receiver (src/receiver/receiver.c)
if (rtp_hdr.marker == 1) {
    printf("[Last packet received]\n");
    // Receiver otomatik kapanır
}
```

---

## Özellikler

### ✅ Tamamlanan
- [x] Üç aşamalı paketleme (Framing → RTP → UDP/IP)
- [x] RFC 3550 uyumlu RTP implementasyonu
- [x] Gerçek UDP socket iletimi
- [x] Sender ve Receiver uygulamaları
- [x] Marker bit ile stream kontrolü
- [x] Sequence number takibi
- [x] Timestamp yönetimi
- [x] Network byte order dönüşümleri
- [x] Checksum hesaplama
- [x] Paket reassembly
- [x] Localhost testi
- [x] LAN testi
- [x] Internet üzerinden test (DigitalOcean)
- [x] Byte-perfect file transfer
- [x] Otomatik loopback test

### 🚧 Gelecek Geliştirmeler
- [ ] RTCP implementasyonu (sender/receiver reports)
- [ ] Jitter buffer (out-of-order paket desteği)
- [ ] Forward Error Correction (FEC)
- [ ] Adaptive bitrate
- [ ] Multi-stream support (audio + video)
- [ ] H.264 video codec entegrasyonu
- [ ] RTSP kontrol protokolü
- [ ] SDP (Session Description Protocol)
- [ ] NAT traversal (STUN/TURN)
- [ ] FPGA Verilog/VHDL dönüşümü

---

## Make Komutları Referansı

```bash
# Tüm programları derle (tests + sender + receiver)
make all

# Sadece sender'ı derle
make sender

# Sadece receiver'ı derle
make receiver

# Basit test çalıştır (256 byte)
make test

# MP3 streaming test çalıştır
make test-mp3

# Otomatik loopback test (sender → receiver → karşılaştır)
make stream

# Build dosyalarını temizle
make clean

# Yardım mesajını göster
make help
```

---

## Hızlı Başlangıç

**1. Projeyi derle:**
```bash
make clean && make all
```

**2. Localhost'ta test et:**
```bash
make stream
```

**3. İki bilgisayar arası test:**
```bash
# Bilgisayar 1 (Receiver)
./build/receiver 5005 output.mp3 120

# Bilgisayar 2 (Sender)
./build/sender ses.mp3 <IP_ADDRESS> 5005
```

**4. Sonucu doğrula:**
```bash
md5sum ses.mp3 output.mp3
```

---

## Önemli Notlar

### UDP Paket Kaybı
UDP, connection-less bir protokoldür ve paket kaybı normaldir:
- **Localhost:** %0-1 kayıp (buffer yeterli)
- **LAN:** %0-2 kayıp (düşük latency)
- **Internet:** %0-5 kayıp (network kalitesine bağlı)

Paket kaybını azaltmak için:
1. `apps/sender.c`'de `.delay_ms` artırın (2ms → 5ms)
2. Chunk size küçültün (1024 → 512)
3. QoS ayarları (router/firewall)

### Güvenlik
Bu sistem **eğitim/test amaçlıdır**. Production'da:
- SRTP (Secure RTP) kullanın
- TLS/DTLS ekleyin
- Authentication ekleyin
- Rate limiting yapın

---

## Kaynaklar

### RFC Dokümanları
- [RFC 3550 - RTP Protocol](https://tools.ietf.org/html/rfc3550)
- [RFC 3551 - RTP Payload Types](https://tools.ietf.org/html/rfc3551)
- [RFC 768 - UDP](https://tools.ietf.org/html/rfc768)
- [RFC 791 - IP](https://tools.ietf.org/html/rfc791)

### Test Araçları
- **Wireshark:** RTP stream analizi
- **VLC:** RTP player (rtp://)
- **FFmpeg:** Stream encoder/decoder
- **tcpdump:** Packet capture

### Dokümanlar
- `docs/PACKET_VALIDATION.md` - Detaylı paket doğrulama kılavuzu
- Source code comments - Inline açıklamalar
- Header files - API dokümantasyonu

---

## Lisans ve Kullanım

Bu proje **MIT Lisansı** altında eğitim amaçlıdır.

**Kullanım Alanları:**
- ✅ Akademik araştırma
- ✅ FPGA öğrenme projesi
- ✅ Network protokol eğitimi
- ✅ RTP/UDP implementasyon referansı
- ✅ Savunma sanayii R&D (temel altyapı)
- ✅ Telekomünikasyon sistem prototipi

---

## İletişim ve Destek

**Sorun Bildirimi:**
- GitHub Issues (varsa)
- E-posta ile destek

**Katkıda Bulunma:**
- Pull request'ler kabul edilir
- Code review süreci uygulanır
- Test coverage korunmalı

---

**Versiyon:** 2.0
**Son Güncelleme:** 2025
**Durum:** ✅ Production Ready (Eğitim/Test)
**Test Edildi:** ✅ DigitalOcean VM'ler arası gerçek network
