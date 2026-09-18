package item

import "testing"

func TestParseAbradeRateReadsEveryModeInPartOrder(t *testing.T) {
	ini := "[Repair]\nItemPriceScale=50\nMagicPriceScale=16\nWarningBaseline=5\n" +
		"[Attack]\nWeapon=256\nHead=0\n" +
		"[Defend]\nHead=320\nBody=320\nCuff=320\n" +
		"[Move]\nFoot=2560\nHorse=5120\n" +
		"[AdvPlatina_Defend]\nAmulet=320\nRing1=320\n"
	tb := ParseAbradeRate([]byte(ini))
	if tb.Repair.ItemPriceScale != 50 || tb.Repair.MagicPriceScale != 16 || tb.Repair.WarningBaseline != 5 {
		t.Fatalf("repair %+v", tb.Repair)
	}
	// ITEM_PART order: head 0, body 1, belt 2, weapon 3, foot 4, cuff 5, amulet 6, ring1 7, ..., horse 10, mask 11
	if tb.Rate.Attack[3] != 256 || tb.Rate.Attack[0] != 0 || tb.Rate.Defend[0] != 320 || tb.Rate.Defend[1] != 320 || tb.Rate.Defend[5] != 320 {
		t.Fatalf("attack %v defend %v", tb.Rate.Attack, tb.Rate.Defend)
	}
	if tb.Rate.Move[4] != 2560 || tb.Rate.Move[10] != 5120 || tb.Rate.Move[3] != 0 {
		t.Fatalf("move %v", tb.Rate.Move)
	}
	if tb.AdvRate.Defend[6] != 320 || tb.AdvRate.Defend[7] != 320 || tb.AdvRate.Attack[3] != 0 {
		t.Fatalf("adv %v", tb.AdvRate)
	}
	if tb.Rate.Attack[12] != 0 || tb.Rate.Attack[14] != 0 {
		t.Fatalf("the JX2 slots must stay 0: %v", tb.Rate.Attack)
	}
}
