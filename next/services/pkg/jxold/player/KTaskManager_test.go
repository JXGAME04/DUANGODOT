package player

import "testing"

func TestParseTaskTables(t *testing.T) {
	files := map[string]string{
		"settings/task/task_id.txt": "TaskID\tTaskName\tEventID\tTaskType\tCanCancel\tTaskText\r\n" +
			"ID nv\tTen nv\tID sk\tLoai\tco the huy\tMieu ta\r\n" +
			"101\tNgau nhien_Doi thoai_A\t3\tNgau nhien_Doi thoai\t0\tTang hoa\r\n" +
			"102\tNgau nhien_Thu thap_B\t3\tNgau nhien_Thu thap\t1\tGiao \xb8\r\n",
		"settings/task/task_event.txt": "EventID\tEventName\tEventText\r\n1\tPhuong thuc test\tthu\r\n3\tNhiem vu ban\tngau nhien\r\n",
		"settings/task/task_type.txt": "TaskType\tConditionFile\tEntityFile\tAwardFile\tTalkFile\r\n" +
			"Ngau nhien_Doi thoai\tsettings\\task\\random\\talk\\condition.txt\tsettings\\task\\random\\talk\\entity.txt\tsettings\\task\\random\\talk\\award.txt\tsettings\\task\\random\\talk\\talk.txt\r\n" +
			"Ngau nhien_Thu thap\t\tsettings\\task\\random\\coll\\entity.txt\t\t\r\n",
		"settings/task/random/talk/condition.txt": "TaskName\tTaskType\tTaskDesc\r\nTen\tLoai\tMo ta\r\nNgau nhien_Doi thoai_A\tdang cap lon\t5\r\nNgau nhien_Doi thoai_A\tvat pham\t1,2,3\r\n",
		"settings/task/random/talk/entity.txt":    "TaskName\tTaskType\tGenre\r\nNgau nhien_Doi thoai_A\tdoi thoai\t\r\n",
		"settings/task/random/talk/award.txt":     "TaskName\tAwardType\r\n",
		"settings/task/random/talk/talk.txt":      "TaskName\tTaskStart\r\nNgau nhien_Doi thoai_A\t<dec>xin chao\r\n",
		"settings/task/random/coll/entity.txt":    "TaskName\tTaskType\r\nNgau nhien_Thu thap_B\tthu thap\r\n",
	}
	read := func(rel string) ([]byte, error) {
		s, ok := files[rel]
		if !ok {
			return nil, errNoFile
		}
		return []byte(s), nil
	}
	tt, err := ParseTaskTables(read)
	if err != nil {
		t.Fatal(err)
	}
	// task_id.txt: the description row is a record too (the loader starts at row 2 and never skips)
	if len(tt.Tasks) != 3 || tt.Tasks[0].ID != 0 || tt.Tasks[1].ID != 101 || tt.Tasks[2].ID != 102 {
		t.Fatalf("tasks: %+v", tt.Tasks)
	}
	if tt.Tasks[1].Name != "Ngau nhien_Doi thoai_A" || tt.Tasks[1].Event != 3 || tt.Tasks[1].Type != "Ngau nhien_Doi thoai" {
		t.Fatalf("task 101: %+v", tt.Tasks[1])
	}
	// the cells are the columns after the key: TaskName, EventID, TaskType, CanCancel, TaskText
	if len(tt.Tasks[2].Cells) != 5 || tt.Tasks[2].Cells[3] != "1" || tt.Tasks[2].Cells[4] != "Giao ¸" {
		t.Fatalf("cells: %q", tt.Tasks[2].Cells)
	}
	if len(tt.Events) != 2 || tt.Events[1].Key != "3" || tt.Events[1].Cells[0] != "Nhiem vu ban" {
		t.Fatalf("events: %+v", tt.Events)
	}
	if len(tt.Types) != 2 || tt.Types[0].Name != "Ngau nhien_Doi thoai" || len(tt.Types[0].Condition) != 3 {
		t.Fatalf("types: %+v", tt.Types)
	}
	// the condition table: two rows of the task after the description row, two cells each (the key column left out)
	c := tt.Types[0].Condition
	if c[1].Key != "Ngau nhien_Doi thoai_A" || len(c[1].Cells) != 2 || c[1].Cells[0] != "dang cap lon" || c[2].Cells[1] != "1,2,3" {
		t.Fatalf("condition: %+v", c)
	}
	if len(tt.Types[0].Award) != 0 || len(tt.Types[0].Talk) != 1 || tt.Types[0].Talk[0].Cells[0] != "<dec>xin chao" {
		t.Fatalf("award / talk: %+v %+v", tt.Types[0].Award, tt.Types[0].Talk)
	}
	// a kind without three of its files keeps them empty
	if len(tt.Types[1].Condition) != 0 || len(tt.Types[1].Entity) != 1 {
		t.Fatalf("type 2: %+v", tt.Types[1])
	}
	// without task_id.txt nothing loads
	if _, err := ParseTaskTables(func(string) ([]byte, error) { return nil, errNoFile }); err == nil {
		t.Fatal("expected an error without task_id.txt")
	}
}

func TestTaskKeyHash(t *testing.T) {
	// the values of the binary's hash (0x0821DF00) for the TmpType names of task_head.lua
	cases := map[string]uint32{"TalkNpc": 0xfa7e2d03, "KillNpc": 0xc947adf0, "Collect": 0xa26cb6dc, "ItemNpc": 0x3fe53c11, "a": 0xedcbaff7, "": 0x12345678}
	for s, want := range cases {
		if got := TaskKeyHash(s); got != want {
			t.Fatalf("%q: %#x, want %#x", s, got, want)
		}
	}
}

type noFile struct{}

func (noFile) Error() string { return "no file" }

var errNoFile error = noFile{}
