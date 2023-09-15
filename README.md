# Monitoring Kualitas Udara dengan ESP32 + DSM501a + DHT22

![ESP32](https://github.com/Anemo27/Sistem_Monitoring_Udara/raw/master/images/esp32.jpg)

## Deskripsi Proyek
Proyek ini adalah sistem pemantauan kualitas udara menggunakan mikrokontroler ESP32. Proyek ini mengukur berbagai parameter seperti suhu, kelembaban, konsentrasi karbon monoksida (CO), dan partikel PM2.5 dalam udara. Ketika kualitas udara tidak baik, sistem akan memberikan notifikasi ke Telegram.

## Komponen Proyek
- Mikrokontroler ESP32
- Sensor DHT22 untuk suhu dan kelembaban
- Sensor MQ-7 untuk konsentrasi karbon monoksida (CO)
- Sensor DSM501a untuk partikel PM2.5
- Library Universal Telegram Bot untuk mengirim notifikasi ke Telegram

## Konfigurasi
1. Pasang sensor-sensor sesuai dengan koneksi yang benar.
2. Upload kode proyek ini ke ESP32.
3. Pastikan Anda memiliki token bot Telegram dan chat ID penerima notifikasi.
4. Gantilah nilai `BOT_TOKEN` dan `CHAT_ID` dalam kode proyek dengan nilai yang sesuai.

## Cara Menggunakan
1. Hubungkan ESP32 ke sumber daya dan jaringan WiFi.
2. ESP32 akan mulai mengukur parameter kualitas udara secara periodik.
3. Jika kualitas udara tidak baik (kadar PM2.5 di atas ambang batas yang ditentukan), sistem akan mengirimkan notifikasi ke Telegram.
4. Jika kualitas udara membaik, sistem akan berhenti mengirim notifikasi sampai kondisi kembali tidak baik.

## Kontribusi
Kami sangat menghargai kontribusi dari komunitas. Jika Anda ingin berkontribusi pada proyek ini, silakan buka *issue* atau *pull request* di repositori ini.

## Lisensi
Proyek ini dilisensikan di bawah [Lisensi MIT](LICENSE).

## Penulis
Dibuat oleh [ANeMo](https://github.com/Anemo27)

