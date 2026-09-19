// jxrecord - record what crosses a game connection into a .jxrec file, and read one back.
//
// It sits between a client and a server as a plain TCP proxy and writes every byte of both
// directions with its timestamp.  It understands nothing about the payload, so it records the
// old game (Rainbow / Bishop, own obfuscated stream) as well as Protocol V2.  The roadmap wants
// these recordings early: they are the evidence the new server is compared against.
//
//	# the old game: point the client at 127.0.0.1:7100 instead of the real server
//	jxrecord proxy -listen 127.0.0.1:7100 -to 203.0.113.9:5600 -out logs/old-login.jxrec -note "dang nhap"
//
//	# the new one (checking the recorder itself, or a client bug)
//	jxrecord proxy -listen 127.0.0.1:7100 -to 127.0.0.1:19100 -out logs/new.jxrec
//
//	jxrecord dump logs/old-login.jxrec            # what was recorded, one line per chunk
//	jxrecord dump -hex -max 64 logs/old.jxrec     # with the first bytes of every chunk
//	jxrecord dump -jx logs/new.jxrec              # decode the Protocol V2 framing (msg ids)
package main

import (
	"context"
	"encoding/hex"
	"errors"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"

	"github.com/JXGAME04/DUANGODOT/next/services/pkg/frame"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxpb"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/jxrec"
	"github.com/JXGAME04/DUANGODOT/next/services/pkg/log"
)

func usage() {
	fmt.Fprintln(os.Stderr, "usage: jxrecord proxy -listen ADDR -to ADDR -out FILE [-note TEXT] [-one]")
	fmt.Fprintln(os.Stderr, "       jxrecord dump [-hex] [-max N] [-jx] FILE")
	os.Exit(2)
}

func main() {
	if len(os.Args) < 2 {
		usage()
	}
	switch os.Args[1] {
	case "proxy":
		proxy(os.Args[2:])
	case "dump":
		dump(os.Args[2:])
	default:
		usage()
	}
}

func proxy(args []string) {
	fs := flag.NewFlagSet("proxy", flag.ExitOnError)
	listen := fs.String("listen", "127.0.0.1:7100", "address the client connects to")
	to := fs.String("to", "", "the real server, host:port")
	out := fs.String("out", "", "output file (default logs/<time>.jxrec)")
	note := fs.String("note", "", "free text stored in the recording")
	one := fs.Bool("one", false, "record one connection and exit")
	level := fs.String("log-level", "info", "log level")
	_ = fs.Parse(args)
	if *to == "" {
		usage()
	}
	_ = log.Init(log.Options{Level: log.ParseLevel(*level), Console: true, Process: "jxrecord"})
	defer log.Shutdown()

	ln, err := net.Listen("tcp", *listen)
	if err != nil {
		log.Fatal("boot", "cannot listen", log.F("addr", *listen), log.F("error", err))
		os.Exit(1)
	}
	log.Info("boot", "recording", log.F("listen", ln.Addr().String()), log.F("target", *to), log.F("one", *one))

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()
	go func() {
		<-ctx.Done()
		_ = ln.Close()
	}()

	var wg sync.WaitGroup
	n := 0
	for {
		c, err := ln.Accept()
		if err != nil {
			break
		}
		n++
		path := *out
		if path == "" || (!*one && n > 1) {
			path = nextPath(*out, n)
		}
		wg.Add(1)
		go func(c net.Conn, path string) {
			defer wg.Done()
			record(c, *to, path, *note, *listen)
		}(c, path)
		if *one {
			break
		}
	}
	wg.Wait()
	log.Info("boot", "recorder stopped", log.F("connections", n))
}

// nextPath makes one file per connection: out.jxrec, out.2.jxrec, ...
func nextPath(out string, n int) string {
	if out == "" {
		_ = os.MkdirAll("logs", 0o755)
		return fmt.Sprintf("logs/%s.jxrec", time.Now().Format("20060102-150405"))
	}
	if n <= 1 {
		return out
	}
	return fmt.Sprintf("%s.%d.jxrec", out, n)
}

