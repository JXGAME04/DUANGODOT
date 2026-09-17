package spr

import (
	"bytes"
	"compress/zlib"
	"io"
)

func inflate(src []byte, size int) ([]byte, error) {
	r, err := zlib.NewReader(bytes.NewReader(src))
	if err != nil {
		return nil, err
	}
	defer r.Close()
	out := make([]byte, size)
	n, err := io.ReadFull(r, out)
	if err != nil && err != io.ErrUnexpectedEOF {
		return nil, err
	}
	return out[:n], nil
}
