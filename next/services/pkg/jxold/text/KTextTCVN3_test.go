package text

import "testing"

func TestTCVN3RoundTripWithGameSamples(t *testing.T) {
	// names taken from Settings/MapList.ini of the old client
	cases := map[string]string{
		"Ph\xad\xeeng T\xad\xeang":     "Phượng Tường",
		"Hoa S\xacn":                   "Hoa Sơn",
		"Ki\xd5m C\xb8c T\xa9y B\xbec": "Kiếm Các Tây Bắc",
		"Kinh Ho\xb5ng \xae\xe9ng":     "Kinh Hoàng động",
		"Nh\xb9n Th\xb9ch \xae\xe9ng":  "Nhạn Thạch động",
		"Chu Ti\xaan tr\xcan":          "Chu Tiên trấn",
		"\xa7\xb9i Hi\xd6p":            "Đại Hiệp",
	}
	for in, want := range cases {
		if got := TCVN3ToUTF8([]byte(in)); got != want {
			t.Errorf("decode %q: got %q want %q", in, got, want)
		}
		back, ok := UTF8ToTCVN3(want)
		if !ok || string(back) != in {
			t.Errorf("encode %q: got %q (ok=%v) want %q", want, back, ok, in)
		}
	}
	if _, ok := UTF8ToTCVN3("CHÚC"); ok {
		t.Error("uppercase accented vowels do not exist in TCVN3")
	}
}

func TestIsTCVN3RejectsGBKRuns(t *testing.T) {
	if !IsTCVN3([]byte("\xae\xad\xeec")) {
		t.Error("three accented letters in a row must pass")
	}
	if IsTCVN3([]byte("\xce\xf7\xb1\xb1\xc4\xcf")) {
		t.Error("a GBK path must fail (run of 6 high bytes)")
	}
	if IsTCVN3([]byte("a\xbfb")) {
		t.Error("unassigned byte 0xbf must fail")
	}
}

func TestGBK(t *testing.T) {
	if got := GBKToUTF8([]byte("\xce\xf7\xb1\xb1\xc4\xcf\xc7\xf8")); got != "西北南区" {
		t.Errorf("gbk decode: %q", got)
	}
	b, err := UTF8ToGBK("凤翔")
	if err != nil || string(b) != "\xb7\xef\xcf\xe8" {
		t.Errorf("gbk encode: % x %v", b, err)
	}
}

func TestDecodeMixedKeepsBothSidesOfALine(t *testing.T) {
	vn, _ := UTF8ToTCVN3("Thiết Trúc")
	zh, _ := UTF8ToGBK("铁竹")
	line := append(append([]byte{}, vn...), []byte(" - ")...)
	line = append(line, zh...)
	if got := DecodeMixed(line); got != "Thiết Trúc - 铁竹" {
		t.Fatalf("mixed line: %q", got)
	}
	if got := DecodeMixed([]byte("plain ascii")); got != "plain ascii" {
		t.Fatalf("ascii: %q", got)
	}
	// Word's curly quotes inside Vietnamese do not turn the line into GBK
	quoted := append(append([]byte{0x93}, vn...), 0x94)
	if got := DecodeMixed(quoted); got != "\u201cThiết Trúc\u201d" {
		t.Fatalf("punctuation: %q", got)
	}
}
