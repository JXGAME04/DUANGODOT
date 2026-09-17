package gateway

import (
	"encoding/json"
	"io"
	"net/http"
	"testing"
	"time"
)

// /healthz on the WebSocket door tells whether the gateway can take players right now.  A
// monitor, a load balancer and tools/dev.py use it instead of guessing from the open port.
func TestHealthEndpoint(t *testing.T) {
	zone := startFakeZone(t)
	srv, _, cancel, done := startGatewayWith(t, zone.ln.Addr().String(), Config{ListenWS: "127.0.0.1:0"}, nil)
	url := "http://" + srv.AddrWS() + "/healthz"

	status, body := get(t, url)
	var h struct {
		Status    string `json:"status"`
		Version   string `json:"version"`
		Auth      string `json:"auth_mode"`
		ZoneReady bool   `json:"zone_ready"`
		Sessions  int    `json:"sessions"`
		Online    int    `json:"online"`
		Stopping  bool   `json:"stopping"`
	}
	if err := json.Unmarshal(body, &h); err != nil {
		t.Fatalf("body %q: %v", body, err)
	}
	if status != http.StatusOK || h.Status != "ok" || !h.ZoneReady || h.Auth != "dev" || h.Version != Version {
		t.Fatalf("healthy gateway: %d %+v", status, h)
	}

	c := dial(t, srv.Addr())
	if _, l := c.login("health1", "pw"); l.Result != 0 {
		t.Fatalf("login %+v", l)
	}
	_, body = get(t, url)
	_ = json.Unmarshal(body, &h)
	if h.Sessions != 1 || h.Online != 1 {
		t.Fatalf("counters: %+v", h)
	}

	// while shutting down it reports 503 so a load balancer stops sending players here
	cancel()
	deadline := time.Now().Add(5 * time.Second)
	for time.Now().Before(deadline) {
		select {
		case <-done:
			return // the door closed before we could read it: that is fine too
		default:
		}
		status, body = getMaybe(url)
		if status == http.StatusServiceUnavailable {
			_ = json.Unmarshal(body, &h)
			if !h.Stopping || h.Status != "unavailable" {
				t.Fatalf("shutting down: %+v", h)
			}
			return
		}
		time.Sleep(20 * time.Millisecond)
	}
	t.Fatal("never reported unavailable while shutting down")
}

func get(t *testing.T, url string) (int, []byte) {
	t.Helper()
	res, err := http.Get(url)
	if err != nil {
		t.Fatal(err)
	}
	defer res.Body.Close()
	body, _ := io.ReadAll(res.Body)
	return res.StatusCode, body
}

func getMaybe(url string) (int, []byte) {
	res, err := http.Get(url)
	if err != nil {
		return 0, nil
	}
	defer res.Body.Close()
	body, _ := io.ReadAll(res.Body)
	return res.StatusCode, body
}
