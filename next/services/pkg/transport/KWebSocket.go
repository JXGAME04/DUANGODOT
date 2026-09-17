// Package transport carries the JX NEXT frame stream over the links the clients can reach:
// raw TCP (PC client, bots), TLS, and WebSocket / WebSocket over TLS (browser and mobile
// builds, and anything behind a company proxy).  Everything above this package - the gateway
// session, the framing, the protobuf - stays the same: a transport is just a net.Conn.
//
// The old cluster had one raw TCP port with its own XOR obfuscation and nothing else, so the
// web/mobile client the roadmap wants could never connect to it.
//
// The WebSocket implementation here is RFC 6455 reduced to what the protocol needs: binary
// messages, fragmentation on read, ping/pong, close.  No extensions, no compression, no
// subprotocol negotiation - one dependency-free file that the tests fully cover.
package transport

import (
	"bufio"
	"crypto/rand"
	"crypto/sha1"
	"encoding/base64"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"net"
	"net/http"
	"strings"
	"sync"
	"time"
)

// wsMagic is the GUID of RFC 6455 section 1.3.
const wsMagic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

const (
	opContinuation = 0x0
	opText         = 0x1
	opBinary       = 0x2
	opClose        = 0x8
	opPing         = 0x9
	opPong         = 0xA
)

// DefaultMaxMessage is the largest WebSocket message accepted (one message carries one frame
// of the game protocol, capped at 64 KiB by docs/PROTOCOL.md).
const DefaultMaxMessage = 256 * 1024

var (
	ErrNotWebSocket   = errors.New("transport: not a websocket upgrade request")
	ErrMessageTooBig  = errors.New("transport: websocket message too large")
	ErrProtocol       = errors.New("transport: websocket protocol error")
	ErrUnmaskedClient = errors.New("transport: client frame was not masked")
)

// acceptKey answers the client's Sec-WebSocket-Key.
func acceptKey(key string) string {
	h := sha1.New()
	_, _ = io.WriteString(h, key+wsMagic)
	return base64.StdEncoding.EncodeToString(h.Sum(nil))
}

// IsWebSocketUpgrade reports whether r asks for a WebSocket connection.
func IsWebSocketUpgrade(r *http.Request) bool {
	return r.Method == http.MethodGet &&
		strings.EqualFold(r.Header.Get("Upgrade"), "websocket") &&
		strings.Contains(strings.ToLower(r.Header.Get("Connection")), "upgrade") &&
		r.Header.Get("Sec-WebSocket-Key") != ""
}

// Upgrade completes the handshake and returns the connection as a net.Conn whose Read/Write
// carry the bytes of binary messages.  On failure it has already answered with an error status.
func Upgrade(w http.ResponseWriter, r *http.Request, maxMessage int) (net.Conn, error) {
	if !IsWebSocketUpgrade(r) {
		http.Error(w, "expected a websocket upgrade", http.StatusBadRequest)
		return nil, ErrNotWebSocket
	}
	if v := r.Header.Get("Sec-WebSocket-Version"); v != "13" {
		w.Header().Set("Sec-WebSocket-Version", "13")
		http.Error(w, "unsupported websocket version", http.StatusUpgradeRequired)
		return nil, ErrProtocol
	}
	hj, ok := w.(http.Hijacker)
	if !ok {
		http.Error(w, "connection cannot be hijacked", http.StatusInternalServerError)
		return nil, ErrProtocol
	}
	raw, brw, err := hj.Hijack()
	if err != nil {
		return nil, err
	}
	res := "HTTP/1.1 101 Switching Protocols\r\n" +
		"Upgrade: websocket\r\n" +
		"Connection: Upgrade\r\n" +
		"Sec-WebSocket-Accept: " + acceptKey(r.Header.Get("Sec-WebSocket-Key")) + "\r\n\r\n"
	if _, err := brw.WriteString(res); err != nil {
		_ = raw.Close()
		return nil, err
	}
	if err := brw.Flush(); err != nil {
		_ = raw.Close()
		return nil, err
	}
	if maxMessage <= 0 {
		maxMessage = DefaultMaxMessage
	}
	return &wsConn{raw: raw, br: brw.Reader, server: true, maxMessage: maxMessage}, nil
}

// DialWebSocket opens a client connection to rawURL ("ws://host:port/path").  It exists for
// the tests and for jxbot; the Godot client uses Godot's own WebSocketPeer.
func DialWebSocket(rawURL string, timeout time.Duration) (net.Conn, error) {
	return dialWebSocket(rawURL, timeout, false)
}

// DialWebSocketInsecure is DialWebSocket without certificate verification (wss:// against the
// self-signed certificate of a development machine).
func DialWebSocketInsecure(rawURL string, timeout time.Duration) (net.Conn, error) {
	return dialWebSocket(rawURL, timeout, true)
}

