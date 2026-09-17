package transport

import (
	"context"
	"crypto/tls"
	"errors"
	"fmt"
	"net"
	"net/http"
	"sync"
	"time"
)

// Options describe the doors the gateway opens.  Empty address = that door stays closed.
type Options struct {
	TCP        string // raw frame stream, e.g. ":17100" (PC client, bots, LAN)
	WS         string // HTTP(S) endpoint carrying the same frames, e.g. ":17102" (web / mobile)
	WSPath     string // "" = "/ws"
	CertFile   string // PEM certificate; set both Cert and Key to serve TLS on both doors
	KeyFile    string
	MaxMessage int // largest WebSocket message (0 = DefaultMaxMessage)
	// Status answers GET /healthz on the WebSocket door: ready decides 200 or 503, body is the
	// JSON shown.  Monitoring, load balancers and tools/dev.py use it to wait for a server that
	// is really able to take players (the old cluster had no such thing).
	Status func() (ready bool, body []byte)
}

// Listener is one accepting door.  Kind is "tcp", "tls", "ws" or "wss" and appears in the log.
type Listener interface {
	Accept() (net.Conn, error)
	Close() error
	Addr() net.Addr
	Kind() string
}

// TLSConfig builds the server configuration (TLS 1.2+, modern ciphers) or nil when no
// certificate was configured.
func (o Options) TLSConfig() (*tls.Config, error) {
	if o.CertFile == "" && o.KeyFile == "" {
		return nil, nil
	}
	if o.CertFile == "" || o.KeyFile == "" {
		return nil, errors.New("transport: tls needs both a certificate and a key")
	}
	cert, err := tls.LoadX509KeyPair(o.CertFile, o.KeyFile)
	if err != nil {
		return nil, fmt.Errorf("transport: %w", err)
	}
	return &tls.Config{Certificates: []tls.Certificate{cert}, MinVersion: tls.VersionTLS12}, nil
}

// Listen opens every configured door.  On error the doors already open are closed again.
func Listen(o Options) ([]Listener, error) {
	tc, err := o.TLSConfig()
	if err != nil {
		return nil, err
	}
	var out []Listener
	fail := func(err error) ([]Listener, error) {
		for _, l := range out {
			_ = l.Close()
		}
		return nil, err
	}
	if o.TCP != "" {
		ln, err := net.Listen("tcp", o.TCP)
		if err != nil {
			return fail(err)
		}
		kind := "tcp"
		if tc != nil {
			ln, kind = tls.NewListener(ln, tc), "tls"
		}
		out = append(out, &plainListener{Listener: ln, kind: kind})
	}
	if o.WS != "" {
		ln, err := net.Listen("tcp", o.WS)
		if err != nil {
			return fail(err)
		}
		kind := "ws"
		if tc != nil {
			ln, kind = tls.NewListener(ln, tc), "wss"
		}
		out = append(out, newWSListener(ln, kind, o))
	}
	if len(out) == 0 {
		return nil, errors.New("transport: no listen address configured")
	}
	return out, nil
}

type plainListener struct {
	net.Listener
	kind string
}

func (l *plainListener) Kind() string { return l.kind }

// wsListener runs an HTTP server whose upgrade handler feeds Accept.
type wsListener struct {
	ln     net.Listener
	srv    *http.Server
	kind   string
	path   string
	conns  chan net.Conn
	closed chan struct{}
	once   sync.Once
}

func newWSListener(ln net.Listener, kind string, o Options) *wsListener {
	path := o.WSPath
	if path == "" {
		path = "/ws"
	}
	l := &wsListener{ln: ln, kind: kind, path: path, conns: make(chan net.Conn, 64), closed: make(chan struct{})}
	mux := http.NewServeMux()
	mux.HandleFunc(path, func(w http.ResponseWriter, r *http.Request) {
		c, err := Upgrade(w, r, o.MaxMessage)
		if err != nil {
			return // Upgrade already answered
		}
		select {
		case l.conns <- c:
		case <-l.closed:
			_ = c.Close()
		case <-time.After(5 * time.Second): // the gateway is not accepting: do not pile up
			_ = c.Close()
		}
	})
	// a load balancer, a monitor or tools/dev.py asks here whether the server can take players
	mux.HandleFunc("/healthz", func(w http.ResponseWriter, r *http.Request) {
		ready, body := true, []byte(`{"status":"ok"}`)
		if o.Status != nil {
			ready, body = o.Status()
		}
		w.Header().Set("Content-Type", "application/json")
		if !ready {
			w.WriteHeader(http.StatusServiceUnavailable)
		}
		_, _ = w.Write(append(body, '\n'))
	})
	// anything else on this port gets a plain answer instead of a hang (browsers, probes)
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/plain; charset=utf-8")
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write([]byte("jx gateway: websocket endpoint is " + path + ", health is /healthz\n"))
	})
	l.srv = &http.Server{Handler: mux, ReadHeaderTimeout: 10 * time.Second}
	go func() {
		if err := l.srv.Serve(ln); err != nil && !errors.Is(err, http.ErrServerClosed) {
			l.closeOnce()
		}
	}()
	return l
}

func (l *wsListener) Accept() (net.Conn, error) {
	select {
	case c := <-l.conns:
		return c, nil
	case <-l.closed:
		return nil, net.ErrClosed
	}
}

func (l *wsListener) Close() error {
	l.closeOnce()
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	return l.srv.Shutdown(ctx)
}

func (l *wsListener) closeOnce() { l.once.Do(func() { close(l.closed) }) }

func (l *wsListener) Addr() net.Addr { return l.ln.Addr() }
func (l *wsListener) Kind() string   { return l.kind }

// tlsDial is the client side of a wss:// connection (tests, jxbot).  insecure skips
// certificate verification and is only for the self-signed certificate of a dev machine.
func tlsDial(d net.Dialer, host string, insecure bool) (net.Conn, error) {
	name, _, err := net.SplitHostPort(host)
	if err != nil {
		name = host
	}
	return tls.DialWithDialer(&d, "tcp", host, &tls.Config{ServerName: name, InsecureSkipVerify: insecure, MinVersion: tls.VersionTLS12}) //nolint:gosec // dev only
}

// DialTLSInsecure connects to a TLS gateway without checking the certificate: for the
// self-signed certificate of a development machine only.
func DialTLSInsecure(addr string, timeout time.Duration) (net.Conn, error) {
	d := net.Dialer{Timeout: timeout}
	return tls.DialWithDialer(&d, "tcp", addr, &tls.Config{InsecureSkipVerify: true, MinVersion: tls.VersionTLS12}) //nolint:gosec // dev only
}