// record proxies one client connection to the server and writes both directions to path.
func record(client net.Conn, target, path, note, listen string) {
	defer client.Close()
	server, err := net.DialTimeout("tcp", target, 10*time.Second)
	if err != nil {
		log.Error("net", "cannot reach the server", log.F("target", target), log.F("error", err))
		return
	}
	defer server.Close()
	w, err := jxrec.Create(path, jxrec.Header{Listen: listen, Target: target, Client: client.RemoteAddr().String(), Note: note})
	if err != nil {
		log.Error("rec", "cannot write the recording", log.F("path", path), log.F("error", err))
		return
	}
	log.Info("rec", "connection", log.F("client", client.RemoteAddr().String()), log.F("file", path))

	done := make(chan struct{}, 2)
	go copyAndRecord(server, client, w, jxrec.DirClient, done) // client -> server
	go copyAndRecord(client, server, w, jxrec.DirServer, done) // server -> client

	// flush regularly so a crash of the recorded game still leaves a usable file
	tick := time.NewTicker(time.Second)
	defer tick.Stop()
	open := 2
	for open > 0 {
		select {
		case <-done:
			open--
		case <-tick.C:
			_ = w.Flush()
		}
	}
	_ = client.Close()
	_ = server.Close()
	records, bytes := w.Stats()
	if err := w.Close(); err != nil {
		log.Error("rec", "closing the recording failed", log.F("error", err))
	}
	log.Info("rec", "connection closed", log.F("file", path), log.F("records", records), log.F("bytes", bytes))
}

func copyAndRecord(dst io.Writer, src io.Reader, w *jxrec.Writer, dir byte, done chan<- struct{}) {
	buf := make([]byte, 32*1024)
	for {
		n, err := src.Read(buf)
		if n > 0 {
			if werr := w.Write(dir, buf[:n]); werr != nil {
				log.Error("rec", "write failed", log.F("error", werr))
			}
			if _, werr := dst.Write(buf[:n]); werr != nil {
				break
			}
		}
		if err != nil {
			break
		}
	}
	done <- struct{}{}
}

func dump(args []string) {
	fs := flag.NewFlagSet("dump", flag.ExitOnError)
	withHex := fs.Bool("hex", false, "print the first bytes of every chunk")
	max := fs.Int("max", 32, "how many bytes to print with -hex")
	jx := fs.Bool("jx", false, "decode Protocol V2 framing and name the messages")
	_ = fs.Parse(args)
	if fs.NArg() != 1 {
		usage()
	}
	r, err := jxrec.Open(fs.Arg(0))
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	defer r.Close()
	h := r.Header
	fmt.Printf("# %s -> %s  client %s  started %s\n", h.Listen, h.Target, h.Client,
		time.UnixMilli(h.StartedMs).Format("2006-01-02 15:04:05"))
	if h.Note != "" {
		fmt.Printf("# note: %s\n", h.Note)
	}
	parsers := map[byte]*frame.Parser{
		jxrec.DirClient: frame.NewParser(frame.MaxInternalPayload),
		jxrec.DirServer: frame.NewParser(frame.MaxInternalPayload),
	}
	var records, total int64
	counts := map[string]int{}
	for {
		rec, err := r.Next()
		if errors.Is(err, io.EOF) {
			break
		}
		if err != nil {
			fmt.Fprintf(os.Stderr, "%v (after %d records)\n", err, records)
			break
		}
		records++
		total += int64(len(rec.Data))
		line := fmt.Sprintf("%8d ms %s %6d bytes", rec.Ms, rec.DirString(), len(rec.Data))
		if *jx {
			p := parsers[rec.Dir]
			p.Feed(rec.Data)
			for {
				f, ok, perr := p.Next()
				if perr != nil {
					line += "  [not Protocol V2]"
					p.Reset()
					break
				}
				if !ok {
					break
				}
				name := jxpb.MsgId(f.MsgID).String()
				counts[name]++
				line += fmt.Sprintf("  %s(%d bytes)", name, len(f.Payload))
			}
		}
		if *withHex {
			n := min(*max, len(rec.Data))
			line += "  " + hex.EncodeToString(rec.Data[:n])
			if n < len(rec.Data) {
				line += "..."
			}
		}
		fmt.Println(line)
	}
	fmt.Printf("# %d records, %d bytes\n", records, total)
	for name, n := range counts {
		fmt.Printf("#   %-24s %d\n", name, n)
	}
}
