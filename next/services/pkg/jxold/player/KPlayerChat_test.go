package player

import "testing"

func TestParseChatCost(t *testing.T) {
	// the real file: five types, comments in GBK between them
	data := []byte("// Level dang cap\r\n\r\n[0]\r\nLevel=0\r\nMoney=0\r\nManaPercent=0\r\nStaminaPercent=0\r\n\r\n[1]\r\nLevel=0\r\nMoney=10\r\n" +
		"ManaPercent=0\r\nStaminaPercent=0\r\n\r\n[2]\r\nLevel=20\r\nMoney=0\r\nManaPercent=20\r\nStaminaPercent=0\r\n\r\n[3]\r\nLevel=0\r\nMoney=0\r\n" +
		"ManaPercent=10\r\nStaminaPercent=0\r\n\r\n[4]\r\nLevel=30\r\nMoney=0\r\nManaPercent=80\r\nStaminaPercent=0\r\n")
	c := ParseChatCost(data)
	if c.Rows[1].Money != 10 || c.Rows[2].Level != 20 || c.Rows[2].ManaPercent != 20 || c.Rows[3].ManaPercent != 10 {
		t.Fatalf("rows: %+v", c.Rows)
	}
	if c.Rows[4].Level != 30 || c.Rows[4].ManaPercent != 80 || c.Rows[0] != (ChatCostRow{}) {
		t.Fatalf("rows: %+v", c.Rows)
	}
	// a missing section stays 0
	c = ParseChatCost([]byte("[2]\nLevel=5\n"))
	if c.Rows[2].Level != 5 || c.Rows[4] != (ChatCostRow{}) {
		t.Fatalf("rows: %+v", c.Rows)
	}
}
