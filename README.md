# MiniProjectRTOS
Smart Door Lock
# Smart Door Lock System with PIN Authentication

ESP32-S3 + FreeRTOS — Semaphore, Interrupt, Queue, Mutex, OLED Display & Servo Control

---

Project ini merupakan sistem keamanan pintu cerdas yang berjalan pada ESP32-S3 menggunakan FreeRTOS. Sistem menggunakan autentikasi PIN 4-digit dengan input 2-button, menampilkan status melalui OLED display, mengontrol kunci pintu dengan servo motor, dan memberikan feedback audio-visual melalui LED dan buzzer.

---

## Komponen yang Digunakan

| Komponen | Fungsi |
|---|---|
| ESP32-S3 Dev Board | Proses utama, menjalankan multitasking RTOS |
| Servo Motor (SG90) | Mengunci dan membuka pintu (0°=terkunci, 90°=terbuka) |
| OLED Display (SSD1306 128x64) | Menampilkan status sistem dan input PIN |
| Buzzer | Memberikan feedback audio untuk setiap aksi |
| 2 Tombol Push Button | BUTTON_UP (increment digit), BUTTON_DOWN (decrement digit) |
| 2 LED (Merah & Hijau) | Indikator status akses (granted/denied) |

---

# GPIO Mapping

| Fungsi | GPIO |
|---|---|
| BUTTON_UP | 6 |
| BUTTON_DOWN | 7 |
| SERVO_PWM | 2 |
| BUZZER | 3 |
| LED_RED (Access Denied) | 4 |
| LED_GREEN (Access Granted) | 5 |
| OLED_SDA | 8 |
| OLED_SCL | 9 |

---

# Fitur Utama

- **PIN Authentication** - Sistem keamanan 4-digit PIN (default: 1-2-3-4)
- **2-Button Input** - Intuitif UP/DOWN control untuk memilih digit (0-9)
- **Auto-Confirm** - Digit otomatis tersimpan setelah stabil 3 detik
- **Real-time Feedback** - OLED display menunjukkan progres PIN dan countdown
- **Multitasking RTOS** - Tiga task concurrent: Input Processing, Display Update, PIN Validation
- **Resource Protection** - Mutex untuk mencegah konflik akses hardware
- **System Recovery** - Reset otomatis setelah 4 detik menunjukkan hasil

---

# Arsitektur Komunikasi Antar Task

## 1. Queue — Komunikasi Data ke Display Task
- Task Input mengirim data digit dan status ke displayQueue
- Task Display membaca dari queue dan update OLED
- Memastikan update tampilan yang smooth dan teratur

## 2. Semaphore — Sinkronisasi Event
- GPIO Interrupt memberikan semaphore saat button ditekan
- Task Input menunggu semaphore untuk memproses input
- Auto-confirm timer memberikan semaphore ke Task Validator

## 3. Mutex — Proteksi Resource Shared
- Servo motor, LED, dan buzzer dilindungi mutex
- Mencegah multiple task mengakses hardware bersamaan
- Memastikan operasi servo yang aman dan stabil

## 4. Interrupt — Respons Input Cepat
- Button UP/DOWN menggunakan GPIO interrupt
- Immediate response tanpa polling delay
- ISR memberikan semaphore ke task processing

---

# Input
- **BUTTON_UP** → Increment digit (0→1→2...→9→0)
- **BUTTON_DOWN** → Decrement digit (9→8→7...→0→9)

## Output
- **Servo Motor** → 0° (terkunci) / 90° (terbuka)
- **Buzzer** → Short beep (digit change), Long beep (confirm), High tone (granted), Low tone (denied)
- **LED** → Hijau = access granted, Merah = access denied
- **OLED Display** → Menampilkan digit aktif, progres PIN, countdown, dan status
- **Serial Monitor** → Debug semua aktivitas sistem

---

# Cara Kerja Sistem

1. **Pilih Digit** → Tekan UP/DOWN untuk pilih digit 0-9
2. **Auto-Confirm** → Tunggu 3 detik stabil → digit tersimpan
3. **Ulangi 4x** → Pilih 4 digit untuk membentuk PIN lengkap
4. **Validasi Otomatis** → Sistem cek PIN (1-2-3-4 untuk granted)
5. **Granted** → Servo buka (90°), LED hijau, buzzer tinggi
6. **Denied** → Servo tetap kunci (0°), LED merah, buzzer rendah
7. **Reset Otomatis** → Sistem reset setelah 4 detik, siap untuk input baru

---

# Implementasi RTOS Concepts

 *Semaphore
- Binary semaphore untuk sinkronisasi button press
- Task signaling dari ISR ke processing task

*Interrupt
- GPIO interrupt untuk button UP/DOWN
- Immediate response tanpa task blocking

*Queue
- Data communication ke display task
- Update OLED dengan data terbaru

*Mutex
- Resource protection untuk servo, LED, buzzer
- Thread-safe hardware access

*Multicore Ready
- Architecture mendukung multicore processing
- Task dapat diassign ke core berbeda untuk optimasi
