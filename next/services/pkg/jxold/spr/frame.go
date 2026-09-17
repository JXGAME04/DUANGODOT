package spr

import (
	"encoding/binary"
	"errors"
	"fmt"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxold/pak"
)

// ReadFromPak decodes a sprite stored in an archive, handling the frame-compressed layouts
// (XPackFile::GetSprHeader / GetSprFrame) as well as plain entries.
func ReadFromPak(p *pak.File, e pak.Entry) (*Sprite, error) {
	if !e.IsFrame() {
		data, err := p.Read(e)
		if err != nil {
			return nil, err
		}
		return Decode(data)
	}
	head, err := p.ReadRaw(e.Offset, HeaderSize+8)
	if err != nil {
		return nil, err
	}
	// "TF" layout: [u32 count][u32 frameTableOffset][SPRHEAD]...
	if binary.LittleEndian.Uint32(head[8:])&0x00ffffff == magicSPR {
		return readFrameTF(p, e, head)
	}
	if binary.LittleEndian.Uint32(head[0:])&0x00ffffff == magicSPR {
		return readFrameOld(p, e)
	}
	return nil, errors.New("spr: frame entry without SPR magic")
}

func readFrameTF(p *pak.File, e pak.Entry, head []byte) (*Sprite, error) {
	tabOff := binary.LittleEndian.Uint32(head[4:])
	h, err := ParseHeader(head[8:])
	if err != nil {
		return nil, err
	}
	palRaw, err := p.ReadRaw(e.Offset+8+HeaderSize, uint32(h.Colors*3))
	if err != nil {
		return nil, err
	}
	pal := make([][3]uint8, h.Colors)
	for i := range pal {
		pal[i] = [3]uint8{palRaw[i*3], palRaw[i*3+1], palRaw[i*3+2]}
	}
	// frame table: (Frames + 2) entries of {offset, size, compressedSizeFlag}; the first two are skipped
	tab, err := p.ReadRaw(e.Offset+tabOff+24, uint32(h.Frames*12))
	if err != nil {
		return nil, err
	}
	s := &Sprite{Header: h, Palette: pal, Frames: make([]Frame, h.Frames)}
	for i := 0; i < h.Frames; i++ {
		off := binary.LittleEndian.Uint32(tab[i*12:])
		size := binary.LittleEndian.Uint32(tab[i*12+4:])
		flag := binary.LittleEndian.Uint32(tab[i*12+8:])
		blob, err := p.Extract(e.Offset+off, flag&0x00ffffff, int(size), pak.MethodOf(flag))
		if err != nil {
			return nil, fmt.Errorf("spr: frame %d: %w", i, err)
		}
		fr, err := DecodeFrame(blob, pal)
		if err != nil {
			return nil, fmt.Errorf("spr: frame %d: %w", i, err)
		}
		s.Frames[i] = fr
	}
	return s, nil
}

func readFrameOld(p *pak.File, e pak.Entry) (*Sprite, error) {
	head, err := p.ReadRaw(e.Offset, HeaderSize)
	if err != nil {
		return nil, err
	}
	h, err := ParseHeader(head)
	if err != nil {
		return nil, err
	}
	listSize := uint32(h.Colors*3 + h.Frames*8)
	list, err := p.ReadRaw(e.Offset+HeaderSize, listSize)
	if err != nil {
		return nil, err
	}
	pal := make([][3]uint8, h.Colors)
	for i := range pal {
		pal[i] = [3]uint8{list[i*3], list[i*3+1], list[i*3+2]}
	}
	info := list[h.Colors*3:]
	src := e.Offset + HeaderSize + listSize
	method := e.Method()
	s := &Sprite{Header: h, Palette: pal, Frames: make([]Frame, h.Frames)}
	for i := 0; i < h.Frames; i++ {
		compressed := int32(binary.LittleEndian.Uint32(info[i*8:]))
		size := int32(binary.LittleEndian.Uint32(info[i*8+4:]))
		var blob []byte
		if size < 0 { // stored as is
			blob, err = p.ReadRaw(src, uint32(-size))
			src += uint32(-size)
		} else {
			blob, err = p.Extract(src, uint32(compressed), int(size), method)
			src += uint32(compressed)
		}
		if err != nil {
			return nil, fmt.Errorf("spr: frame %d: %w", i, err)
		}
		fr, err := DecodeFrame(blob, pal)
		if err != nil {
			return nil, fmt.Errorf("spr: frame %d: %w", i, err)
		}
		s.Frames[i] = fr
	}
	return s, nil
}
