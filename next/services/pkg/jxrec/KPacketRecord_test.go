package jxrec

import (
	"bytes"
	"errors"
	"io"
	"os"
	"path/filepath"
	"testing"
)

func TestRecordRoundTrip(t *testing.T) {
	path := filepath.Join(t.TempDir(), "session.jxrec")
	w, err := Create(path, Header{Listen: ":7100", Target: "10.0.0.5:5600", Note: "Rainbow cũ, đăng nhập"})
	if err != nil {
		t.Fatal(err)
	}
	if err := w.Write(DirClient, []byte("login")); err != nil {
		t.Fatal(err)
	}
	if err := w.Write(DirServer, []byte{0x00, 0xFF, 0x10}); err != nil {
		t.Fatal(err)
	}
	if err := w.Write(DirClient, nil); err != nil { // empty chunks are skipped, not recorded
		t.Fatal(err)
	}
	n, bytesWritten := w.Stats()
	if n != 2 || bytesWritten != 8 {
		t.Fatalf("stats %d %d", n, bytesWritten)
	}
	if err := w.Close(); err != nil {
		t.Fatal(err)
	}

	r, err := Open(path)
	if err != nil {
		t.Fatal(err)
	}
	defer r.Close()
	if r.Header.Target != "10.0.0.5:5600" || r.Header.Note != "Rainbow cũ, đăng nhập" || r.Header.StartedMs == 0 || r.Header.Tool != "jxrecord" {
		t.Fatalf("header %+v", r.Header)
	}
	recs, err := r.All()
	if err != nil {
		t.Fatal(err)
	}
	if len(recs) != 2 {
		t.Fatalf("%d records", len(recs))
	}
	if recs[0].Dir != DirClient || string(recs[0].Data) != "login" || recs[0].DirString() != "c2s" {
		t.Fatalf("record 0: %+v", recs[0])
	}
	if recs[1].Dir != DirServer || !bytes.Equal(recs[1].Data, []byte{0x00, 0xFF, 0x10}) || recs[1].DirString() != "s2c" {
		t.Fatalf("record 1: %+v", recs[1])
	}
}

func TestReaderRejectsBrokenFiles(t *testing.T) {
	dir := t.TempDir()
	bad := filepath.Join(dir, "bad.jxrec")
	if err := os.WriteFile(bad, []byte("not a recording"), 0o600); err != nil {
		t.Fatal(err)
	}
	if _, err := Open(bad); !errors.Is(err, ErrBadMagic) {
		t.Fatalf("bad magic: %v", err)
	}
	if err := os.WriteFile(bad, []byte(Magic+"{not json}\n"), 0o600); err != nil {
		t.Fatal(err)
	}
	if _, err := Open(bad); err == nil {
		t.Fatal("bad header accepted")
	}

	// a recording cut short (the recorded process crashed) reads up to the last whole record
	good := filepath.Join(dir, "cut.jxrec")
	w, _ := Create(good, Header{})
	_ = w.Write(DirClient, []byte("first"))
	_ = w.Write(DirServer, bytes.Repeat([]byte("x"), 100))
	_ = w.Close()
	raw, _ := os.ReadFile(good)
	if err := os.WriteFile(good, raw[:len(raw)-50], 0o600); err != nil {
		t.Fatal(err)
	}
	r, err := Open(good)
	if err != nil {
		t.Fatal(err)
	}
	defer r.Close()
	first, err := r.Next()
	if err != nil || string(first.Data) != "first" {
		t.Fatalf("first record: %v %+v", err, first)
	}
	if _, err := r.Next(); !errors.Is(err, ErrCorrupt) {
		t.Fatalf("truncated record: %v", err)
	}
}

func TestReaderStopsAtEOF(t *testing.T) {
	var buf bytes.Buffer
	buf.WriteString(Magic)
	buf.WriteString("{}\n")
	r, err := NewReader(&buf)
	if err != nil {
		t.Fatal(err)
	}
	if _, err := r.Next(); !errors.Is(err, io.EOF) {
		t.Fatalf("empty recording: %v", err)
	}
}