func dialWebSocket(rawURL string, timeout time.Duration, insecure bool) (net.Conn, error) {
	scheme, host, path, err := splitWS(rawURL)
	if err != nil {
		return nil, err
	}
	var raw net.Conn
	d := net.Dialer{Timeout: timeout}
	if scheme == "wss" {
		raw, err = tlsDial(d, host, insecure)
	} else {
		raw, err = d.Dial("tcp", host)
	}
	if err != nil {
		return nil, err
	}
	key := make([]byte, 16)
	if _, err := rand.Read(key); err != nil {
		_ = raw.Close()
		return nil, err
	}
	k := base64.StdEncoding.EncodeToString(key)
	req := fmt.Sprintf("GET %s HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"+
		"Sec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n\r\n", path, host, k)
	if timeout > 0 {
		_ = raw.SetDeadline(time.Now().Add(timeout))
	}
	if _, err := io.WriteString(raw, req); err != nil {
		_ = raw.Close()
		return nil, err
	}
	br := bufio.NewReader(raw)
	res, err := http.ReadResponse(br, nil)
	if err != nil {
		_ = raw.Close()
		return nil, err
	}
	_ = res.Body.Close()
	if res.StatusCode != http.StatusSwitchingProtocols {
		_ = raw.Close()
		return nil, fmt.Errorf("transport: websocket handshake failed: %s", res.Status)
	}
	if res.Header.Get("Sec-WebSocket-Accept") != acceptKey(k) {
		_ = raw.Close()
		return nil, ErrProtocol
	}
	_ = raw.SetDeadline(time.Time{})
	return &wsConn{raw: raw, br: br, server: false, maxMessage: DefaultMaxMessage}, nil
}

func splitWS(rawURL string) (scheme, host, path string, err error) {
	switch {
	case strings.HasPrefix(rawURL, "ws://"):
		scheme, rawURL = "ws", rawURL[len("ws://"):]
	case strings.HasPrefix(rawURL, "wss://"):
		scheme, rawURL = "wss", rawURL[len("wss://"):]
	default:
		return "", "", "", fmt.Errorf("transport: %q is not a ws:// or wss:// url", rawURL)
	}
	host, path, _ = strings.Cut(rawURL, "/")
	if host == "" {
		return "", "", "", fmt.Errorf("transport: missing host in url")
	}
	return scheme, host, "/" + path, nil
}

// wsConn presents a WebSocket as a byte stream: Read hands out the payload of binary
// messages in order, Write sends one binary message per call.  The protocol's own framing
// (docs/PROTOCOL.md) delimits messages, so a message boundary carries no meaning.
type wsConn struct {
	raw        net.Conn
	br         *bufio.Reader
	server     bool // server connections must not mask, client connections must
	maxMessage int

	rest   []byte // payload of the current message not yet handed to Read
	closed bool

	wmu sync.Mutex // one writer at a time (game frames, pongs and the close frame)
}

func (c *wsConn) LocalAddr() net.Addr                { return c.raw.LocalAddr() }
func (c *wsConn) RemoteAddr() net.Addr               { return c.raw.RemoteAddr() }
func (c *wsConn) SetDeadline(t time.Time) error      { return c.raw.SetDeadline(t) }
func (c *wsConn) SetReadDeadline(t time.Time) error  { return c.raw.SetReadDeadline(t) }
func (c *wsConn) SetWriteDeadline(t time.Time) error { return c.raw.SetWriteDeadline(t) }

func (c *wsConn) Close() error {
	c.wmu.Lock()
	if !c.closed {
		c.closed = true
		// best effort "going away"; the peer may already be gone
		_ = c.writeFrameLocked(opClose, []byte{0x03, 0xE9}) // 1001
	}
	c.wmu.Unlock()
	return c.raw.Close()
}

// Read fills p from the current message, reading further messages when it runs out.
func (c *wsConn) Read(p []byte) (int, error) {
	for len(c.rest) == 0 {
		msg, err := c.readMessage()
		if err != nil {
			return 0, err
		}
		c.rest = msg
	}
	n := copy(p, c.rest)
	c.rest = c.rest[n:]
	return n, nil
}

// Write sends p as one binary message.
func (c *wsConn) Write(p []byte) (int, error) {
	c.wmu.Lock()
	defer c.wmu.Unlock()
	if c.closed {
		return 0, net.ErrClosed
	}
	if err := c.writeFrameLocked(opBinary, p); err != nil {
		return 0, err
	}
	return len(p), nil
}

