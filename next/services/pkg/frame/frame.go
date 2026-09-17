// Package frame implements the JX NEXT wire framing (next/docs/PROTOCOL.md), byte-compatible
// with jx::frame (C++) and Net.gd (Godot):
//
//	u32 len | u16 msg | u16 flags | payload      (little-endian; len counts msg+flags+payload)
package frame

import (
	"bufio"
	"encoding/binary"
	"errors"
	"io"
)

const (
	LengthSize         = 4
	HeaderSize         = 4
	MinFrameSize       = LengthSize + HeaderSize
	MaxClientPayload   = 64 * 1024
	MaxInternalPayload = 4 * 1024 * 1024

	FlagCompressed uint16 = 0x0001
	FlagEncrypted  uint16 = 0x0002
)

var (
	ErrTooLarge = errors.New("frame: payload exceeds limit")
	ErrCorrupt  = errors.New("frame: corrupt header")
)

// Frame is one decoded message.
type Frame struct {
	MsgID   uint16
	Flags   uint16
	Payload []byte
}

// Append encodes one frame at the end of dst and returns the extended slice.
func Append(dst []byte, msgID, flags uint16, payload []byte) []byte {
	var hdr [MinFrameSize]byte
	binary.LittleEndian.PutUint32(hdr[0:], uint32(HeaderSize+len(payload)))
	binary.LittleEndian.PutUint16(hdr[4:], msgID)
	binary.LittleEndian.PutUint16(hdr[6:], flags)
	dst = append(dst, hdr[:]...)
	return append(dst, payload...)
}

// Encode returns a new buffer holding one frame.
func Encode(msgID, flags uint16, payload []byte) []byte {
	return Append(make([]byte, 0, MinFrameSize+len(payload)), msgID, flags, payload)
}

// Write encodes and writes one frame with a single Write call.
func Write(w io.Writer, msgID, flags uint16, payload []byte) error {
	_, err := w.Write(Encode(msgID, flags, payload))
	return err
}

// Parser is the incremental decoder used when bytes arrive in arbitrary chunks.
type Parser struct {
	buf        []byte
	maxPayload uint32
}

// NewParser creates a parser that rejects payloads larger than maxPayload.
func NewParser(maxPayload uint32) *Parser {
	return &Parser{maxPayload: maxPayload}
}

// Feed appends received bytes.
func (p *Parser) Feed(b []byte) { p.buf = append(p.buf, b...) }

// Buffered reports how many bytes are waiting.
func (p *Parser) Buffered() int { return len(p.buf) }

// Reset drops buffered bytes.
func (p *Parser) Reset() { p.buf = p.buf[:0] }

// Next returns the next complete frame.  ok is false when more bytes are needed.  The
// payload is a copy, so it stays valid after further Feed calls.
func (p *Parser) Next() (f Frame, ok bool, err error) {
	if len(p.buf) < LengthSize {
		return Frame{}, false, nil
	}
	n := binary.LittleEndian.Uint32(p.buf[0:])
	if n < HeaderSize {
		return Frame{}, false, ErrCorrupt
	}
	if n-HeaderSize > p.maxPayload {
		return Frame{}, false, ErrTooLarge
	}
	total := LengthSize + int(n)
	if len(p.buf) < total {
		return Frame{}, false, nil
	}
	f.MsgID = binary.LittleEndian.Uint16(p.buf[4:])
	f.Flags = binary.LittleEndian.Uint16(p.buf[6:])
	f.Payload = append([]byte(nil), p.buf[MinFrameSize:total]...)
	p.buf = append(p.buf[:0], p.buf[total:]...)
	return f, true, nil
}

// Reader decodes frames from a stream; used by connection read loops.
type Reader struct {
	r          *bufio.Reader
	maxPayload uint32
	hdr        [MinFrameSize]byte
}

// NewReader wraps r with the buffer a real connection deserves.
func NewReader(r io.Reader, maxPayload uint32) *Reader {
	return NewReaderSize(r, maxPayload, 64*1024)
}

// NewReaderSize is NewReader with a chosen buffer size.  A load test holding thousands of
// connections in one process pays this per connection, so it asks for a small one; the buffer only
// has to be big enough to keep syscalls rare, never to hold a whole frame.
func NewReaderSize(r io.Reader, maxPayload uint32, bufSize int) *Reader {
	if bufSize < MinFrameSize {
		bufSize = MinFrameSize
	}
	return &Reader{r: bufio.NewReaderSize(r, bufSize), maxPayload: maxPayload}
}

// Read blocks until one frame is available or the stream fails.
func (r *Reader) Read() (Frame, error) {
	if _, err := io.ReadFull(r.r, r.hdr[:]); err != nil {
		return Frame{}, err
	}
	n := binary.LittleEndian.Uint32(r.hdr[0:])
	if n < HeaderSize {
		return Frame{}, ErrCorrupt
	}
	if n-HeaderSize > r.maxPayload {
		return Frame{}, ErrTooLarge
	}
	f := Frame{
		MsgID:   binary.LittleEndian.Uint16(r.hdr[4:]),
		Flags:   binary.LittleEndian.Uint16(r.hdr[6:]),
		Payload: make([]byte, n-HeaderSize),
	}
	if _, err := io.ReadFull(r.r, f.Payload); err != nil {
		return Frame{}, err
	}
	return f, nil
}
