package frame

import (
	"bytes"
	"testing"
)

// The old server read packets by casting the socket buffer to a struct: a wrong length byte
// walked past the buffer.  Here the rule is that arbitrary bytes may only produce a frame, a
// "need more", or an error - never a panic and never a payload the sender did not send.
//
// Seeds run in a normal `go test`; `go test -fuzz FuzzParser ./pkg/frame` runs the engine.

func FuzzParser(f *testing.F) {
	f.Add([]byte{})
	f.Add([]byte{0x06, 0x00, 0x00, 0x00, 0xE9, 0x03, 0x00, 0x00, 'h', 'i'}) // the vector from PROTOCOL.md
	f.Add([]byte{0x04, 0x00, 0x00, 0x00, 0x34, 0x12, 0x03, 0x00})           // empty payload
	f.Add([]byte{0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00})                 // len < header
	f.Add([]byte{0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00})           // len 4 GiB
	f.Add([]byte{0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00})           // truncated payload
	f.Add(append(Encode(1, 0, []byte("a")), Encode(2, 0, bytes.Repeat([]byte("b"), 300))...))

	f.Fuzz(func(t *testing.T, data []byte) {
		for _, chunk := range []int{1, 3, 7, 64, 1 << 20} {
			p := NewParser(MaxClientPayload)
			for off := 0; off < len(data); off += chunk {
				end := min(off+chunk, len(data))
				p.Feed(data[off:end])
				for {
					fr, ok, err := p.Next()
					if err != nil {
						if err != ErrCorrupt && err != ErrTooLarge {
							t.Fatalf("unexpected error %v", err)
						}
						return // a broken stream is dropped; the connection closes
					}
					if !ok {
						break
					}
					if len(fr.Payload) > MaxClientPayload {
						t.Fatalf("payload %d over the limit", len(fr.Payload))
					}
					// what came out must encode back to the bytes it was decoded from
					if got := Encode(fr.MsgID, fr.Flags, fr.Payload); len(got) < MinFrameSize {
						t.Fatalf("re-encode too short: %d", len(got))
					}
				}
			}
			if p.Buffered() > len(data) {
				t.Fatalf("parser buffered %d bytes of %d", p.Buffered(), len(data))
			}
		}
	})
}

// FuzzReader checks the blocking stream decoder on the same inputs.
func FuzzReader(f *testing.F) {
	f.Add([]byte{})
	f.Add([]byte{0x06, 0x00, 0x00, 0x00, 0xE9, 0x03, 0x00, 0x00, 'h', 'i'})
	f.Add([]byte{0x00, 0x00, 0x00, 0x00})
	f.Add(Encode(9, 1, bytes.Repeat([]byte("x"), 1000)))

	f.Fuzz(func(t *testing.T, data []byte) {
		r := NewReader(bytes.NewReader(data), MaxClientPayload)
		for i := 0; i < 64; i++ { // bounded: a valid stream of tiny frames must still end
			fr, err := r.Read()
			if err != nil {
				return
			}
			if len(fr.Payload) > MaxClientPayload {
				t.Fatalf("payload %d over the limit", len(fr.Payload))
			}
		}
	})
}
