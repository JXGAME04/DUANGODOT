# Hàm script bang hội / xưởng bang của `jx_linux_y` (sinh bởi `tools/re` + `scratch gen_tong_api.py`, 2026-09-19)

Khảo sát S8 (HANDOVER phần 71, `LINUX-SERVER.md` §37). Mỗi bang trong bản sao của zone (`0x081B2BD0()+0x24` = `std::map<id, KTong*>`, `+0x34` số bang) giữ **cây giá trị `KTong+0x768` = `map<khoá word, int>`** (mọi thuộc tính lẫn giá trị nhiệm vụ bang) và tên tại `KTong+0x38`; `TONG_Get*` đọc một khoá cố định (cột **Khoá**), `TONG_ApplySet*`/`ApplyAdd*` gọi vtable của đối tượng relay `0x081B2BD0()+0x258` (cột **Slot**: `0xa8` = đặt theo khoá, `0xac` = cộng theo khoá, `0xb0` = cộng quỹ) rồi máy chủ bang (`S2SSyncRelayD`, mã 2003 `MultiServer/S3Relay/KTongSet.h` + `KTongControl.h`) trả lời để cập nhật bản sao. `TWS_*` làm việc trên xưởng (`0x08196A60` = xưởng theo id) với map khoá riêng.

| Hàm | Địa chỉ | Kiểm `top` | Khoá đọc | Slot relay | Gọi |
|---|---|---|---|---|---|
| `Msg2Tong` | `0x08120E00` | ('cmp', '1', 'jle') | — | — | 0x80d09e0, 0x804aedc, 0x8065280, 0x804af8c |
| `CreateTong` | `0x0811B020` | ('test', 'eax', 'jle') | — | — | 0x80ce6f0, 0x80a8400, 0x807b8f0, 0x807b850 |
| `GetTongMTask` | `0x0812DC60` | ('cmp', '1', 'jle') | — | — | 0x82339b0, 0x80d7780, 0x80d0140 |
| `GetTongMaster` | `0x081144F0` | - | — | — | 0x804b70c |
| `GetTongFigure` | `0x081145B0` | - | — | — | — |
| `GetTongTitle` | `0x0811AF00` | ('test', 'eax', 'jle') | — | — | 0x80cec90, 0x80ce6f0, 0x80a8400, 0x807b8f0 |
| `GetTongCamp` | `0x08114560` | - | — | — | — |
| `GetTongName` | `0x0811AB30` | ('cmp', '2', 'jg') | — | — | 0x80d0780, 0x80ce670, 0x80d1280, 0x80ce760 |
| `GetTongMemberCount` | `0x0811AC80` | ('test', 'eax', 'jle') | — | — | 0x80ce760, 0x80cee60, 0x80d2b00 |
| `GetTong` | `0x0811AAB0` | ('cmp', '2', 'jg') | — | — | 0x80d0780, 0x80ce670, 0x80d1280 |
| `GetTongMemberID` | `0x08114600` | ('cmp', '1', 'jle') | — | — | — |
| `GetTongWeek` | `0x08113E10` | - | — | — | — |
| `TONG_GetTongCount` | `0x0818A6C0` | ('cmp', '3', 'jle') | — | 0xf0 TWS AddU | 0x81b2bd0 |
| `TONG_GetFirstTong` | `0x0818A680` | ('cmp', '3', 'jle') | — | 0xf0 TWS AddU | 0x81b2bd0 |
| `TONG_GetNextTong` | `0x0818B4C0` | ('test', 'eax', 'jle') | — | — | 0x81b2bd0, 0x804b31c, 0x821d6f0 |
| `TONG_GetTongByRoleName` | `0x0819A700` | ('test', 'eax', 'jle') | — | — | 0x81b2bd0, 0x804b2dc |
| `TONG_GetName` | `0x0818AC80` | ('test', 'eax', 'jle') | — | 0xb0 AddValue2 (quỹ) | 0x81b2bd0 |
| `TONG_WriteLog` | `0x0818B5A0` | ('test', 'eax', 'jle') | — | — | 0x821d6f0, 0x821df00, 0x818a4c0 |
| `TONG_Name2ID` | `0x0818B600` | ('test', 'eax', 'jle') | — | — | 0x821df00, 0x818a4c0 |
| `TONG_IsExist` | `0x08190890` | - | — | 0x38 SetStunt/IsExist | 0x818bdb0, 0x818a4c0, 0x81b2bd0 |
| `TONG_GetSelfCamp` | `0x0818C530` | - | 1 | — | 0x818bdb0 |
| `TONG_GetCurCamp` | `0x0818C670` | - | 2 | — | 0x818bdb0 |
| `TONG_GetCredit` | `0x0818C7B0` | - | 5 | — | 0x818bdb0 |
| `TONG_GetExp` | `0x0818C8F0` | - | 6 | — | 0x818bdb0 |
| `TONG_GetExpLevel` | `0x0818CA30` | - | 6 | — | 0x818bdb0 |
| `TONG_GetWarState` | `0x0818CD30` | - | 0xb | — | 0x818bdb0 |
| `TONG_GetUnionID` | `0x0818CBF0` | - | 0xa | — | 0x818bdb0 |
| `TONG_GetBuildLevel` | `0x0818D600` | - | 0xd | — | 0x818bdb0 |
| `TONG_GetPremium` | `0x0818D740` | - | 0xe | — | 0x818bdb0 |
| `TONG_GetMoney` | `0x0818F900` | - | 4, 3 | 0xbc SetMoney | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplySetMoney` | `0x0818FA10` | - | — | 0xbc SetMoney, 0xc MemberCount / Init | 0x818bdb0, 0x81b2bd0, 0x818a4c0 |
| `TONG_ApplyAddMoney` | `0x0818BE50` | - | — | 0xc0 AddMoney, 0xc4 AddEventRecord(bang, thời điểm, chữ) | 0x818bdb0, 0x81b2bd0, 0x821d7c0, 0x8234090 |
| `TONG_ApplySetAnnouncement` | `0x0818C140` | - | — | 0x2c Announce | 0x818bdb0, 0x818a4c0, 0x81b2bd0 |
| `TONG_ApplyAddEventRecord` | `0x0818BFE0` | - | — | 0xc4 AddEventRecord(bang, thời điểm, chữ), 0xc8 AddHistoryRecord, 0x2c Announce | 0x818bdb0, 0x804b02c, 0x81b2bd0, 0x818a4c0 |
| `TONG_ApplyAddHistoryRecord` | `0x0818C090` | - | — | 0xc8 AddHistoryRecord, 0x2c Announce | 0x818bdb0, 0x804b02c, 0x81b2bd0, 0x818a4c0 |
| `TONG_GetBuildFund` | `0x0818D560` | - | 0xc, 0xd | — | 0x818bdb0 |
| `TONG_ApplySetBuildFund` | `0x08192CD0` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddBuildFund` | `0x08194650` | - | — | 0xb0 AddValue2 (quỹ) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWarBuildFund` | `0x0818D920` | - | 0xf, 0x2c | — | 0x818bdb0 |
| `TONG_ApplySetWarBuildFund` | `0x08192A90` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddWarBuildFund` | `0x08194410` | - | — | 0xb0 AddValue2 (quỹ) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetMaintainFund` | `0x0818DC40` | - | 0x10 | — | 0x818bdb0 |
| `TONG_ApplySetMaintainFund` | `0x08192850` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetPauseState` | `0x0818D9C0` | - | 0x2c | — | 0x818bdb0 |
| `TONG_ApplySetPauseState` | `0x081929D0` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetStoredOffer` | `0x0818DB00` | - | 0x12 | — | 0x818bdb0 |
| `TONG_ApplySetStoredOffer` | `0x08192910` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddStoredOffer` | `0x08195C10` | - | — | 0xac AddValue(bang, khoá, cộng) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetDay` | `0x0818DEC0` | - | 0x14 | — | 0x818bdb0 |
| `TONG_ApplySetDay` | `0x081926D0` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddDay` | `0x081959D0` | - | — | 0xac AddValue(bang, khoá, cộng) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWeek` | `0x0818E000` | - | 0x15 | — | 0x818bdb0 |
| `TONG_ApplySetWeek` | `0x08192610` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddWeek` | `0x08195910` | - | — | 0xac AddValue(bang, khoá, cộng) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWeekGoalEvent` | `0x0818E140` | - | 0x16 | — | 0x818bdb0 |
| `TONG_ApplySetWeekGoalEvent` | `0x08192550` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWeekGoalLevel` | `0x0818E280` | - | 0x17 | — | 0x818bdb0 |
| `TONG_ApplySetWeekGoalLevel` | `0x08192490` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWeekGoalTotal` | `0x0818E3C0` | - | 0x18 | — | 0x818bdb0 |
| `TONG_ApplySetWeekGoalTotal` | `0x081923D0` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWeekGoalPlayer` | `0x0818E500` | - | 0x19 | — | 0x818bdb0 |
| `TONG_ApplySetWeekGoalPlayer` | `0x08192310` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWeekGoalValue` | `0x0818E640` | - | 0x1a | — | 0x818bdb0 |
| `TONG_ApplySetWeekGoalValue` | `0x08192250` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddWeekGoalValue` | `0x08195550` | - | — | 0xac AddValue(bang, khoá, cộng) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWeekGoalPriceTong` | `0x0818E780` | - | 0x1b | — | 0x818bdb0 |
| `TONG_ApplySetWeekGoalPriceTong` | `0x08192190` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWeekGoalPricePlayer` | `0x0818E8C0` | - | 0x1c | — | 0x818bdb0 |
| `TONG_ApplySetWeekGoalPricePlayer` | `0x081920D0` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetLWeekGoalEvent` | `0x0818EA00` | - | 0x1d | — | 0x818bdb0 |
| `TONG_ApplySetLWeekGoalEvent` | `0x08192010` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetLWeekGoalLevel` | `0x0818EB40` | - | 0x1e | — | 0x818bdb0 |
| `TONG_ApplySetLWeekGoalLevel` | `0x08191F50` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetLWeekGoalTotal` | `0x0818EC80` | - | 0x1f | — | 0x818bdb0 |
| `TONG_ApplySetLWeekGoalTotal` | `0x08191E90` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetLWeekGoalPlayer` | `0x0818EDC0` | - | 0x20 | — | 0x818bdb0 |
| `TONG_ApplySetLWeekGoalPlayer` | `0x08191DD0` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetLWeekGoalValue` | `0x0818EF00` | - | 0x21 | — | 0x818bdb0 |
| `TONG_ApplySetLWeekGoalValue` | `0x08191D10` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddLWeekGoalValue` | `0x08195010` | - | — | 0xac AddValue(bang, khoá, cộng) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetLWeekGoalPriceTong` | `0x0818F040` | - | 0x22 | — | 0x818bdb0 |
| `TONG_ApplySetLWeekGoalPriceTong` | `0x08191C50` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetLWeekGoalPricePlayer` | `0x0818F180` | - | 0x23 | — | 0x818bdb0 |
| `TONG_ApplySetLWeekGoalPricePlayer` | `0x08191B90` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetCurWeekGoalLevel` | `0x0818F2C0` | - | 0x24 | — | 0x818bdb0 |
| `TONG_ApplySetCurWeekGoalLevel` | `0x08191AD0` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWeekBuildFund` | `0x0818F400` | - | 0x29 | — | 0x818bdb0 |
| `TONG_ApplySetWeekBuildFund` | `0x08191A10` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddWeekBuildFund` | `0x08194D10` | - | — | 0xac AddValue(bang, khoá, cộng) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetWeekBuildUpper` | `0x0818F540` | - | 0x2a | — | 0x818bdb0 |
| `TONG_ApplySetWeekBuildUpper` | `0x08191950` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetTotalBuildFund` | `0x0818F680` | - | 0x2b | — | 0x818bdb0 |
| `TONG_ApplySetTotalBuildFund` | `0x08191890` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddTotalBuildFund` | `0x08194B90` | - | — | 0xac AddValue(bang, khoá, cộng) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetPerStandFund` | `0x0818F860` | - | 0x11, 4, 3 | — | 0x818bdb0 |
| `TONG_ApplySetPerStandFund` | `0x081917D0` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddPerStandFund` | `0x08193150` | - | — | 0xb0 AddValue2 (quỹ) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetStoredBuildFund` | `0x0818DD80` | - | 0x13 | — | 0x818bdb0 |
| `TONG_ApplySetStoredBuildFund` | `0x08192790` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyAddStoredBuildFund` | `0x08195A90` | - | — | 0xac AddValue(bang, khoá, cộng) | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetStandFund` | `0x0818CF10` | - | 0x11, 0x2d | — | 0x818bdb0 |
| `TONG_GetTongMap` | `0x0818D060` | - | 0x2d, 0x2e | — | 0x818bdb0 |
| `TONG_ApplySetTongMap` | `0x08190670` | - | — | 0x34 SetTongMap, 0xd4 Map | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetTongMapTemplate` | `0x0818D1A0` | - | 0x2e, 0x2f | — | 0x818bdb0 |
| `TONG_GetTongMapBan` | `0x0818D240` | - | 0x2f | — | 0x818bdb0 |
| `TONG_ApplySetTongMapBan` | `0x0818AF30` | ('cmp', '1', 'jle') | — | 0xe0 SetTaskValue/MapBan | 0x818a4c0, 0x81b2bd0 |
| `TONG_GetOnlineCount` | `0x0818FFF0` | ('cmp', '1', 'jle') | — | 0xc MemberCount / Init | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetOccupyCityDay` | `0x0818D380` | - | 0x30 | — | 0x818bdb0 |
| `TONG_ApplySetOccupyCityDay` | `0x08192D90` | - | — | 0xa8 SetValue(bang, khoá, giá trị) | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyInit` | `0x0818FAC0` | - | 0xd | 0xc MemberCount / Init | 0x818bdb0, 0x81b2bd0, 0x818a4c0 |
| `TONG_ApplyUpgrade` | `0x0818FB40` | - | 0xd | — | 0x818bdb0, 0x818a4c0, 0x81b2bd0 |
| `TONG_ApplyDegrade` | `0x0818FCC0` | - | 0xd | 0x18 Degrade/Maintain | 0x818bdb0, 0x818a4c0, 0x81b2bd0 |
| `TONG_ApplyMaintain` | `0x0818FE40` | ('cmp', '1', 'jle') | — | 0x18 Degrade/Maintain, 0x1c Maintain | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyWeeklyMaintain` | `0x0818FEC0` | ('cmp', '1', 'jle') | — | 0x1c Maintain, 0xc MemberCount / Init | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetTaskValue` | `0x081900A0` | - | — | — | 0x818bdb0 |
| `TONG_GetUTaskValue` | `0x08190180` | - | — | — | 0x818bdb0 |
| `TONG_ApplySetTaskValue` | `0x0818AE40` | ('cmp', '2', 'jle') | — | 0xa8 SetValue(bang, khoá, giá trị), 0xe0 SetTaskValue/MapBan | 0x81b2bd0, 0x818a4c0 |
| `TONG_ApplyAddTaskValue` | `0x08196450` | ('cmp', '2', 'jle') | — | 0xac AddValue(bang, khoá, cộng) | 0x81b2bd0, 0x818b8c0 |
| `TONG_ApplyAddUTaskValue` | `0x0818AD50` | ('cmp', '2', 'jle') | — | 0xb0 AddValue2 (quỹ), 0xa8 SetValue(bang, khoá, giá trị) | 0x81b2bd0 |
| `TONG_SetTaskTemp` | `0x0818B350` | ('test', 'eax', 'jle') | — | — | 0x818b1e0, 0x81b2bd0 |
| `TONG_AddTaskTemp` | `0x0818B280` | - | — | — | 0x818b1e0 |
| `TONG_GetTaskTemp` | `0x08190260` | - | — | — | 0x818bdb0, 0x818a4c0 |
| `TONG_GetMaster` | `0x08197EC0` | ('cmp', '2', 'jle') | — | — | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetMemberCount` | `0x0818FF40` | ('cmp', '1', 'jle') | — | 0xc MemberCount / Init | 0x818bdb0, 0x81b2bd0 |
| `TONG_GetFirstMember` | `0x08197880` | - | — | — | 0x818bdb0 |
| `TONG_GetNextMember` | `0x08197C10` | - | — | — | 0x818bdb0, 0x804b31c |
| `TONG_ApplyKickMember` | `0x08190310` | - | — | 0x7c KickMember | 0x818bdb0, 0x818a4c0, 0x81b2bd0 |
| `TONG_ApplyDeleteMember` | `0x08190510` | - | — | 0x11c DeleteMember, 0x30 DeleteMember/CreatMap | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyCreatMap` | `0x081905C0` | - | — | 0x30 DeleteMember/CreatMap, 0x34 SetTongMap, 0xd4 Map | 0x818bdb0, 0x81b2bd0 |
| `TONG_ApplyDeleteMap` | `0x08190720` | - | — | 0xd4 Map, 0x100 DeleteMap/ChangeFeature | 0x818bdb0, 0x81b2bd0 |
| `TONG_ChangeAllMemberFeature` | `0x081907A0` | - | — | 0x100 DeleteMap/ChangeFeature | 0x818bdb0, 0x81b2bd0, 0x818a4c0 |
| `TONG_ApplySetStunt` | `0x081908F0` | - | — | 0x38 SetStunt/IsExist, 0xfc SetStunt | 0x818bdb0, 0x818a4c0, 0x81b2bd0 |
| `TWS_IsExist` | `0x08196B90` | - | — | — | 0x8196a60 |
| `TWS_GetWorkshopCount` | `0x0818C3D0` | ('cmp', '1', 'jle') | — | — | 0x818bdb0, 0x804b31c |
| `TWS_GetFirstWorkshop` | `0x08190AB0` | ('cmp', '1', 'jle') | 0x15 | — | 0x818bdb0, 0x8232be0 |
| `TWS_GetNextWorkshop` | `0x0818C200` | - | — | — | 0x818bdb0, 0x804b31c |
| `TWS_GetType` | `0x081977E0` | - | 1 | — | 0x8196a60, 0x818bdb0 |
| `TWS_GetLevel` | `0x081976A0` | - | 2, 1 | — | 0x8196a60 |
| `TWS_GetUseLevel` | `0x08197560` | - | 3, 2 | — | 0x8196a60 |
| `TWS_ApplySetUseLevel` | `0x0818A930` | ('cmp', '2', 'jle') | — | 0xe4 TWS SetUseLevel, 0xe8 TWS Set | 0x81b2bd0 |
| `TWS_ApplySetUseLevelSet` | `0x0818AB40` | ('cmp', '2', 'jle') | — | 0x4c TWS UseLevelSet | 0x818a4c0, 0x81b2bd0 |
| `TWS_GetUseLevelSet` | `0x08197420` | - | 6, 3 | — | 0x8196a60 |
| `TWS_GetDayOutput` | `0x08197240` | - | 5 | — | 0x8196a60 |
| `TWS_ApplySetDayOutput` | `0x08197160` | - | 5 | 0xe8 TWS Set | 0x8196a60, 0x81b2bd0 |
| `TWS_ApplyAddDayOutput` | `0x08196FA0` | - | — | 0xf0 TWS AddU, 0xec TWS Add | 0x8196a60, 0x81b2bd0 |
| `TWS_IsOpen` | `0x08196D50` | - | 4 | — | 0x8196a60 |
| `TWS_GetBuildingNpc` | `0x08196DF0` | - | 4 | — | 0x8196a60 |
| `TWS_SetBuildingNpc` | `0x0818B150` | - | — | — | 0x818b020, 0x81b2bd0, 0x818b1e0 |
| `TWS_GetTaskValue` | `0x08196CA0` | - | 4 | — | 0x8196a60 |
| `TWS_GetUTaskValue` | `0x08196BF0` | - | — | — | 0x8196a60 |
| `TWS_ApplySetTaskValue` | `0x0818AA20` | ('cmp', '3', 'jle') | — | 0xe8 TWS Set, 0x4c TWS UseLevelSet | 0x81b2bd0, 0x818a4c0 |
| `TWS_ApplyAddTaskValue` | `0x0818A810` | ('cmp', '3', 'jle') | — | 0xec TWS Add, 0xe4 TWS SetUseLevel | 0x81b2bd0 |
| `TWS_ApplyAddUTaskValue` | `0x0818A6F0` | ('cmp', '3', 'jle') | — | 0xf0 TWS AddU | 0x81b2bd0 |
| `TWS_ApplyAdd` | `0x0818BCE0` | - | — | 0x40 TWS Add | 0x818b8c0, 0x81b2bd0, 0x818bdb0 |
| `TWS_ApplyRemove` | `0x0818BC20` | - | — | 0x44 TWS Remove, 0x40 TWS Add | 0x818b8c0, 0x81b2bd0 |
| `TWS_ApplyOpen` | `0x0818BB60` | - | — | 0x48 TWS Open, 0x44 TWS Remove | 0x818b8c0, 0x81b2bd0 |
| `TWS_ApplyClose` | `0x0818BAA0` | - | — | 0x50 TWS Close, 0x48 TWS Open | 0x818b8c0, 0x81b2bd0 |
| `TWS_ApplyUpgrade` | `0x08196700` | - | — | — | 0x818b8c0, 0x81b2bd0 |
| `TWS_ApplyDegrade` | `0x08196540` | - | — | — | 0x818b8c0, 0x81b2bd0 |
| `TWS_ApplyUse` | `0x0818B7F0` | - | — | 0x5c TWS Use | 0x818b680, 0x81b2bd0, 0x818a4c0, 0x821df00 |
| `TWS_ApplyMaintain` | `0x0818B9E0` | - | — | 0x60 TWS Maintain, 0x50 TWS Close | 0x818b8c0, 0x81b2bd0 |
| `GetTongLogData` | `0x08190BB0` | - | 0x15 | — | 0x818bdb0, 0x8232be0, 0x8233430 |
| `TONG_ContributeOffer` | `0x081968C0` | ('cmp', '2', 'jle') | — | 0xa4 Offer, 0xf8 Offer, 0xac AddValue(bang, khoá, cộng) | 0x81b2bd0 |
| `TONG_DistributeOfferToMember` | `0x08197A70` | ('cmp', '2', 'jle') | — | 0xa4 Offer, 0xac AddValue(bang, khoá, cộng), 0xf8 Offer | 0x81b2bd0, 0x818bdb0 |
| `TONG_DistributeOfferToGroup` | `0x08197F70` | ('cmp', '2', 'jle') | — | 0xc MemberCount / Init, 0xa4 Offer, 0xac AddValue(bang, khoá, cộng), 0xfc SetStunt | 0x81b2bd0 |

**Khoá đã đối chiếu tên hàm**: 1 `SelfCamp`, 2 `CurCamp`, 3/4 `Money` (thấp/cao), 5 `Credit`, 6 `Exp`/`ExpLevel`, 0xa `UnionID`, 0xb `WarState`, 0xc `BuildFund`, 0xd `BuildLevel`, 0xe `Premium`, 0xf `WarBuildFund`, 0x10 `MaintainFund`, 0x11 `PerStandFund`/`StandFund`, 0x12 `StoredOffer`, 0x13 `StoredBuildFund`, 0x14 `Day`, 0x15 `Week`, 0x16..0x1c `WeekGoal Event/Level/Total/Player/Value/PriceTong/PricePlayer`, 0x1d..0x23 `LWeekGoal …`, 0x24 `CurWeekGoalLevel`, 0x29 `WeekBuildFund`, 0x2a `WeekBuildUpper`, 0x2b `TotalBuildFund`, 0x2c `PauseState`, 0x2d `TongMap`, 0x2e `TongMapTemplate`, 0x2f `TongMapBan`, 0x30 `OccupyCityDay`; các khoá khác do script đặt (`TONG_GetTaskValue(bang, khoá)`).
