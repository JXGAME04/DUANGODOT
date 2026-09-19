package player

import "testing"

func TestParseTaskDef(t *testing.T) {
	// the shape of the real file: the header, a description row, then the rows; the flags read by position
	data := []byte("TASK_ID_FIRST\tTASK_ID_LAST\tTASK_NAME\tSYNC_FLAG\tCLIENT_FLAG\tTASK_DESCRIBE\r\n" +
		"mo ta\tmo ta\tmo ta\tmo ta\tmo ta\tmo ta\r\n" +
		"1\t\tmot\t1\t\tghi chu\r\n" +
		"201\t400\t\t\t\t\r\n" +
		"0\t9\t\t1\t1\t\r\n" +
		"1276\t1277\thai\t1\t1\t\r\n" +
		"x\t5\t\t1\t\t\r\n" +
		"2881\t\t\t2\t1\t\r\n")
	d := ParseTaskDef(data)
	if len(d.Rows) != 4 {
		t.Fatalf("rows: %+v", d.Rows)
	}
	want := []TaskDefRow{
		{First: 1, Last: 1, Name: "mot", Sync: true},
		{First: 201, Last: 400},
		{First: 1276, Last: 1277, Name: "hai", Sync: true, Client: true},
		{First: 2881, Last: 2881, Client: true}, // a SYNC_FLAG of 2 is not 1
	}
	for i, w := range want {
		if d.Rows[i] != w {
			t.Fatalf("row %d: %+v, want %+v", i, d.Rows[i], w)
		}
	}
	// the ids the login sends: 1, 1276, 1277 (the row without a flag names none)
	if n := d.SyncCount(); n != 3 {
		t.Fatalf("sync count %d", n)
	}
	// a second row of the same first id is not a second range
	d.Rows = append(d.Rows, TaskDefRow{First: 1, Last: 5, Sync: true})
	if n := d.SyncCount(); n != 3 {
		t.Fatalf("sync count %d", n)
	}
	// nothing but the header
	if d := ParseTaskDef([]byte("TASK_ID_FIRST\tTASK_ID_LAST\n")); len(d.Rows) != 0 {
		t.Fatalf("rows: %+v", d.Rows)
	}
}
