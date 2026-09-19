package player

import "testing"

func TestParseKillEvents(t *testing.T) {
	data := []byte("EventID\tScript\tFunction\tTaskID\tOnce\tTotal\tMapId\tNpcTemplate\tPower\tLevel\tName\tNote\r\n" +
		"1\t\\script\\task\\newtask\\branch\\branch_killtimer.lua\tkillwolfone\t199\t1\t49\t90\t7\t1\t-1\t\tghi chu\r\n" +
		"2\t\\script\\x.lua\tkillhedgehog\t1062\t\t\t\t\t\t\tS\xe3i\r\n" +
		"x\t\\script\\y.lua\tnone\t1\t0\t1\t1\t1\t1\t1\r\n")
	e := ParseKillEvents(data)
	if len(e.Rows) != 2 {
		t.Fatalf("rows: %+v", e.Rows)
	}
	r := e.Rows[0]
	if r.ID != 1 || r.Script != `\script\task\newtask\branch\branch_killtimer.lua` || r.Function != "killwolfone" || r.TaskID != 199 || !r.OnlyOnce ||
		r.Total != 49 || r.Map != 90 || r.NpcTemplate != 7 || r.Power != 1 || r.Level != -1 {
		t.Fatalf("row 1: %+v", r)
	}
	// blanks: the task value 0, once off, every filter -1
	r = e.Rows[1]
	if r.TaskID != 1062 || r.OnlyOnce || r.Total != -1 || r.Map != -1 || r.NpcTemplate != -1 || r.Power != -1 || r.Level != -1 || r.NpcName == "" {
		t.Fatalf("row 2: %+v", r)
	}
}
