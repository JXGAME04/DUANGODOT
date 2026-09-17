package transport

import (
	"bytes"
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"io"
	"math/big"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"testing"
	"time"
)

// echoServer accepts one connection and copies bytes back.
func echoServer(t *testing.T, o Options) []Listener {
	t.Helper()
	ls, err := Listen(o)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() {
		for _, l := range ls {
			_ = l.Close()
		}
	})
	for _, l := range ls {
		go func(l Listener) {
			for {
				c, err := l.Accept()
				if err != nil {
					return
				}
				go func() {
					defer c.Close()
					_, _ = io.Copy(c, c)
				}()
			}
		}(l)
	}
	return ls
}

func TestAcceptKeyMatchesRFC6455(t *testing.T) {
	// the example of RFC 6455 section 1.3
	if got := acceptKey("dGhlIHNhbXBsZSBub25jZQ=="); got != "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=" {
		t.Fatalf("accept key %q", got)
	}
}

func TestWebSocketRoundTrip(t *testing.T) {
	ls := echoServer(t, Options{WS: "127.0.0.1:0"})
	url := "ws://" + ls[0].Addr().String() + "/ws"
	c, err := DialWebSocket(url, 5*time.Second)
	if err != nil {
		t.Fatal(err)
	}
	defer c.Close()
	if ls[0].Kind() != "ws" {
		t.Fatalf("kind %q", ls[0].Kind())
	}

	// small message, empty message and one that needs the 16 and 64 bit length forms
	for _, payload := range [][]byte{[]byte("xin chào"), {}, bytes.Repeat([]byte("a"), 200), bytes.Repeat([]byte("b"), 70000)} {
		if _, err := c.Write(payload); err != nil {
			t.Fatalf("write %d bytes: %v", len(payload), err)
		}
		got := make([]byte, 0, len(payload))
		buf := make([]byte, 4096)
		for len(got) < len(payload) {
			_ = c.SetReadDeadline(time.Now().Add(5 * time.Second))
			n, err := c.Read(buf)
			if err != nil {
				t.Fatalf("read back %d/%d: %v", len(got), len(payload), err)
			}
			got = append(got, buf[:n]...)
		}
		if !bytes.Equal(got, payload) {
			t.Fatalf("payload %d bytes came back wrong", len(payload))
		}
	}
}

func TestWebSocketFragmentsAndPing(t *testing.T) {
	ls := echoServer(t, Options{WS: "127.0.0.1:0"})
	c, err := DialWebSocket("ws://"+ls[0].Addr().String()+"/ws", 5*time.Second)
	if err != nil {
		t.Fatal(err)
	}
	defer c.Close()
	ws := c.(*wsConn)

	// a message split into three frames must arrive as one stream of bytes
	ws.wmu.Lock()
	_ = ws.writeFragmentLocked(opBinary, []byte("mot-"), false)
	_ = ws.writeFragmentLocked(opContinuation, []byte("hai-"), false)
	_ = ws.writeFragmentLocked(opContinuation, []byte("ba"), true)
	// and a ping in between must be answered by the server without disturbing the data
	_ = ws.writeFrameLocked(opPing, []byte("hb"))
	ws.wmu.Unlock()

	got := make([]byte, 0, 10)
	buf := make([]byte, 64)
	for len(got) < 10 {
		_ = c.SetReadDeadline(time.Now().Add(5 * time.Second))
		n, err := c.Read(buf)
		if err != nil {
			t.Fatalf("read: %v", err)
		}
		got = append(got, buf[:n]...)
	}
	if string(got) != "mot-hai-ba" {
		t.Fatalf("fragments joined wrong: %q", got)
	}
}

func TestWebSocketRejectsBadClients(t *testing.T) {
	ls := echoServer(t, Options{WS: "127.0.0.1:0"})
	addr := ls[0].Addr().String()

	// a plain GET is answered, not hung
	res, err := http.Get("http://" + addr + "/")
	if err != nil {
		t.Fatal(err)
	}
	body, _ := io.ReadAll(res.Body)
	_ = res.Body.Close()
	if res.StatusCode != http.StatusOK || !bytes.Contains(body, []byte("/ws")) {
		t.Fatalf("plain GET: %d %q", res.StatusCode, body)
	}

	// an upgrade with the wrong version is refused
	req, _ := http.NewRequest(http.MethodGet, "http://"+addr+"/ws", nil)
	req.Header.Set("Upgrade", "websocket")
	req.Header.Set("Connection", "Upgrade")
	req.Header.Set("Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ==")
	req.Header.Set("Sec-WebSocket-Version", "8")
	res, err = http.DefaultClient.Do(req)
	if err != nil {
		t.Fatal(err)
	}
	_ = res.Body.Close()
	if res.StatusCode != http.StatusUpgradeRequired {
		t.Fatalf("old version accepted: %d", res.StatusCode)
	}

	// a client that does not mask its frames is dropped (RFC 6455 section 5.1)
	c, err := DialWebSocket("ws://"+addr+"/ws", 5*time.Second)
	if err != nil {
		t.Fatal(err)
	}
	defer c.Close()
	ws := c.(*wsConn)
	ws.server = true // pretend to be a server: writes are no longer masked
	ws.wmu.Lock()
	_ = ws.writeFrameLocked(opBinary, []byte("unmasked"))
	ws.wmu.Unlock()
	_ = c.SetReadDeadline(time.Now().Add(5 * time.Second))
	if _, err := c.Read(make([]byte, 16)); err == nil {
		t.Fatal("server accepted an unmasked frame")
	}
}

