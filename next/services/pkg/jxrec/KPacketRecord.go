// Package jxrec reads and writes .jxrec files: everything that crossed a TCP connection, in
// order, with the time and the direction.  The roadmap asks for this early (priority 6, "packet
// recorder on the old system"): a recording of the live game is the evidence the new server is
// checked against, and it can be collected long before the new code is finished.
//
// The format knows nothing about the payload, so it records the old game (its own obfuscated
// stream) exactly as well as Protocol V2.
//
//	"JXREC1\n"                       magic
//	<json header line>"\n"           {"started_ms":…, "listen":…, "target":…, "note":…}
//	record*                          u8 dir | u32 ms since start | u32 length | bytes
//
// Direction is from the client's point of view: 0 = client -> server, 1 = server -> client.
// Everything is little-endian, like the wire format in docs/PROTOCOL.md.
package jxrec

import (
	"bufio"
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
	"sync"
	"time"
)

const (
	Magic        = "JXREC1\n"
	DirClient    = 0 // client -> server
	DirServer    = 1 // server -> client
	MaxChunk     = 16 * 1024 * 1024
	headerMaxLen = 64 * 1024
)

var (
	ErrBadMagic = errors.New("jxrec: not a .jxrec file")
	ErrCorrupt  = errors.New("jxrec: corrupt record")
)

// Header describes one recorded connection.
type Header struct {
	StartedMs int64  `json:"started_ms"`
	Listen    string `json:"listen,omitempty"` // where the proxy listened
	Target    string `json:"target,omitempty"` // the server the traffic went to
	Client    string `json:"client,omitempty"` // remote address of the recorded client
	Note      string `json:"note,omitempty"`   // free text: which build, which scenario
	Tool      string `json:"tool,omitempty"`
}

// Record is one chunk of bytes as it crossed the connection.
type Record struct {
	Dir  byte
	Ms   uint32 // milliseconds since Header.StartedMs
	Data []byte
}

func (r Record) DirString() string {
	if r.Dir == DirClient {
		return "c2s"
	}
	return "s2c"
}

// Writer appends records to a file.  It is safe for the two directions to write concurrently.
type Writer struct {
	mu    sync.Mutex
	w     *bufio.Writer
	f     *os.File
	start time.Time
	n     int
	bytes int64
}

// Create starts a new recording at path.
func Create(path string, h Header) (*Writer, error) {
	f, err := os.Create(path)
	if err != nil {
		return nil, err
	}
	start := time.Now()
	if h.StartedMs == 0 {
		h.StartedMs = start.UnixMilli()
	} else {
		start = time.UnixMilli(h.StartedMs)
	}
	if h.Tool == "" {
		h.Tool = "jxrecord"
	}
	line, err := json.Marshal(h)
	if err != nil {
		_ = f.Close()
		return nil, err
	}
	w := bufio.NewWriterSize(f, 64*1024)
	if _, err := w.WriteString(Magic); err != nil {
		_ = f.Close()
		return nil, err
	}
	if _, err := w.Write(append(line, '\n')); err != nil {
		_ = f.Close()
		return nil, err
	}
	return &Writer{w: w, f: f, start: start}, nil
}

// Write appends one chunk.  An empty chunk is ignored.
func (w *Writer) Write(dir byte, data []byte) error {
	if len(data) == 0 {
		return nil
	}
	if len(data) > MaxChunk {
		return fmt.Errorf("jxrec: chunk of %d bytes over the limit", len(data))
	}
	var hdr [9]byte
	hdr[0] = dir
	binary.LittleEndian.PutUint32(hdr[1:], uint32(time.Since(w.start).Milliseconds()))
	binary.LittleEndian.PutUint32(hdr[5:], uint32(len(data)))
	w.mu.Lock()
	defer w.mu.Unlock()
	if _, err := w.w.Write(hdr[:]); err != nil {
		return err
	}
	if _, err := w.w.Write(data); err != nil {
		return err
	}
	w.n++
	w.bytes += int64(len(data))
	return nil
}

// Stats returns how many records and payload bytes were written.
func (w *Writer) Stats() (records int, bytes int64) {
	w.mu.Lock()
	defer w.mu.Unlock()
	return w.n, w.bytes
}

// Flush pushes buffered records to the file (the recorder calls it regularly so a crash of the
// recorded system still leaves a usable file).
func (w *Writer) Flush() error {
	w.mu.Lock()
	defer w.mu.Unlock()
	return w.w.Flush()
}

func (w *Writer) Close() error {
	w.mu.Lock()
	defer w.mu.Unlock()
	if err := w.w.Flush(); err != nil {
		_ = w.f.Close()
		return err
	}
	return w.f.Close()
}

// Reader walks the records of a file.
type Reader struct {
	r      *bufio.Reader
	c      io.Closer
	Header Header
}

// Open reads the header and leaves the reader at the first record.
func Open(path string) (*Reader, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	r, err := NewReader(f)
	if err != nil {
		_ = f.Close()
		return nil, err
	}
	r.c = f
	return r, nil
}

// NewReader reads a recording from any stream.
func NewReader(in io.Reader) (*Reader, error) {
	br := bufio.NewReaderSize(in, 64*1024)
	magic := make([]byte, len(Magic))
	if _, err := io.ReadFull(br, magic); err != nil {
		return nil, ErrBadMagic
	}
	if string(magic) != Magic {
		return nil, ErrBadMagic
	}
	line, err := br.ReadBytes('\n')
	if err != nil || len(line) > headerMaxLen {
		return nil, ErrCorrupt
	}
	r := &Reader{r: br}
	if err := json.Unmarshal(line[:len(line)-1], &r.Header); err != nil {
		return nil, fmt.Errorf("jxrec: header: %w", err)
	}
	return r, nil
}

// Next returns the next record; io.EOF marks the end of the file.
func (r *Reader) Next() (Record, error) {
	var hdr [9]byte
	if _, err := io.ReadFull(r.r, hdr[:]); err != nil {
		if errors.Is(err, io.ErrUnexpectedEOF) {
			return Record{}, ErrCorrupt // the recorder died mid-record
		}
		return Record{}, err
	}
	if hdr[0] > DirServer {
		return Record{}, ErrCorrupt
	}
	n := binary.LittleEndian.Uint32(hdr[5:])
	if n == 0 || n > MaxChunk {
		return Record{}, ErrCorrupt
	}
	rec := Record{Dir: hdr[0], Ms: binary.LittleEndian.Uint32(hdr[1:]), Data: make([]byte, n)}
	if _, err := io.ReadFull(r.r, rec.Data); err != nil {
		return Record{}, ErrCorrupt
	}
	return rec, nil
}

// All reads the whole file (small recordings, tests).
func (r *Reader) All() ([]Record, error) {
	var out []Record
	for {
		rec, err := r.Next()
		if errors.Is(err, io.EOF) {
			return out, nil
		}
		if err != nil {
			return out, err
		}
		out = append(out, rec)
	}
}

func (r *Reader) Close() error {
	if r.c != nil {
		return r.c.Close()
	}
	return nil
}