// readMessage returns the payload of the next data message, answering control frames on the way.
func (c *wsConn) readMessage() ([]byte, error) {
	var msg []byte
	var started bool
	for {
		fin, opcode, payload, err := c.readFrame()
		if err != nil {
			return nil, err
		}
		switch opcode {
		case opPing:
			c.wmu.Lock()
			err = c.writeFrameLocked(opPong, payload)
			c.wmu.Unlock()
			if err != nil {
				return nil, err
			}
		case opPong:
			// nothing to do: the game protocol has its own heartbeat
		case opClose:
			c.wmu.Lock()
			if !c.closed {
				c.closed = true
				_ = c.writeFrameLocked(opClose, payload)
			}
			c.wmu.Unlock()
			return nil, io.EOF
		case opBinary, opText:
			if started {
				return nil, ErrProtocol // a new message before the previous one finished
			}
			started = true
			msg = append(msg, payload...)
			if fin {
				return msg, nil
			}
		case opContinuation:
			if !started {
				return nil, ErrProtocol
			}
			msg = append(msg, payload...)
			if len(msg) > c.maxMessage {
				return nil, ErrMessageTooBig
			}
			if fin {
				return msg, nil
			}
		default:
			return nil, ErrProtocol
		}
	}
}

// readFrame reads one WebSocket frame (RFC 6455 section 5.2).
func (c *wsConn) readFrame() (fin bool, opcode byte, payload []byte, err error) {
	var hdr [2]byte
	if _, err = io.ReadFull(c.br, hdr[:]); err != nil {
		return false, 0, nil, err
	}
	fin = hdr[0]&0x80 != 0
	if hdr[0]&0x70 != 0 { // RSV bits: no extension was negotiated
		return false, 0, nil, ErrProtocol
	}
	opcode = hdr[0] & 0x0F
	masked := hdr[1]&0x80 != 0
	if c.server && !masked {
		return false, 0, nil, ErrUnmaskedClient
	}
	if !c.server && masked {
		return false, 0, nil, ErrProtocol
	}
	length := uint64(hdr[1] & 0x7F)
	switch length {
	case 126:
		var ext [2]byte
		if _, err = io.ReadFull(c.br, ext[:]); err != nil {
			return false, 0, nil, err
		}
		length = uint64(binary.BigEndian.Uint16(ext[:]))
	case 127:
		var ext [8]byte
		if _, err = io.ReadFull(c.br, ext[:]); err != nil {
			return false, 0, nil, err
		}
		length = binary.BigEndian.Uint64(ext[:])
	}
	isControl := opcode&0x8 != 0
	if isControl && (length > 125 || !fin) {
		return false, 0, nil, ErrProtocol // control frames are short and never fragmented
	}
	if length > uint64(c.maxMessage) {
		return false, 0, nil, ErrMessageTooBig
	}
	var mask [4]byte
	if masked {
		if _, err = io.ReadFull(c.br, mask[:]); err != nil {
			return false, 0, nil, err
		}
	}
	payload = make([]byte, length)
	if _, err = io.ReadFull(c.br, payload); err != nil {
		return false, 0, nil, err
	}
	if masked {
		for i := range payload {
			payload[i] ^= mask[i%4]
		}
	}
	return fin, opcode, payload, nil
}

// writeFrameLocked writes one unfragmented frame; the caller holds wmu.
func (c *wsConn) writeFrameLocked(opcode byte, payload []byte) error {
	return c.writeFragmentLocked(opcode, payload, true)
}

// writeFragmentLocked writes one frame, final or not; the caller holds wmu.
func (c *wsConn) writeFragmentLocked(opcode byte, payload []byte, fin bool) error {
	hdr := make([]byte, 0, 14)
	first := opcode
	if fin {
		first |= 0x80
	}
	hdr = append(hdr, first)
	maskBit := byte(0)
	if !c.server {
		maskBit = 0x80
	}
	n := len(payload)
	switch {
	case n <= 125:
		hdr = append(hdr, maskBit|byte(n))
	case n <= 0xFFFF:
		hdr = append(hdr, maskBit|126, byte(n>>8), byte(n))
	default:
		hdr = append(hdr, maskBit|127)
		var ext [8]byte
		binary.BigEndian.PutUint64(ext[:], uint64(n))
		hdr = append(hdr, ext[:]...)
	}
	body := payload
	if !c.server { // a client must mask every frame
		var mask [4]byte
		if _, err := rand.Read(mask[:]); err != nil {
			return err
		}
		hdr = append(hdr, mask[:]...)
		body = make([]byte, n)
		for i := 0; i < n; i++ {
			body[i] = payload[i] ^ mask[i%4]
		}
	}
	if _, err := c.raw.Write(append(hdr, body...)); err != nil {
		return err
	}
	return nil
}
