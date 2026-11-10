# Blatt 2 – Giải thích bằng tiếng Việt (giữ thuật ngữ tiếng Đức)

Tài liệu chính bằng tiếng Đức nằm ở [`README.md`](README.md). Các lệnh bên dưới chạy từ thư mục `Aufgabe2/`.

## 1. Blatt 2 khác Blatt 1 ở đâu?

- **Blatt 1:** chương trình tính toàn bộ Game of Life trên một máy.
- **Blatt 2:** `BoardServer` giữ **Spielfeld**, còn từng `LifeClient` chỉ tính một **Spielfeldausschnitt**. Mọi thao tác đọc (`READ`) và ghi (`WRITE`) trạng thái ô đều đi qua mạng.
- Cùng một chương trình phải chạy bằng **UDP (Teil a)** và **TCP (Teil b)**.
- Kết quả cuối phải khớp với phiên bản **Local** từ Blatt 1. Có đo **Laufzeit**.

## 2. Các khái niệm cần hiểu

| Begriff | Giải thích |
|---|---|
| `Client-Server-Kommunikation` | Client gửi Anfrage, Server xử lý và gửi Antwort. |
| `Synchrone Kommunikation` | Client đợi Antwort trước khi đi tiếp. |
| `Diskriminator` | Trường `type` cho biết Nachricht thuộc loại `LOGON`, `READ`, `WRITE`, `BARRIER`, ... |
| `Sequenznummer` | Mã số của Request; Server giữ nguyên số đó trong Response. Client bỏ qua Response đến muộn. |
| `Timeout` | Hết thời gian đợi UDP mà không nhận Response thì gửi lại đúng Request cũ. |
| `Barriere / Barrierensynchronisation` | Phải có đủ mọi Client xong một Zeitschritt mới chuyển sang Zeitschritt tiếp theo. |
| `Double Buffering` | `current_` chỉ để đọc, `next_` chỉ để ghi, xong một bước mới đổi hai Board. |
| `toroidales Spielfeld` | Cạnh trái nối cạnh phải, cạnh trên nối cạnh dưới, giống Blatt 1. |

## 3. Vì sao UDP và TCP có cách xử lý Barrier khác nhau?

**UDP:** Khi một Client gửi `BARRIER`, Server có thể *giữ Response lại* đến khi đủ các Client. Khi tất cả đến, Server gửi `RESPONSE` cho từng Client. Nếu Response bị mất, Client gửi lại Request với `sequence` cũ; Server biết Zeitschritt đã xong để trả lời lại.

**TCP:** Theo đề, mỗi Request phải có một **neue Verbindung** rồi đóng ngay sau Response. Vì vậy Server trả `WAIT` nếu chưa đủ Client, đóng Verbindung; Client đợi khoảng 10 ms rồi hỏi lại bằng một Verbindung mới. Khi đủ Client, Server trả `OK` để tính tiếp hoặc `FINISHED` khi kết thúc.

## 4. Chạy thử

Đầu tiên mở Terminal và biên dịch:

```bash
cd Aufgabe2
make
```

Mở ba Terminal (đều đứng trong `Aufgabe2/`):

```bash
# Terminal 1: Server
./bin/server --protocol udp --clients 2 --steps 10 --input boards/glider.rle --output udp.rle

# Terminal 2: Client thứ nhất
./bin/client --protocol udp --host 127.0.0.1

# Terminal 3: Client thứ hai
./bin/client --protocol udp --host 127.0.0.1
```

Đổi `--protocol udp` thành `--protocol tcp` ở cả Server và Client để chạy Teil b. Nên dùng `--port 7655` cho cả 3 cửa sổ khi chạy TCP và `--output tcp.rle`.

Kiểm tra với phiên bản Local:

```bash
./bin/local --input boards/glider.rle --steps 10 --output local.rle
cmp local.rle udp.rle
cmp local.rle tcp.rle
make test
```

Lệnh `cmp` không in ra gì nghĩa là hai file giống nhau; `make test` sẽ kiểm thử tự động UDP/TCP với 1 và 3 Client.

## 5. Nên đọc Code theo thứ tự nào?

1. `include/net/Protocol.h`: định dạng Message và Nachrichtentyp.
2. `include/net/IPNetwork.h`: Interface dùng chung.
3. `src/net/UDPNetwork.cc`: `socket()`, `sendto()`, `recvfrom()`, `select()`, Timeout.
4. `src/board/BoardServer.cc`: `logon`, `READ`, `WRITE`, Barrier.
5. `src/client/LifeClient.cc`: cách đọc 9 trạng thái, tính 8 Nachbarn và ghi kết quả.
6. `src/net/TCPNetwork.cc`: `connect`, `accept`, `send`, `recv`, `close`.
7. `src/server.cc`, `src/client.cc`, `src/local.cc`: cách chạy chương trình.

**Lưu ý:** Đây là bản học tập theo yêu cầu của Blatt 2, chưa thiết kế cho môi trường mạng thực tế có lỗi/phá kết nối kéo dài. Các Test đã chạy trên Linux, chưa xác nhận build trực tiếp trên macOS.