func TestWebSocketMessageLimit(t *testing.T) {
	ls := echoServer(t, Options{WS: "127.0.0.1:0", MaxMessage: 1024})
	c, err := DialWebSocket("ws://"+ls[0].Addr().String()+"/ws", 5*time.Second)
	if err != nil {
		t.Fatal(err)
	}
	defer c.Close()
	if _, err := c.Write(bytes.Repeat([]byte("x"), 4096)); err != nil {
		t.Fatal(err)
	}
	_ = c.SetReadDeadline(time.Now().Add(3 * time.Second))
	if _, err := c.Read(make([]byte, 64)); err == nil {
		t.Fatal("oversized message accepted")
	}
}

func TestBadURLs(t *testing.T) {
	for _, u := range []string{"http://x/ws", "127.0.0.1:1/ws", "ws:///ws", ""} {
		if _, err := DialWebSocket(u, time.Second); err == nil {
			t.Fatalf("url %q accepted", u)
		}
	}
}

// writeCert makes a self-signed certificate for 127.0.0.1 and returns the two file paths.
func writeCert(t *testing.T) (certFile, keyFile string) {
	t.Helper()
	key, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		t.Fatal(err)
	}
	tmpl := x509.Certificate{
		SerialNumber:          big.NewInt(1),
		Subject:               pkix.Name{CommonName: "jx-next-test"},
		NotBefore:             time.Now().Add(-time.Hour),
		NotAfter:              time.Now().Add(24 * time.Hour),
		KeyUsage:              x509.KeyUsageDigitalSignature | x509.KeyUsageCertSign,
		ExtKeyUsage:           []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
		IPAddresses:           []net.IP{net.ParseIP("127.0.0.1")},
		IsCA:                  true,
		BasicConstraintsValid: true,
	}
	der, err := x509.CreateCertificate(rand.Reader, &tmpl, &tmpl, &key.PublicKey, key)
	if err != nil {
		t.Fatal(err)
	}
	dir := t.TempDir()
	certFile = filepath.Join(dir, "cert.pem")
	keyFile = filepath.Join(dir, "key.pem")
	certPEM := pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: der})
	if err := os.WriteFile(certFile, certPEM, 0o600); err != nil {
		t.Fatal(err)
	}
	kb, err := x509.MarshalECPrivateKey(key)
	if err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(keyFile, pem.EncodeToMemory(&pem.Block{Type: "EC PRIVATE KEY", Bytes: kb}), 0o600); err != nil {
		t.Fatal(err)
	}
	return certFile, keyFile
}

func TestTLSAndSecureWebSocket(t *testing.T) {
	cert, key := writeCert(t)
	ls := echoServer(t, Options{TCP: "127.0.0.1:0", WS: "127.0.0.1:0", CertFile: cert, KeyFile: key})
	var tcp, ws Listener
	for _, l := range ls {
		switch l.Kind() {
		case "tls":
			tcp = l
		case "wss":
			ws = l
		}
	}
	if tcp == nil || ws == nil {
		t.Fatalf("expected a tls and a wss door, got %d listeners", len(ls))
	}

	// raw frames over TLS
	c, err := DialTLSInsecure(tcp.Addr().String(), 5*time.Second)
	if err != nil {
		t.Fatal(err)
	}
	defer c.Close()
	if _, err := c.Write([]byte("qua tls")); err != nil {
		t.Fatal(err)
	}
	buf := make([]byte, 32)
	_ = c.SetReadDeadline(time.Now().Add(5 * time.Second))
	n, err := c.Read(buf)
	if err != nil || string(buf[:n]) != "qua tls" {
		t.Fatalf("tls echo: %v %q", err, buf[:n])
	}

	// the same frames over wss
	w, err := DialWebSocketInsecure("wss://"+ws.Addr().String()+"/ws", 5*time.Second)
	if err != nil {
		t.Fatal(err)
	}
	defer w.Close()
	if _, err := w.Write([]byte("qua wss")); err != nil {
		t.Fatal(err)
	}
	_ = w.SetReadDeadline(time.Now().Add(5 * time.Second))
	n, err = w.Read(buf)
	if err != nil || string(buf[:n]) != "qua wss" {
		t.Fatalf("wss echo: %v %q", err, buf[:n])
	}

	// a client that verifies the certificate refuses this self-signed one
	if _, err := DialWebSocket("wss://"+ws.Addr().String()+"/ws", 3*time.Second); err == nil {
		t.Fatal("self-signed certificate accepted by a verifying client")
	}
}

func TestListenErrors(t *testing.T) {
	if _, err := Listen(Options{}); err == nil {
		t.Fatal("empty options accepted")
	}
	if _, err := Listen(Options{TCP: "127.0.0.1:0", CertFile: "only-cert.pem"}); err == nil {
		t.Fatal("half a tls configuration accepted")
	}
	if _, err := Listen(Options{TCP: "127.0.0.1:0", CertFile: "missing.pem", KeyFile: "missing.key"}); err == nil {
		t.Fatal("missing certificate accepted")
	}
	// the second door failing must close the first one again
	busy, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	defer busy.Close()
	ls, err := Listen(Options{TCP: "127.0.0.1:0", WS: busy.Addr().String()})
	if err == nil {
		for _, l := range ls {
			_ = l.Close()
		}
		t.Fatal("listening on a busy port succeeded")
	}
}
