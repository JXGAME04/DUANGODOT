// Package nrv2b decompresses UCL NRV2B-99 streams (ucl_nrv2b_decompress_8), the codec used by
// the old JX .pak archives (XPackFile TYPE_UCL).  Port of ucl/n2b_d.c; pure Go, no cgo.
package nrv2b

import "errors"

var (
	ErrInputOverrun  = errors.New("nrv2b: input overrun")
	ErrOutputOverrun = errors.New("nrv2b: output overrun")
	ErrLookbehind    = errors.New("nrv2b: lookbehind overrun")
)

type bitReader struct {
	src  []byte
	pos  int
	bb   uint32
	left int
}

func (r *bitReader) bit() (uint32, error) {
	if r.left == 0 {
		if r.pos >= len(r.src) {
			return 0, ErrInputOverrun
		}
		r.bb = uint32(r.src[r.pos])
		r.pos++
		r.left = 8
	}
	r.left--
	return (r.bb >> uint(r.left)) & 1, nil
}

// Decompress inflates src into a buffer of exactly dstLen bytes (the original size stored in the
// pak index).  It fails when the stream ends early or would overflow.
func Decompress(src []byte, dstLen int) ([]byte, error) {
	dst := make([]byte, dstLen)
	r := &bitReader{src: src}
	olen := 0
	lastOff := uint32(1)
	for {
		for {
			b, err := r.bit()
			if err != nil {
				return nil, err
			}
			if b == 0 {
				break
			}
			if r.pos >= len(src) {
				return nil, ErrInputOverrun
			}
			if olen >= dstLen {
				return nil, ErrOutputOverrun
			}
			dst[olen] = src[r.pos]
			olen++
			r.pos++
		}
		mOff := uint32(1)
		for {
			b, err := r.bit()
			if err != nil {
				return nil, err
			}
			mOff = mOff*2 + b
			if mOff > 0xffffff+3 {
				return nil, ErrLookbehind
			}
			stop, err := r.bit()
			if err != nil {
				return nil, err
			}
			if stop == 1 {
				break
			}
		}
		if mOff == 2 {
			mOff = lastOff
		} else {
			if r.pos >= len(src) {
				return nil, ErrInputOverrun
			}
			mOff = (mOff-3)*256 + uint32(src[r.pos])
			r.pos++
			if mOff == 0xffffffff {
				break // end of stream marker
			}
			mOff++
			lastOff = mOff
		}
		b1, err := r.bit()
		if err != nil {
			return nil, err
		}
		b2, err := r.bit()
		if err != nil {
			return nil, err
		}
		mLen := b1*2 + b2
		if mLen == 0 {
			mLen = 1
			for {
				b, err := r.bit()
				if err != nil {
					return nil, err
				}
				mLen = mLen*2 + b
				if int(mLen) >= dstLen {
					return nil, ErrOutputOverrun
				}
				stop, err := r.bit()
				if err != nil {
					return nil, err
				}
				if stop == 1 {
					break
				}
			}
			mLen += 2
		}
		if mOff > 0xd00 {
			mLen++
		}
		n := int(mLen) + 1
		if olen+n > dstLen {
			return nil, ErrOutputOverrun
		}
		if int(mOff) > olen {
			return nil, ErrLookbehind
		}
		pos := olen - int(mOff)
		for i := 0; i < n; i++ {
			dst[olen] = dst[pos]
			olen++
			pos++
		}
	}
	if olen != dstLen {
		return dst[:olen], ErrOutputOverrun
	}
	return dst, nil
}
