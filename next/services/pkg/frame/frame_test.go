package frame

import (
	"bytes"
	"testing"
)

// Contract vector shared with C++ (tests/test_frame.cpp) and GDScript (tests/test_net.gd).
func TestEncodeIsLittleEndian(t *testing.T) {
	got := Encode(1001, 0, []byte("hi"))
	want := []byte{0x06, 0x00, 0x00, 0x00, 0xE9, 0x03, 0x00, 0x00, 'h', 'i'}
	if !bytes.Equal(got, want) {
		t.Fatalf("got % x want % x", got, want)
	}
	got = Encode(0x1234, 0x0003, nil)
	want = []byte{0x04, 0x00, 0x00, 0x00, 0x34, 0x12, 0x03, 0x00}
	if !bytes.Equal(got, want) {
		t.Fatalf("got % x want % x", got, want)
	}
}

func TestParserSplitsAtAnyPoint(t *testing.T) {
	var stream []byte
	stream = Append(stream, 1, 0, []byte("one"))
	stream = Append(stream, 2, 0, nil)
	stream = Append(stream, 3, FlagCompressed, []byte("three"))

	for chunk := 1; chunk <= len(stream); chunk++ {
		p := NewParser(MaxInternalPayload)
		var got []Frame
		for pos := 0; pos < len(stream); pos += chunk {
			end := min(pos+chunk, len(stream))
			p.Feed(stream[pos:end])
			for {
				f, ok, err := p.Next()
				if err != nil {
					t.Fatal(err)
				}
				if !ok {
					break
				}
				got = append(got, f)
			}
		}
		if len(got) != 3 || got[0].MsgID != 1 || string(got[0].Payload) != "one" || got[1].MsgID != 2 || len(got[1].Payload) != 0 ||
			got[2].MsgID != 3 || string(got[2].Payload) != "three" || got[2].Flags != FlagCompressed {
			t.Fatalf("chunk %d: got %+v", chunk, got)
		}
		if p.Buffered() != 0 {
			t.Fatalf("chunk %d: leftover bytes", chunk)
		}
	}
}

func TestParserRejectsBadFrames(t *testing.T) {
	p := NewParser(16)
	p.Feed(Encode(7, 0, bytes.Repeat([]byte("x"), 17)))
	if _, _, err := p.Next(); err != ErrTooLarge {
		t.Fatalf("want ErrTooLarge got %v", err)
	}
	p = NewParser(16)
	p.Feed([]byte{0x02, 0x00, 0x00, 0x00, 0x00, 0x00})
	if _, _, err := p.Next(); err != ErrCorrupt {
		t.Fatalf("want ErrCorrupt got %v", err)
	}
	p = NewParser(16)
	p.Feed([]byte{0x06, 0x00, 0x00})
	if _, ok, err := p.Next(); ok || err != nil || p.Buffered() != 3 {
		t.Fatal("partial frame must wait")
	}
}

func TestReaderRoundTrip(t *testing.T) {
	var buf bytes.Buffer
	for i := 0; i < 100; i++ {
		if err := Write(&buf, uint16(i), 0, []byte("payload")); err != nil {
			t.Fatal(err)
		}
	}
	r := NewReader(&buf, MaxClientPayload)
	for i := 0; i < 100; i++ {
		f, err := r.Read()
		if err != nil {
			t.Fatal(err)
		}
		if f.MsgID != uint16(i) || string(f.Payload) != "payload" {
			t.Fatalf("frame %d wrong: %+v", i, f)
		}
	}
	if _, err := r.Read(); err == nil {
		t.Fatal("expected EOF")
	}
}
