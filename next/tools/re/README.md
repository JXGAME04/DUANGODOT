# Bộ công cụ mổ nhị phân client (Windows, thuần Python)

Dùng để trả lời câu hỏi "client 2.0 **thật sự** làm gì" bằng bằng chứng, không đoán. Cần
`pip install capstone pillow`. Mọi công cụ chỉ **đọc**: thứ duy nhất ghi vào tiến trình game là
byte breakpoint `INT3` của `filetrace.py`.

| Công cụ | Việc |
|---|---|
| `re_upx.py <exe>` | **Bộ mở mới (2026-09-18)**: giải nén UPX (NRV2E) + gỡ bộ lọc `call` + **dựng lại bảng import ngay từ stub nạp** (không cần tiến trình sống) → `<exe>.unpacked.img` + `.json` mà **`re_elf.py` / `re_calls.py` / `re_scan.py` / `re_tabdesc.py` đọc như một ELF** (đồ thị gọi hàm, quét thành viên, prologue MSVC). Dùng cho `KItem::GetDesc` ([`docs/CLIENT-2.0.md`](../../docs/CLIENT-2.0.md)). |
| `upx_unpack.py <exe> <out.bin>` | Bộ mở cũ hơn (cùng thuật toán, không import): ảnh thô cho `re_pe.py`. |
| `re_pe.py <img> sections\|xref\|xrefstr\|dis\|func` | Đọc ảnh của `upx_unpack.py`: bảng section gốc, ai nạp một địa chỉ/chuỗi, dịch ngược có chú thích chuỗi GBK. |
| `re_dll.py <dll> exports\|dis` | DLL không nén (`engineFree.dll`): bảng export, dịch ngược theo tên export, chú thích import/chuỗi. |
| `imgstr.py <img> near\|grep` | Chuỗi trong ảnh theo vùng địa chỉ hoặc theo regex. |
| `callers.py`, `iatcalls.py` | Ai gọi một hàm (`call rel32`, con trỏ bảng ảo) / ai gọi một ô import. |
| `proc_iat.py <exe> --range lo hi --dump out.json` | UPX dựng lại bảng import lúc chạy → đọc các ô import từ **tiến trình đang sống** rồi gọi tên chúng (970 ô của `gamecl.exe`). |
| `proc_files.py <exe>` | Tệp và module mà tiến trình đang mở. |
| `filetrace.py <exe> <engine.dll> <giây> <out.tsv>` | Chạy game dưới debug API, đặt breakpoint trong `KPakFile::Open` của engine: ghi lại **tên từng tệp game xin mở và nó được tìm thấy ở đâu** (đĩa / kho thứ mấy, id, vị trí, cỡ / bộ nhớ / không có). |
| `shot_game.py <exe> <prefix>` | Mở game, chụp **đúng cửa sổ của nó** bằng `PrintWindow` (không chụp gì khác trên màn hình), rồi tắt. |

## Quy trình đã dùng để tìm ra `\reslst.dat`

```bash
python upx_unpack.py ".../gamecl.exe" gamecl.bin
python re_pe.py gamecl.bin xrefstr "UiNewLogin"          # 14 tệp .ini mà exe gọi tên
python filetrace.py ".../gamecl.exe" enginefree.dll 45 trace.tsv
../../build/go/jxassets -client ".../Vo Lam Truyen Ky 2.0" check-trace trace.tsv
```

`check-trace` so từng tệp game tìm thấy với bộ đọc của ta. Lần đầu: **225/490 tệp game thấy mà ta
không thấy** — tất cả nằm ở "kho #13", một kho thứ 14 không có trên đĩa. Đó là `\reslst.dat`, xem
[`docs/VLTK20-CLIENT.md`](../../docs/VLTK20-CLIENT.md). Sau khi sửa: khớp 503, thiếu 0, thừa 0.

Offset breakpoint trong `filetrace.py` là của `engineFree.dll` bản 2.0 (PDB
`D:\newBuilder\projects\jxvn20\...\engineFree.pdb`). Engine khác thì tìm lại bằng
`re_dll.py <dll> dis "?Open@KPakFile@@QAEHPBD@Z"`.

Lưu ý khi viết thêm công cụ: công cụ Bash ở máy này rút `\\` thành `\` trong heredoc, nên **đừng
viết Python có dấu `\` qua heredoc** — viết tệp bằng công cụ Write rồi mới chạy.
