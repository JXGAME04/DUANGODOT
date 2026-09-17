package main

import "testing"

func TestMapDirPathAddsTheMapsRootTheNewerListsOmit(t *testing.T) {
	cases := map[string]string{
		`\maps\西北南区\凤翔`: `\maps\西北南区\凤翔`, // JX1 MapList.ini
		`西北南区\凤翔`:       `\maps\西北南区\凤翔`, // Linux server / VLTK 2.0 lists
		`\西北南区\凤翔`:      `\maps\西北南区\凤翔`,
		`maps\x\y`:      `maps\x\y`,
		`\MAPS\x\y`:     `\MAPS\x\y`,
	}
	for in, want := range cases {
		if got := mapDirPath(in); got != want {
			t.Errorf("mapDirPath(%q) = %q, want %q", in, got, want)
		}
	}
}

func TestPrimaryDirIsTheFirstOfTheClientChain(t *testing.T) {
	if got := primaryDir(`C:\ref ; D:\fallback`); got != `C:\ref` {
		t.Fatalf("got %q", got)
	}
	if got := primaryDir(`C:\only`); got != `C:\only` {
		t.Fatalf("got %q", got)
	}
}
