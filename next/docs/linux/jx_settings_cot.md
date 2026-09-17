# Bảng settings của jx_linux_y — cột / khoá mà mã đọc

Sinh bởi `tools/re/re_tables.py report`. Mỗi tệp: đối tượng bảng (toàn cục, thành viên lớp hay
trên stack), hàm nạp, và các cột (`KTabFile::Get*(nRow, szColumn)`) hoặc khoá
(`KIniFile::Get*(section, key)`) được đọc, kèm hàm đọc. `đọc theo chỉ số cột` = `Get*(nRow, nColumn)`:
tệp được đọc từng cột theo số thứ tự, không có tên cột trong mã.

## `(đường dẫn ghép lúc chạy, không thấy mẫu)`
- bảng cục bộ `[ebp-0x78]`, nạp ở `0x08082E20`, `0x08085E70`, `0x0809FBD0`, `0x0811BB10`, `0x0811BF40`, **16 cột/khoá**
  - `[(section ghép lúc chạy)] DetailType` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] DetailType2` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ForbitDrop` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ForbitMove` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ForbitSell` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ForbitThrowAway` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ForbitTrade` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] Genre` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] IsValuable` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] IsWrapable` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] Level` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ParticulType` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ParticulType2` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] Quality` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] TypeName` — đọc ở `0x08064D40`
  - `[Info] Count` — đọc ở `0x08064D40`
- bảng cục bộ `[ebp-0x44]`, nạp ở `0x08070E20`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08070E20`
- bảng cục bộ `[ebp-0x44]`, nạp ở `0x08071050`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08071050`
- bảng cục bộ `[ebp-0x9C]`, nạp ở `0x080A41E0`, **27 cột/khoá**
  - `[(section ghép lúc chạy)] (khoá ghép lúc chạy)` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Detail` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] EnchasableRate` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Genre` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MaxItemLevel` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MaxSocket` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MinItemLevel` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MinSocket` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Particular` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Quality` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] RandRate` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Series` — đọc ở `0x080A3B80`
  - `[Main] Count` — đọc ở `0x080A3B80`
  - `[Main] EnchasableRate` — đọc ở `0x080A3B80`
  - `[Main] IsTeamShare` — đọc ở `0x080A3B80`
  - `[Main] MagicRate` — đọc ở `0x080A3B80`
  - `[Main] MaxItemLevel` — đọc ở `0x080A3B80`
  - `[Main] MaxItemLevelScale` — đọc ở `0x080A3B80`
  - `[Main] MaxSocket` — đọc ở `0x080A3B80`
  - `[Main] MinItemLevel` — đọc ở `0x080A3B80`
  - `[Main] MinItemLevelScale` — đọc ở `0x080A3B80`
  - `[Main] MinSocket` — đọc ở `0x080A3B80`
  - `[Main] MoneyRate` — đọc ở `0x080A3B80`
  - `[Main] MoneyScale` — đọc ở `0x080A3B80`
  - `[Main] RandRange` — đọc ở `0x080A3B80`
  - `[Main] Series` — đọc ở `0x080A3B80`
  - `[Main] TeamShareRate` — đọc ở `0x080A3B80`
- bảng cục bộ `[ebp-0x144]`, nạp ở `0x080F1110`, **6 cột/khoá**
  - `[MAIN] IsInDoor` — đọc ở `0x080F1110`
  - `[MAIN] rect` — đọc ở `0x080F1110`
  - `[Weather] (khoá ghép lúc chạy)` — đọc ở `0x080FB430`
  - `[Weather] HappenTimeMax` — đọc ở `0x080FB430`
  - `[Weather] HappenTimeMin` — đọc ở `0x080FB430`
  - `[Weather] WeatherNum` — đọc ở `0x080FB430`
- đối tượng cục bộ (con trỏ chưa theo dõi được), nạp ở `0x0814AFB0`, **0 cột/khoá**
- đối tượng cục bộ (con trỏ chưa theo dõi được), nạp ở `0x0814B160`, **0 cột/khoá**
- thành viên `this+0x4`, nạp ở `0x08152840`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08152840`
- bảng cục bộ `[ebp-0x5C]`, nạp ở `0x08172030`, `0x081725F0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08170990`
- bảng cục bộ `[ebp-0x70]`, nạp ở `0x0817D870`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x0817D870`
- bảng cục bộ `[ebp-0x38]`, nạp ở `0x081F06E0`, **0 cột/khoá**
- bảng cục bộ `[ebp-0xCC]`, nạp ở `0x08211AD0`, **1 cột/khoá**
  - `[?] (khoá ghép lúc chạy)` — đọc ở `0x08211AD0`
- bảng cục bộ `[ebp-0x74]`, nạp ở `0x08218340`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08215AF0`
- bảng cục bộ `[ebp-0x80]`, nạp ở `0x08218340`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x082169C0`
- bảng cục bộ `[ebp-0x4C]`, nạp ở `0x08218340`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08217120`
- bảng cục bộ `[ebp-0x64]`, nạp ở `0x0821BFD0`, **0 cột/khoá**
- bảng cục bộ `[ebp-0x84]`, nạp ở `0x0821BFD0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x0821BFD0`

## `\AntiBotConfig.ini`
- đối tượng `0x09781100`, nạp ở `0x08140200`, **11 cột/khoá**, ghi 7 khoá
  - `[AntiBot] DelayCatchTime` — đọc ở `0x08140200`
  - `[AntiBot] DeleteAccount` — đọc ở `0x08140200`
  - `[AntiBot] Punish` — đọc ở `0x08140200`
  - `[ClientSign] (khoá ghép lúc chạy)` — đọc ở `0x08141050`
  - `[ClientSign] CheckInterval` — đọc ở `0x08141050`
  - `[ClientSign] Count` — đọc ở `0x08141050`
  - `[ClientSign] TimeLimit` — đọc ở `0x08141050`
  - `[SignCode] (khoá ghép lúc chạy)` — đọc ở `0x08141050`
  - `[SignCode] Count` — đọc ở `0x08141050`
  - `[SignCode] CurRecord` — đọc ở `0x08141050`
  - `[SignCode] SyncPassTime` — đọc ở `0x08141050`
  - `[AntiBot] DelayCatchTime` — **ghi** ở `0x08140110`, `0x08140200`
  - `[AntiBot] DeleteAccount` — **ghi** ở `0x08140160`, `0x08140200`
  - `[AntiBot] Punish` — **ghi** ở `0x081401B0`, `0x08140200`
  - `[SignCode] (khoá ghép lúc chạy)` — **ghi** ở `0x08140A70`, `0x08140BC0`
  - `[SignCode] Count` — **ghi** ở `0x08140200`, `0x08140A70`, `0x08140BC0`
  - `[SignCode] CurRecord` — **ghi** ở `0x08140200`, `0x08140A70`, `0x08140BC0`
  - `[SignCode] SyncPassTime` — **ghi** ở `0x08140200`, `0x081408B0`

## `\lang\%s\replacename_npc.txt (đường dẫn ghép lúc chạy)`
- bảng cục bộ `[ebp-0x5C]`, nạp ở `0x080A07B0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080A14E0`

## `\lang\%s\replacename_obj.txt (đường dẫn ghép lúc chạy)`
- bảng cục bộ `[ebp-0x5C]`, nạp ở `0x080A7880`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080A14E0`

## `\lang\%s\stringtable_core.txt (đường dẫn ghép lúc chạy)`
- bảng cục bộ `[ebp-0x5C]`, nạp ở `0x0805DEC0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080A14E0`
- đối tượng `0x09782248`, nạp ở `0x0805DEC0`, **0 cột/khoá**
- đối tượng `0x097827C4`, nạp ở `0x0805DEC0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x081535B0`

## `\magicattrib_limit.txt (đường dẫn ghép lúc chạy)`
- bảng cục bộ `[ebp-0x8C]`, nạp ở `0x0806BE00`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x0806BE00`

## `\maps\WorldSet.ini`
- bảng cục bộ `[ebp-0x60]`, nạp ở `0x080F7210`, **1 cột/khoá**
  - `[Init] ServerID` — đọc ở `0x080F7210`

## `\rolenamechangehis.ini`
- bảng cục bộ `[ebp-0x70]`, nạp ở `0x081D9A90`, **0 cột/khoá**, ghi 4 khoá
  - `[(section ghép lúc chạy)] Account` — **ghi** ở `0x081D9A90`
  - `[(section ghép lúc chạy)] NewName` — **ghi** ở `0x081D9A90`
  - `[(section ghép lúc chạy)] OldName` — **ghi** ở `0x081D9A90`
  - `[Global] Count` — **ghi** ở `0x081D9A90`
- đối tượng `0x097AC278`, nạp ở `0x081DA120`, **4 cột/khoá**
  - `[(section ghép lúc chạy)] Account` — đọc ở `0x081DA120`
  - `[(section ghép lúc chạy)] NewName` — đọc ở `0x081DA120`
  - `[(section ghép lúc chạy)] OldName` — đọc ở `0x081DA120`
  - `[Global] Count` — đọc ở `0x081DA120`

## `\settings\achievement\task_data.txt`
- bảng cục bộ `[ebp-0x38]`, nạp ở `0x081E9D60`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x081E91B0`

## `\settings\achievement\use_data.txt`
- bảng cục bộ `[ebp-0x40]`, nạp ở `0x0805EA10`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x081E9D60`

## `\settings\adventure.txt`
- đối tượng `0x09780AA0`, nạp ở `0x0805D580`, **3 cột/khoá**
  - `MapId` — đọc ở `0x0820A460`
  - `PosX` — đọc ở `0x0820A460`
  - `PosY` — đọc ở `0x0820A460`

## `\settings\attribconstdata.ini`
- bảng cục bộ `[ebp-0xD4]`, nạp ở `0x080E7200`, **2 cột/khoá**
  - `[?] (khoá ghép lúc chạy)` — đọc ở `0x080E7200`
  - `[?] Count` — đọc ở `0x080E7200`

## `\settings\auction.ini`
- bảng cục bộ `[ebp-0x70]`, nạp ở `0x0805EA10`, **13 cột/khoá**
  - `[Main] AddPricePerTime` — đọc ở `0x081ECC00`
  - `[Main] AuctionTime` — đọc ở `0x081ECC00`
  - `[Main] CountDown` — đọc ở `0x081ECC00`
  - `[Main] FeePerRound` — đọc ở `0x081ECC00`
  - `[Main] ItemsPerRound` — đọc ở `0x081ECC00`
  - `[Main] MaxAuctionPerCity` — đọc ở `0x081ECC00`
  - `[Main] MaxAuctionPerPlayer` — đọc ở `0x081ECC00`
  - `[Main] MaxBidderPerItem` — đọc ở `0x081ECC00`
  - `[Main] MaxLadderCount` — đọc ở `0x081ECC00`
  - `[Main] MaxPrearrangeRounds` — đọc ở `0x081ECC00`
  - `[Main] MinBasePrice` — đọc ở `0x081ECC00`
  - `[Main] ResultKeepTime` — đọc ở `0x081ECC00`
  - `[Main] TaxRate` — đọc ở `0x081ECC00`

## `\settings\buysell.txt`
- bảng cục bộ `[ebp-0x38]`, nạp ở `0x081F0C70`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x081F0C70`

## `\settings\citywar.ini`
- bảng cục bộ `[ebp-0x138]`, nạp ở `0x081DF990`, **15 cột/khoá**
  - `[CityArea] (khoá ghép lúc chạy)` — đọc ở `0x0805B3B0`
  - `[CityArea] StallTax` — đọc ở `0x0805B3B0`
  - `[CityArea] StoreTax` — đọc ở `0x0805B3B0`
  - `[CityArea] TaxScale` — đọc ở `0x0805B3B0`
  - `[CitySettings] EndSetTaxTime` — đọc ở `0x0805B3B0`
  - `[CitySettings] MaxExchangeTax` — đọc ở `0x0805B3B0`
  - `[CitySettings] MaxPriceParam` — đọc ở `0x0805B3B0`
  - `[CitySettings] MinTongCrowNumber` — đọc ở `0x0805B3B0`
  - `[CitySettings] MinTongLevel` — đọc ở `0x0805B3B0`
  - `[CitySettings] SignUpFee` — đọc ở `0x0805B3B0`
  - `[CitySettings] StartSetTaxTime` — đọc ở `0x0805B3B0`
  - `[CitySettings] SupplyLineBuildScale` — đọc ở `0x0805B3B0`
  - `[CitySettings] WarCycleValue` — đọc ở `0x0805B3B0`
  - `[InitCityMaster] (khoá ghép lúc chạy)` — đọc ở `0x0805B3B0`
  - `[InitCityMaster] InitWithTopTongs` — đọc ở `0x0805B3B0`

## `\settings\faction\门派设定.ini`
- bảng cục bộ `[ebp-0x130]`, nạp ở `0x08060C70`, **4 cột/khoá**
  - `[(section ghép lúc chạy)] Camp` — đọc ở `0x08060C70`
  - `[(section ghép lúc chạy)] Name` — đọc ở `0x08060C70`
  - `[(section ghép lúc chạy)] Series` — đọc ở `0x08060C70`
  - `[(section ghép lúc chạy)] ShowName` — đọc ở `0x08060C70`

## `\settings\forbititem.ini`
- bảng cục bộ `[ebp-0x78]`, nạp ở `0x08062EC0`, **16 cột/khoá**
  - `[(section ghép lúc chạy)] DetailType` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] DetailType2` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ForbitDrop` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ForbitMove` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ForbitSell` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ForbitThrowAway` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ForbitTrade` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] Genre` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] IsValuable` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] IsWrapable` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] Level` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ParticulType` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] ParticulType2` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] Quality` — đọc ở `0x08064D40`
  - `[(section ghép lúc chạy)] TypeName` — đọc ở `0x08064D40`
  - `[Info] Count` — đọc ở `0x08064D40`

## `\settings\gamesetting.ini`
- đối tượng `0x0830CF5C`, nạp ở `0x0805EA10`, **116 cột/khoá**
  - `[AutoHang] DigExpPercent` — đọc ở `0x08061650`
  - `[AutoHang] DigStateSkill` — đọc ở `0x08061650`
  - `[AutoHang] RunScriptVer` — đọc ở `0x08061650`
  - `[Coin] CoinParam1` — đọc ở `0x08061650`
  - `[Coin] CoinParam2` — đọc ở `0x08061650`
  - `[Coin] CoinParam3` — đọc ở `0x08061650`
  - `[Coin] CoinParam4` — đọc ở `0x08061650`
  - `[Coin] CoinParam5` — đọc ở `0x08061650`
  - `[Coin] CoinParam6` — đọc ở `0x08061650`
  - `[DiceGame] (khoá ghép lúc chạy)` — đọc ở `0x08061650`
  - `[DiceGame] MaxBet` — đọc ở `0x08061650`
  - `[DiceGame] MaxTotalFundBet` — đọc ở `0x08061650`
  - `[ENCHASER] CONDITIONISVALID` — đọc ở `0x08061650`
  - `[ENCHASER] FireStone` — đọc ở `0x08061650`
  - `[ENCHASER] IceSilk` — đọc ở `0x08061650`
  - `[ENCHASER] ISNOTWEAPON` — đọc ở `0x08061650`
  - `[ENCHASER] ITEMISINVALID` — đọc ở `0x08061650`
  - `[ENCHASER] LEVELISFULL` — đọc ở `0x08061650`
  - `[ENCHASER] LevelStone` — đọc ở `0x08061650`
  - `[ENCHASER] LevelUpRate1` — đọc ở `0x08061650`
  - `[ENCHASER] LevelUpRate2` — đọc ở `0x08061650`
  - `[ENCHASER] LevelUpRate3` — đọc ở `0x08061650`
  - `[ENCHASER] LevelUpRate4` — đọc ở `0x08061650`
  - `[ENCHASER] LevelUpRate5` — đọc ở `0x08061650`
  - `[ENCHASER] LevelUpRate6` — đọc ở `0x08061650`
  - `[ENCHASER] LevelUpRate7` — đọc ở `0x08061650`
  - `[ENCHASER] LevelUpRate8` — đọc ở `0x08061650`
  - `[ENCHASER] LevelUpRate9` — đọc ở `0x08061650`
  - `[ENCHASER] LuckyOneStone` — đọc ở `0x08061650`
  - `[ENCHASER] LuckyRandom` — đọc ở `0x08061650`
  - `[ENCHASER] LuckyThreeStone` — đọc ở `0x08061650`
  - `[ENCHASER] LuckyTwoStone` — đọc ở `0x08061650`
  - `[ENCHASER] MAGICATTRIBISFULL` — đọc ở `0x08061650`
  - `[ENCHASER] MagicAttribStone` — đọc ở `0x08061650`
  - `[ENCHASER] QUALITYINVALID` — đọc ở `0x08061650`
  - `[ENCHASER] SERIESISSAME` — đọc ở `0x08061650`
  - `[ENCHASER] SeriesStone` — đọc ở `0x08061650`
  - `[ENCHASER] SUCCESS` — đọc ở `0x08061650`
  - `[ENCHASER] UNCONDITION` — đọc ở `0x08061650`
  - `[ENCHASER] WRONGSTONES` — đọc ở `0x08061650`
  - `[ENCHASER] WRONGWEAPON` — đọc ở `0x08061650`
  - `[OFFLINE] KICK_OFFLINETIME` — đọc ở `0x08061650`
  - `[OFFLINE] TASK_LASTOFFLINEDAY` — đọc ở `0x08061650`
  - `[OFFLINE] TASK_LASTOFFLINETIME` — đọc ở `0x08061650`
  - `[RELIVEITEM] ITEMID` — đọc ở `0x08061650`
  - `[ServerConfig] ExpRate` — đọc ở `0x08061650`
  - `[ServerConfig] FreezeTimeReduceMax` — đọc ở `0x08061650`
  - `[ServerConfig] MaxFreeLevel` — đọc ở `0x08061650`
  - `[ServerConfig] MaxItemCount` — đọc ở `0x08061650`
  - `[ServerConfig] MaxMissleCount` — đọc ở `0x08061650`
  - `[ServerConfig] MaxNpcCount` — đọc ở `0x08061650`
  - `[ServerConfig] MaxObjCount` — đọc ở `0x08061650`
  - `[ServerConfig] MaxPlayerCount` — đọc ở `0x08061650`
  - `[ServerConfig] MaxSubWorldCount` — đọc ở `0x08061650`
  - `[ServerConfig] MoneyRate` — đọc ở `0x08061650`
  - `[ServerConfig] NpcPoisonDamageMax` — đọc ở `0x08061650`
  - `[ServerConfig] PlayerPoisonDamageMax` — đọc ở `0x08061650`
  - `[ServerConfig] RemoveForItemAbradeToZero` — đọc ở `0x08061650`
  - `[ServerConfig] VirtualLoadScript` — đọc ở `0x08061650`
  - `[SHOP] nCurrtype` — đọc ở `0x08061650`
  - `[STALLLIMITED] bSwitch` — đọc ở `0x08061650`
  - `[SYSTEM] AdventureItemID` — đọc ở `0x08061650`
  - `[SYSTEM] AdventureItemParam1` — đọc ở `0x08061650`
  - `[SYSTEM] AdventureItemParam2` — đọc ở `0x08061650`
  - `[SYSTEM] AdventureItemParam3` — đọc ở `0x08061650`
  - `[SYSTEM] AdventureItemParam4` — đọc ở `0x08061650`
  - `[SYSTEM] AdventureItemParam5` — đọc ở `0x08061650`
  - `[SYSTEM] AdventureItemParam6` — đọc ở `0x08061650`
  - `[SYSTEM] BillWorldChat` — đọc ở `0x08061650`
  - `[SYSTEM] ClothesItemDurInter` — đọc ở `0x08061650`
  - `[SYSTEM] ClothesItemProcessInter` — đọc ở `0x08061650`
  - `[SYSTEM] EnableEmperorRights` — đọc ở `0x08061650`
  - `[SYSTEM] GlobalNpcDeathScript` — đọc ở `0x08061650`
  - `[SYSTEM] GlobalTrade` — đọc ở `0x08061650`
  - `[SYSTEM] GoldCoinExtPoint1` — đọc ở `0x08061650`
  - `[SYSTEM] GoldCoinExtPoint2` — đọc ở `0x08061650`
  - `[SYSTEM] GoldCoinExtPointNew` — đọc ở `0x08061650`
  - `[SYSTEM] IsCheckNpcBarrier` — đọc ở `0x08061650`
  - `[SYSTEM] ItemAbradeType` — đọc ở `0x08061650`
  - `[SYSTEM] LuckyStarAura` — đọc ở `0x08061650`
  - `[SYSTEM] MinScaleValue` — đọc ở `0x08061650`
  - `[SYSTEM] SaveTimeOnShop` — đọc ở `0x08061650`
  - `[SYSTEM] SaveTimeOnStall` — đọc ở `0x08061650`
  - `[SYSTEM] SaveTimeOnTrade` — đọc ở `0x08061650`
  - `[SYSTEM] STUNID` — đọc ở `0x08061650`
  - `[TRANSLIFE] MinEarthResist` — đọc ở `0x08061650`
  - `[TRANSLIFE] MinFireResist` — đọc ở `0x08061650`
  - `[TRANSLIFE] MinPhyResist` — đọc ở `0x08061650`
  - `[TRANSLIFE] MinWaterResist` — đọc ở `0x08061650`
  - `[TRANSLIFE] MinWoodResist` — đọc ở `0x08061650`
  - `[TRANSLIFE] ReqLevelAppend1` — đọc ở `0x08061650`
  - `[TRANSLIFE] ReqLevelAppend2` — đọc ở `0x08061650`
  - `[TRANSLIFE] ReqLevelAppend3` — đọc ở `0x08061650`
  - `[TRANSLIFE] ReqLevelAppend4` — đọc ở `0x08061650`
  - `[TRANSLIFE] ReqLevelAppend5` — đọc ở `0x08061650`
  - `[WeaponChart] ChartOfBlade` — đọc ở `0x08061650`
  - `[WeaponChart] ChartOfCrossbow` — đọc ở `0x08061650`
  - `[WeaponChart] ChartOfDarts` — đọc ở `0x08061650`
  - `[WeaponChart] ChartOfDualBlades` — đọc ở `0x08061650`
  - `[WeaponChart] ChartOfHammer` — đọc ở `0x08061650`
  - `[WeaponChart] ChartOfKnife` — đọc ở `0x08061650`
  - `[WeaponChart] ChartOfSpear` — đọc ở `0x08061650`
  - `[WeaponChart] ChartOfSword` — đọc ở `0x08061650`
  - `[WeaponChart] ChartOfWand` — đọc ở `0x08061650`
  - `[WeaponType] TypeBase` — đọc ở `0x08061650`
  - `[WeaponType] TypeOfBlade` — đọc ở `0x08061650`
  - `[WeaponType] TypeOfCrossbow` — đọc ở `0x08061650`
  - `[WeaponType] TypeOfDarts` — đọc ở `0x08061650`
  - `[WeaponType] TypeOfDualBlades` — đọc ở `0x08061650`
  - `[WeaponType] TypeOfHammer` — đọc ở `0x08061650`
  - `[WeaponType] TypeOfKnife` — đọc ở `0x08061650`
  - `[WeaponType] TypeOfSpear` — đọc ở `0x08061650`
  - `[WeaponType] TypeOfSword` — đọc ở `0x08061650`
  - `[WeaponType] TypeOfWand` — đọc ở `0x08061650`
  - `[YUXIITEM] ITEMID` — đọc ở `0x08061650`
  - `[YUXIITEM] TASK` — đọc ở `0x08061650`

## `\settings\goods.txt`
- bảng cục bộ `[ebp-0xB8]`, nạp ở `0x081F3B40`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x081F3B40`

## `\settings\item\AbradeRate.ini`
- bảng cục bộ `[ebp-0xE8]`, nạp ở `0x0806E250`, **75 cột/khoá**
  - `[AdvPlatina_Attack] Amulet` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Belt` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Body` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Cuff` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Foot` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Head` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Horse` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Mask` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Pendant` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Ring1` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Ring2` — đọc ở `0x0806E250`
  - `[AdvPlatina_Attack] Weapon` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Amulet` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Belt` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Body` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Cuff` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Foot` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Head` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Horse` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Mask` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Pendant` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Ring1` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Ring2` — đọc ở `0x0806E250`
  - `[AdvPlatina_Defend] Weapon` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Amulet` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Belt` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Body` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Cuff` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Foot` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Head` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Horse` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Mask` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Pendant` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Ring1` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Ring2` — đọc ở `0x0806E250`
  - `[AdvPlatina_Move] Weapon` — đọc ở `0x0806E250`
  - `[Attack] Amulet` — đọc ở `0x0806E250`
  - `[Attack] Belt` — đọc ở `0x0806E250`
  - `[Attack] Body` — đọc ở `0x0806E250`
  - `[Attack] Cuff` — đọc ở `0x0806E250`
  - `[Attack] Foot` — đọc ở `0x0806E250`
  - `[Attack] Head` — đọc ở `0x0806E250`
  - `[Attack] Horse` — đọc ở `0x0806E250`
  - `[Attack] Mask` — đọc ở `0x0806E250`
  - `[Attack] Pendant` — đọc ở `0x0806E250`
  - `[Attack] Ring1` — đọc ở `0x0806E250`
  - `[Attack] Ring2` — đọc ở `0x0806E250`
  - `[Attack] Weapon` — đọc ở `0x0806E250`
  - `[Defend] Amulet` — đọc ở `0x0806E250`
  - `[Defend] Belt` — đọc ở `0x0806E250`
  - `[Defend] Body` — đọc ở `0x0806E250`
  - `[Defend] Cuff` — đọc ở `0x0806E250`
  - `[Defend] Foot` — đọc ở `0x0806E250`
  - `[Defend] Head` — đọc ở `0x0806E250`
  - `[Defend] Horse` — đọc ở `0x0806E250`
  - `[Defend] Mask` — đọc ở `0x0806E250`
  - `[Defend] Pendant` — đọc ở `0x0806E250`
  - `[Defend] Ring1` — đọc ở `0x0806E250`
  - `[Defend] Ring2` — đọc ở `0x0806E250`
  - `[Defend] Weapon` — đọc ở `0x0806E250`
  - `[Move] Amulet` — đọc ở `0x0806E250`
  - `[Move] Belt` — đọc ở `0x0806E250`
  - `[Move] Body` — đọc ở `0x0806E250`
  - `[Move] Cuff` — đọc ở `0x0806E250`
  - `[Move] Foot` — đọc ở `0x0806E250`
  - `[Move] Head` — đọc ở `0x0806E250`
  - `[Move] Horse` — đọc ở `0x0806E250`
  - `[Move] Mask` — đọc ở `0x0806E250`
  - `[Move] Pendant` — đọc ở `0x0806E250`
  - `[Move] Ring1` — đọc ở `0x0806E250`
  - `[Move] Ring2` — đọc ở `0x0806E250`
  - `[Move] Weapon` — đọc ở `0x0806E250`
  - `[Repair] ItemPriceScale` — đọc ở `0x0806E250`
  - `[Repair] MagicPriceScale` — đọc ở `0x0806E250`
  - `[Repair] WarningBaseline` — đọc ở `0x0806E250`

## `\settings\item\ArmorRes.txt`
- đối tượng `0x0830D360`, nạp ở `0x08068D90`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08068980`

## `\settings\item\autohangdroprate.ini`
- bảng cục bộ `[ebp-0x9C]`, nạp ở `0x080C46A0`, **27 cột/khoá**
  - `[(section ghép lúc chạy)] (khoá ghép lúc chạy)` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Detail` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] EnchasableRate` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Genre` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MaxItemLevel` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MaxSocket` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MinItemLevel` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MinSocket` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Particular` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Quality` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] RandRate` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Series` — đọc ở `0x080A3B80`
  - `[Main] Count` — đọc ở `0x080A3B80`
  - `[Main] EnchasableRate` — đọc ở `0x080A3B80`
  - `[Main] IsTeamShare` — đọc ở `0x080A3B80`
  - `[Main] MagicRate` — đọc ở `0x080A3B80`
  - `[Main] MaxItemLevel` — đọc ở `0x080A3B80`
  - `[Main] MaxItemLevelScale` — đọc ở `0x080A3B80`
  - `[Main] MaxSocket` — đọc ở `0x080A3B80`
  - `[Main] MinItemLevel` — đọc ở `0x080A3B80`
  - `[Main] MinItemLevelScale` — đọc ở `0x080A3B80`
  - `[Main] MinSocket` — đọc ở `0x080A3B80`
  - `[Main] MoneyRate` — đọc ở `0x080A3B80`
  - `[Main] MoneyScale` — đọc ở `0x080A3B80`
  - `[Main] RandRange` — đọc ở `0x080A3B80`
  - `[Main] Series` — đọc ở `0x080A3B80`
  - `[Main] TeamShareRate` — đọc ở `0x080A3B80`

## `\settings\item\ClothesEquipRes.txt`
- bảng cục bộ `[ebp-0x4C]`, nạp ở `0x08068D00`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08068B70`

## `\settings\item\foundryresdemand.ini`
- bảng cục bộ `[ebp-0xB4]`, nạp ở `0x0814F050`, **6 cột/khoá**
  - `[(section ghép lúc chạy)] (khoá ghép lúc chạy)` — đọc ở `0x0814F050`
  - `[(section ghép lúc chạy)] DetailType` — đọc ở `0x0814EEC0`
  - `[(section ghép lúc chạy)] Genre` — đọc ở `0x0814EEC0`
  - `[(section ghép lúc chạy)] PtcType` — đọc ở `0x0814EEC0`
  - `[(section ghép lúc chạy)] Quality` — đọc ở `0x0814EEC0`
  - `[(section ghép lúc chạy)] Stackable` — đọc ở `0x0814EEC0`

## `\settings\item\GoldEquipRes.txt`
- bảng cục bộ `[ebp-0x4C]`, nạp ở `0x08068D60`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08068B70`

## `\settings\item\HelmRes.txt`
- đối tượng `0x0830D380`, nạp ở `0x08068D90`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08068900`

## `\settings\item\HorseRes.txt`
- đối tượng `0x0830D3A0`, nạp ở `0x08068D90`, **0 cột/khoá**

## `\settings\item\ItemTradeLogFilter.ini`
- bảng cục bộ `[ebp-0x90]`, nạp ở `0x0806E250`, **2 cột/khoá**
  - `[DetailType] (khoá ghép lúc chạy)` — đọc ở `0x0806E250`
  - `[DetailType] FilterNum` — đọc ở `0x0806E250`

## `\settings\item\MeleeRes.txt`
- đối tượng `0x0830D320`, nạp ở `0x08068D90`, **0 cột/khoá**

## `\settings\item\platina_durability.txt`
- bảng cục bộ `[ebp-0x58]`, nạp ở `0x081CACD0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x081CACD0`

## `\settings\item\platina_magicattr.txt`
- bảng cục bộ `[ebp-0x48]`, nạp ở `0x080E6C80`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080E6C80`

## `\settings\item\platina_magicrate.txt`
- bảng cục bộ `[ebp-0x68]`, nạp ở `0x080E68D0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080E68D0`

## `\settings\item\PlatinaEquipRes.txt`
- bảng cục bộ `[ebp-0x4C]`, nạp ở `0x08068D30`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08068B70`

## `\settings\item\questkey.txt`
- đối tượng `0x09780D20`, nạp ở `0x0805D580`, **1 cột/khoá**
  - `DetailType` — đọc ở `0x0811CB00`, `0x0811D250`, `0x0811D3B0`, `0x0811D5D0`

## `\settings\item\RangeRes.txt`
- đối tượng `0x0830D340`, nạp ở `0x08068D90`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08068A00`

## `\settings\item\zijingaodroprate.ini`
- bảng cục bộ `[ebp-0x9C]`, nạp ở `0x080C46A0`, **27 cột/khoá**
  - `[(section ghép lúc chạy)] (khoá ghép lúc chạy)` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Detail` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] EnchasableRate` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Genre` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MaxItemLevel` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MaxSocket` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MinItemLevel` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] MinSocket` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Particular` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Quality` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] RandRate` — đọc ở `0x080A3B80`
  - `[(section ghép lúc chạy)] Series` — đọc ở `0x080A3B80`
  - `[Main] Count` — đọc ở `0x080A3B80`
  - `[Main] EnchasableRate` — đọc ở `0x080A3B80`
  - `[Main] IsTeamShare` — đọc ở `0x080A3B80`
  - `[Main] MagicRate` — đọc ở `0x080A3B80`
  - `[Main] MaxItemLevel` — đọc ở `0x080A3B80`
  - `[Main] MaxItemLevelScale` — đọc ở `0x080A3B80`
  - `[Main] MaxSocket` — đọc ở `0x080A3B80`
  - `[Main] MinItemLevel` — đọc ở `0x080A3B80`
  - `[Main] MinItemLevelScale` — đọc ở `0x080A3B80`
  - `[Main] MinSocket` — đọc ở `0x080A3B80`
  - `[Main] MoneyRate` — đọc ở `0x080A3B80`
  - `[Main] MoneyScale` — đọc ở `0x080A3B80`
  - `[Main] RandRange` — đọc ở `0x080A3B80`
  - `[Main] Series` — đọc ở `0x080A3B80`
  - `[Main] TeamShareRate` — đọc ở `0x080A3B80`

## `\settings\item_log_property.txt`
- bảng cục bộ `[ebp-0x38]`, nạp ở `0x0805EA10`, **5 cột/khoá**
  - `Detail` — đọc ở `0x081E8750`
  - `Genre` — đọc ở `0x081E8750`
  - `Level` — đọc ở `0x081E8750`
  - `Particular` — đọc ở `0x081E8750`
  - `Quality` — đọc ở `0x081E8750`

## `\settings\killer.ini`
- bảng cục bộ `[ebp-0x70]`, nạp ở `0x0805EA10`, **6 cột/khoá**
  - `[Main] MaxActiveTaskTime` — đọc ở `0x0806F890`
  - `[Main] MinReward` — đọc ở `0x0806F890`
  - `[Main] MinTargetLevel` — đọc ở `0x0806F890`
  - `[Main] MoneyPerHour` — đọc ở `0x0806F890`
  - `[Messages] TargetAbsent` — đọc ở `0x0806F890`
  - `[Messages] TargetLevelTooLow` — đọc ở `0x0806F890`

## `\settings\lastupdate.ini`
- đối tượng `0x09781580`, nạp ở `0x0805EA10`, **3 cột/khoá**
  - `[(section ghép lúc chạy)] (khoá ghép lúc chạy)` — đọc ở `0x08141E40`
  - `[(section ghép lúc chạy)] Count` — đọc ở `0x08141E40`
  - `[(section ghép lúc chạy)] Title` — đọc ở `0x08142020`

## `\settings\logset.ini`
- bảng cục bộ `[ebp-0x80]`, nạp ở `0x08062EC0`, **19 cột/khoá**
  - `[LogSet] (khoá ghép lúc chạy)` — đọc ở `0x08061130`
  - `[LogSet] DeathLostExp` — đọc ở `0x08061130`
  - `[LogSet] DeathLostItem` — đọc ở `0x08061130`
  - `[LogSet] DeathLostMoney` — đọc ở `0x08061130`
  - `[LogSet] ItemOptionCount` — đọc ở `0x08061130`
  - `[LogSet] NpcStallMoney` — đọc ở `0x08061130`
  - `[LogSet] NpcStallType` — đọc ở `0x08061130`
  - `[LogSet] NpcTradeBuyMoney` — đọc ở `0x08061130`
  - `[LogSet] NpcTradeBuyType` — đọc ở `0x08061130`
  - `[LogSet] NpcTradeSellMoney` — đọc ở `0x08061130`
  - `[LogSet] NpcTradeSellType` — đọc ở `0x08061130`
  - `[LogSet] PickUpItemType` — đọc ở `0x08061130`
  - `[LogSet] PickUpMoneyNum` — đọc ở `0x08061130`
  - `[LogSet] PickUpMoneyType` — đọc ở `0x08061130`
  - `[LogSet] PlayerTradeItem` — đọc ở `0x08061130`
  - `[LogSet] PlayerTradeMoneyNum` — đọc ở `0x08061130`
  - `[LogSet] PlayerTradeMoneyType` — đọc ở `0x08061130`
  - `[LogSet] ThrowAwayAllMedicine` — đọc ở `0x08061130`
  - `[LogSet] ThrowAwaySingleItem` — đọc ở `0x08061130`

## `\settings\lottery.txt`
- đối tượng `0x0830ADE0`, nạp ở `0x0805DEC0`, **0 cột/khoá**

## `\settings\magicdesc.ini`
- đối tượng `0x0830EBC0`, nạp ở `0x08072480`, **1 cột/khoá**
  - `[Descript] (khoá ghép lúc chạy)` — đọc ở `0x08073380`

## `\settings\maplist.ini`
- đối tượng `0x0977FEC0`, nạp ở `0x080F71D0`, **0 cột/khoá**
- đối tượng `0x09777F38`, nạp ở `0x080F7210`, `0x080F7680`, **2 cột/khoá**
  - `[List] (khoá ghép lúc chạy)` — đọc ở `0x080F72C0`, `0x080F7420`, `0x080F7680`, `0x080F7CC0`
  - `[List] ExportNpcData` — đọc ở `0x080F7680`, `0x080F7CC0`

## `\settings\Missles.txt`
- đối tượng `0x0830AE20`, nạp ở `0x0805EA10`, `0x080E7200`, **1 cột/khoá**
  - `MissleId` — đọc ở `0x0805D210`

## `\settings\npc\NpcGoldTemplate.txt`
- bảng cục bộ `[ebp-0x38]`, nạp ở `0x0809CCC0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x0809CCC0`

## `\settings\npc\PKRate.ini`
- bảng cục bộ `[ebp-0x90]`, nạp ở `0x080A0810`, **11 cột/khoá**
  - `[?] BeKilled` — đọc ở `0x080A0810`
  - `[?] ButcherPKExercise` — đọc ở `0x080A0810`
  - `[?] EnmityPK` — đọc ở `0x080A0810`
  - `[?] FactionPKFaction` — đọc ở `0x080A0810`
  - `[?] KillerPKFaction` — đọc ở `0x080A0810`
  - `[?] KillPartnerPK` — đọc ở `0x080A0810`
  - `[?] LevelDistance` — đọc ở `0x080A0810`
  - `[?] NotEnmityExpPercent` — đọc ở `0x080A0810`
  - `[?] NotFightExpPercent` — đọc ở `0x080A0810`
  - `[?] NotSubPKExpPercent` — đọc ở `0x080A0810`
  - `[?] rate` — đọc ở `0x080A0810`

## `\settings\npc\player\BaseValue.ini`
- bảng cục bộ `[ebp-0x60]`, nạp ở `0x080A0810`, **5 cột/khoá**
  - `[Common] AttackFrame` — đọc ở `0x080A00D0`
  - `[Common] CastFrame` — đọc ở `0x080A00D0`
  - `[Common] HurtFrame` — đọc ở `0x080A00D0`
  - `[Common] RunSpeed` — đọc ở `0x080A00D0`
  - `[Common] WalkSpeed` — đọc ở `0x080A00D0`

## `\settings\npc\player\chatcost.ini`
- bảng cục bộ `[ebp-0x140]`, nạp ở `0x080A0810`, **4 cột/khoá**
  - `[(section ghép lúc chạy)] Level` — đọc ở `0x080A0810`
  - `[(section ghép lúc chạy)] ManaPercent` — đọc ở `0x080A0810`
  - `[(section ghép lúc chạy)] Money` — đọc ở `0x080A0810`
  - `[(section ghép lúc chạy)] StaminaPercent` — đọc ở `0x080A0810`

## `\settings\npc\player\event_killnpc.txt`
- bảng cục bộ `[ebp-0x40]`, nạp ở `0x08156760`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08156760`

## `\settings\npc\player\level_add.txt`
- bảng cục bộ `[ebp-0x5C]`, nạp ở `0x080C4FD0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080C4FD0`

## `\settings\npc\player\level_exp.txt`
- bảng cục bộ `[ebp-0x3C]`, nạp ở `0x080C4FD0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080C4FD0`

## `\settings\npc\player\level_lead_exp.txt`
- bảng cục bộ `[ebp-0x38]`, nạp ở `0x080C4F10`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080C4F10`

## `\settings\npc\player\NewPlayerBaseAttribute.ini`
- bảng cục bộ `[ebp-0x70]`, nạp ở `0x080C4A80`, **5 cột/khoá**
  - `[?] Dexterity` — đọc ở `0x080C4A80`
  - `[?] Engergy` — đọc ở `0x080C4A80`
  - `[?] Lucky` — đọc ở `0x080C4A80`
  - `[?] Strength` — đọc ở `0x080C4A80`
  - `[?] Vitality` — đọc ở `0x080C4A80`

## `\settings\npc\player\PKPunish.txt`
- bảng cục bộ `[ebp-0x3C]`, nạp ở `0x080C5980`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080C5980`

## `\settings\npc\player\stamina.ini`
- bảng cục bộ `[ebp-0xE8]`, nạp ở `0x080A0810`, **5 cột/khoá**
  - `[stamina] ExerciseRunSub` — đọc ở `0x080A0810`
  - `[stamina] FightRunSub` — đọc ở `0x080A0810`
  - `[stamina] KillRunSub` — đọc ở `0x080A0810`
  - `[stamina] NormalAdd` — đọc ở `0x080A0810`
  - `[stamina] SitAdd` — đọc ở `0x080A0810`

## `\settings\npc\player\team_resist_effect.ini`
- bảng cục bộ `[ebp-0xB0]`, nạp ở `0x080CC410`, **1 cột/khoá**
  - `[(section ghép lúc chạy)] (khoá ghép lúc chạy)` — đọc ở `0x080CC410`

## `\settings\npcres\人物类型.txt`
- đối tượng `0x0830AEE0`, nạp ở `0x0805D2B0`, **0 cột/khoá**

## `\settings\npcres\界面状态与图形对照表.txt`
- bảng cục bộ `[ebp-0x38]`, nạp ở `0x080C27B0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080C27B0`

## `\settings\NpcS.txt`
- đối tượng `0x0830AEC0`, nạp ở `0x0805EA10`, **94 cột/khoá**
  - `ActiveRadius` — đọc ở `0x080A4410`
  - `AIMaxTime` — đọc ở `0x080A4410`
  - `AIMode` — đọc ở `0x080A2160`
  - `AIParam1` — đọc ở `0x080A2160`
  - `AIParam10` — đọc ở `0x080A2160`
  - `AIParam2` — đọc ở `0x080A2160`
  - `AIParam3` — đọc ở `0x080A2160`
  - `AIParam4` — đọc ở `0x080A2160`
  - `AIParam5` — đọc ở `0x080A2160`
  - `AIParam6` — đọc ở `0x080A2160`
  - `AIParam7` — đọc ở `0x080A2160`
  - `AIParam8` — đọc ở `0x080A2160`
  - `AIParam9` — đọc ở `0x080A2160`
  - `ARParam` — đọc ở `0x080A2160`
  - `ARParam1` — đọc ở `0x080A2160`
  - `ARParam2` — đọc ở `0x080A2160`
  - `ARParam3` — đọc ở `0x080A2160`
  - `AttackSpeed` — đọc ở `0x080A4410`
  - `AuraSkillId` — đọc ở `0x080A2160`
  - `AuraSkillLevel` — đọc ở `0x080A2160`
  - `Camp` — đọc ở `0x080A4410`
  - `CastSpeed` — đọc ở `0x080A4410`
  - `ClientOnly` — đọc ở `0x080A4410`
  - `ColdDamageBase` — đọc ở `0x080A2160`
  - `ColdMagicBase` — đọc ở `0x080A2160`
  - `ColdResist` — đọc ở `0x080A2160`
  - `ColdResistMax` — đọc ở `0x080A4410`
  - `CorpseIdx` — đọc ở `0x080A4410`
  - `DeathFrame` — đọc ở `0x080A4410`
  - `DefenseParam` — đọc ở `0x080A2160`
  - `DefenseParam1` — đọc ở `0x080A2160`
  - `DefenseParam2` — đọc ở `0x080A2160`
  - `DefenseParam3` — đọc ở `0x080A2160`
  - `DropRateFile` — đọc ở `0x080A4410`
  - `ExpParam` — đọc ở `0x080A2160`
  - `ExpParam1` — đọc ở `0x080A2160`
  - `ExpParam2` — đọc ở `0x080A2160`
  - `ExpParam3` — đọc ở `0x080A2160`
  - `FireDamageBase` — đọc ở `0x080A2160`
  - `FireMagicBase` — đọc ở `0x080A2160`
  - `FireResist` — đọc ở `0x080A2160`
  - `FireResistMax` — đọc ở `0x080A4410`
  - `HeadImage` — đọc ở `0x080A4410`
  - `HitRecover` — đọc ở `0x080A4410`
  - `HurtFrame` — đọc ở `0x080A4410`
  - `Kind` — đọc ở `0x080A4410`
  - `Level1` — đọc ở `0x080A2160`
  - `Level2` — đọc ở `0x080A2160`
  - `Level3` — đọc ở `0x080A2160`
  - `Level4` — đọc ở `0x080A2160`
  - `LevelScript` — đọc ở `0x080A4410`
  - `LifeParam` — đọc ở `0x080A2160`
  - `LifeParam1` — đọc ở `0x080A2160`
  - `LifeParam2` — đọc ở `0x080A2160`
  - `LifeParam3` — đọc ở `0x080A2160`
  - `LifeReplenish` — đọc ở `0x080A2160`
  - `LightingDamageBase` — đọc ở `0x080A2160`
  - `LightingMagicBase` — đọc ở `0x080A2160`
  - `LightResist` — đọc ở `0x080A2160`
  - `LightResistMax` — đọc ở `0x080A4410`
  - `MaxDamageParam` — đọc ở `0x080A2160`
  - `MaxDamageParam1` — đọc ở `0x080A2160`
  - `MaxDamageParam2` — đọc ở `0x080A2160`
  - `MaxDamageParam3` — đọc ở `0x080A2160`
  - `MinDamageParam` — đọc ở `0x080A2160`
  - `MinDamageParam1` — đọc ở `0x080A2160`
  - `MinDamageParam2` — đọc ở `0x080A2160`
  - `MinDamageParam3` — đọc ở `0x080A2160`
  - `Name` — đọc ở `0x080A4410`
  - `PasstSkillId` — đọc ở `0x080A2160`
  - `PasstSkillLevel` — đọc ở `0x080A2160`
  - `PhysicalDamageBase` — đọc ở `0x080A2160`
  - `PhysicalMagicBase` — đọc ở `0x080A2160`
  - `PhysicsResist` — đọc ở `0x080A2160`
  - `PhysicsResistMax` — đọc ở `0x080A4410`
  - `PoisonDamageBase` — đọc ở `0x080A2160`
  - `PoisonMagicBase` — đọc ở `0x080A2160`
  - `PoisonResist` — đọc ở `0x080A2160`
  - `PoisonResistMax` — đọc ở `0x080A4410`
  - `ReviveFrame` — đọc ở `0x080A4410`
  - `RunFrame` — đọc ở `0x080A4410`
  - `RunSpeed` — đọc ở `0x080A4410`
  - `Series` — đọc ở `0x080A4410`
  - `Skill1` — đọc ở `0x080A2160`
  - `Skill2` — đọc ở `0x080A2160`
  - `Skill3` — đọc ở `0x080A2160`
  - `Skill4` — đọc ở `0x080A2160`
  - `StandFrame` — đọc ở `0x080A4410`
  - `StandFrame1` — đọc ở `0x080A4410`
  - `Stature` — đọc ở `0x080A4410`
  - `Treasure` — đọc ở `0x080A4410`
  - `VisionRadius` — đọc ở `0x080A4410`
  - `WalkFrame` — đọc ở `0x080A4410`
  - `WalkSpeed` — đọc ở `0x080A4410`

## `\settings\obj\MoneyObj.txt`
- đối tượng `0x08BAEBC0`, nạp ở `0x080A78E0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080A64A0`

## `\settings\obj\ObjData.txt`
- đối tượng `0x08BAEBA0`, nạp ở `0x080A78E0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x080A6220`, `0x080A6560`

## `\settings\partner\aptitude_mode.txt`
- bảng cục bộ `[ebp-0x3C]`, nạp ở `0x08169B20`, **1 cột/khoá**
  - `APTITUDE_MODE_ID` — đọc ở `0x08169B20`

## `\settings\partner\aptitude_range.txt`
- bảng cục bộ `[ebp-0x6C]`, nạp ở `0x081689D0`, **13 cột/khoá**
  - `APTITUDE` — đọc ở `0x081689D0`
  - `DEFENCE_ATTRIB_MAX` — đọc ở `0x081689D0`
  - `DEFENCE_ATTRIB_MIN` — đọc ở `0x081689D0`
  - `HITTARGETRATE_ATTRIB_MAX` — đọc ở `0x081689D0`
  - `HITTARGETRATE_ATTRIB_MIN` — đọc ở `0x081689D0`
  - `LIFE_ATTRIB_MAX` — đọc ở `0x081689D0`
  - `LIFE_ATTRIB_MIN` — đọc ở `0x081689D0`
  - `LUCK_ATTRIB_MAX` — đọc ở `0x081689D0`
  - `LUCK_ATTRIB_MIN` — đọc ở `0x081689D0`
  - `SPEED_ATTRIB_MAX` — đọc ở `0x081689D0`
  - `SPEED_ATTRIB_MIN` — đọc ở `0x081689D0`
  - `STRENGTH_ATTRIB_MAX` — đọc ở `0x081689D0`
  - `STRENGTH_ATTRIB_MIN` — đọc ở `0x081689D0`

## `\settings\partner\attrib_range.txt`
- bảng cục bộ `[ebp-0x50]`, nạp ở `0x08169030`, **25 cột/khoá**
  - `DEFENCE_ATTRIB_INCREMENT_MAX` — đọc ở `0x08169030`
  - `DEFENCE_ATTRIB_INCREMENT_MIN` — đọc ở `0x08169030`
  - `DEFENCE_ATTRIB_INIT_MAX` — đọc ở `0x08169030`
  - `DEFENCE_ATTRIB_INIT_MIN` — đọc ở `0x08169030`
  - `HITTARGETRATE_ATTRIB_INCREMENT_MAX` — đọc ở `0x08169030`
  - `HITTARGETRATE_ATTRIB_INCREMENT_MIN` — đọc ở `0x08169030`
  - `HITTARGETRATE_ATTRIB_INIT_MAX` — đọc ở `0x08169030`
  - `HITTARGETRATE_ATTRIB_INIT_MIN` — đọc ở `0x08169030`
  - `LIFE_ATTRIB_INCREMENT_MAX` — đọc ở `0x08169030`
  - `LIFE_ATTRIB_INCREMENT_MIN` — đọc ở `0x08169030`
  - `LIFE_ATTRIB_INIT_MAX` — đọc ở `0x08169030`
  - `LIFE_ATTRIB_INIT_MIN` — đọc ở `0x08169030`
  - `LUCK_ATTRIB_INCREMENT_MAX` — đọc ở `0x08169030`
  - `LUCK_ATTRIB_INCREMENT_MIN` — đọc ở `0x08169030`
  - `LUCK_ATTRIB_INIT_MAX` — đọc ở `0x08169030`
  - `LUCK_ATTRIB_INIT_MIN` — đọc ở `0x08169030`
  - `SERIES` — đọc ở `0x08169030`
  - `SPEED_ATTRIB_INCREMENT_MAX` — đọc ở `0x08169030`
  - `SPEED_ATTRIB_INCREMENT_MIN` — đọc ở `0x08169030`
  - `SPEED_ATTRIB_INIT_MAX` — đọc ở `0x08169030`
  - `SPEED_ATTRIB_INIT_MIN` — đọc ở `0x08169030`
  - `STRENGTH_ATTRIB_INCREMENT_MAX` — đọc ở `0x08169030`
  - `STRENGTH_ATTRIB_INCREMENT_MIN` — đọc ở `0x08169030`
  - `STRENGTH_ATTRIB_INIT_MAX` — đọc ở `0x08169030`
  - `STRENGTH_ATTRIB_INIT_MIN` — đọc ở `0x08169030`

## `\settings\partner\character.txt`
- bảng cục bộ `[ebp-0x3C]`, nạp ở `0x08169FB0`, **16 cột/khoá**
  - `ActiveRadius` — đọc ở `0x08169FB0`
  - `AIMaxTime` — đọc ở `0x08169FB0`
  - `AIMode` — đọc ở `0x08169FB0`
  - `AIParam1` — đọc ở `0x08169FB0`
  - `AIParam2` — đọc ở `0x08169FB0`
  - `AIParam3` — đọc ở `0x08169FB0`
  - `AIParam4` — đọc ở `0x08169FB0`
  - `AIParam5` — đọc ở `0x08169FB0`
  - `AIParam6` — đọc ở `0x08169FB0`
  - `AIParam7` — đọc ở `0x08169FB0`
  - `AIParam8` — đọc ở `0x08169FB0`
  - `AIParam9` — đọc ở `0x08169FB0`
  - `Characteristic` — đọc ở `0x08169FB0`
  - `ForceSync` — đọc ở `0x08169FB0`
  - `NPCName` — đọc ở `0x08169FB0`
  - `VisionRadius` — đọc ở `0x08169FB0`

## `\settings\partner\feature.txt`
- bảng cục bộ `[ebp-0x54]`, nạp ở `0x08169850`, **5 cột/khoá**
  - `ESSENTIAL_FEATURE_ID` — đọc ở `0x08169850`
  - `NPC_INDEX_PERIOD_1` — đọc ở `0x08169850`
  - `NPC_INDEX_PERIOD_2` — đọc ở `0x08169850`
  - `NPC_INDEX_PERIOD_3` — đọc ở `0x08169850`
  - `SEX` — đọc ở `0x08169850`

## `\settings\partner\init_skill.ini`
- bảng cục bộ `[ebp-0xB8]`, nạp ở `0x08168020`, **5 cột/khoá**
  - `[(section ghép lúc chạy)] COUNT` — đọc ở `0x08168020`
  - `[(section ghép lúc chạy)] SKILL_EXP` — đọc ở `0x08168020`
  - `[(section ghép lúc chạy)] SKILL_ID` — đọc ở `0x08168020`
  - `[(section ghép lúc chạy)] SKILL_LEVEL` — đọc ở `0x08168020`
  - `[(section ghép lúc chạy)] SKILL_TYPE` — đọc ở `0x08168020`

## `\settings\partner\level_exp.txt`
- bảng cục bộ `[ebp-0x3C]`, nạp ở `0x081684A0`, **2 cột/khoá**
  - `EXP` — đọc ở `0x081684A0`
  - `LEVEL` — đọc ở `0x081684A0`

## `\settings\partner\partner_bag.ini`
- bảng cục bộ `[ebp-0xA4]`, nạp ở `0x08168280`, **2 cột/khoá**
  - `[LEVEL_GRID] (khoá ghép lúc chạy)` — đọc ở `0x08168280`
  - `[MAIN] UnActiveColor` — đọc ở `0x08168280`

## `\settings\partner\partner_event.ini`
- bảng cục bộ `[ebp-0x90]`, nạp ở `0x0816A680`, **7 cột/khoá**
  - `[?] (khoá ghép lúc chạy)` — đọc ở `0x0816A680`
  - `[?] Odds` — đọc ở `0x0816A680`
  - `[?] Param1` — đọc ở `0x0816A680`
  - `[?] Param2` — đọc ở `0x0816A680`
  - `[MAIN] Format` — đọc ở `0x0816A680`
  - `[MAIN] HostName_Man` — đọc ở `0x0816A680`
  - `[MAIN] HostName_Woman` — đọc ở `0x0816A680`

## `\settings\partner\partner_setting.ini`
- bảng cục bộ `[ebp-0x64]`, nạp ở `0x08167E10`, **8 cột/khoá**
  - `[CALLOUT] CALLOUT_INTERVAL` — đọc ở `0x08167E10`
  - `[DEATH_PUNISH] PUNISH_TIME` — đọc ở `0x08167E10`
  - `[EMOTION_DEGREE] EMOTION_DEG_BASE` — đọc ở `0x08167E10`
  - `[EMOTION_DEGREE] EMOTION_DEG_STEP` — đọc ở `0x08167E10`
  - `[EMOTION_DEGREE] EMOTION_INITIALIZE` — đọc ở `0x08167E10`
  - `[EMOTION_DEGREE] EMOTION_MAX` — đọc ở `0x08167E10`
  - `[EMOTION_DEGREE] EMOTION_MIN` — đọc ở `0x08167E10`
  - `[REVIVE] LIFE_RESTORE` — đọc ở `0x08167E10`

## `\settings\partner\resist.txt`
- bảng cục bộ `[ebp-0x50]`, nạp ở `0x08168630`, **11 cột/khoá**
  - `COLD_RESIST_INCREMENT` — đọc ở `0x08168630`
  - `COLD_RESIST_INIT` — đọc ở `0x08168630`
  - `FIRE_RESIST_INCREMENT` — đọc ở `0x08168630`
  - `FIRE_RESIST_INIT` — đọc ở `0x08168630`
  - `LIGHTING_RESIST_INCREMENT` — đọc ở `0x08168630`
  - `LIGHTING_RESIST_INIT` — đọc ở `0x08168630`
  - `PHYSIC_RESIST_INCREMENT` — đọc ở `0x08168630`
  - `PHYSIC_RESIST_INIT` — đọc ở `0x08168630`
  - `POISON_RESIST_INCREMENT` — đọc ở `0x08168630`
  - `POISON_RESIST_INIT` — đọc ở `0x08168630`
  - `SERIES` — đọc ở `0x08168630`

## `\settings\permitdialognpc_info.txt`
- đối tượng `0x0830CFB4`, nạp ở `0x08062EC0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08062B50`

## `\settings\petsys\pet_skill_def.txt`
- bảng cục bộ `[ebp-0x50]`, nạp ở `0x081D47D0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x081D47D0`

## `\settings\player_limittime.ini`
- bảng cục bộ `[ebp-0x60]`, nạp ở `0x08176340`, **15 cột/khoá**
  - `[Config] bCheckExt` — đọc ở `0x08176340`
  - `[Config] bForbitStallWhenTire` — đọc ở `0x08176340`
  - `[Config] bForbitTradeWhenTire` — đọc ở `0x08176340`
  - `[Config] CloseLimit` — đọc ở `0x08176340`
  - `[Config] LimitForRole` — đọc ở `0x08176340`
  - `[Config] ResetWhenNewDay` — đọc ở `0x08176340`
  - `[Config] TireExPointBit` — đọc ở `0x08176340`
  - `[Config] TireExPointNum` — đọc ở `0x08176340`
  - `[Config] TireKickPlayer` — đọc ở `0x08176340`
  - `[Config] TireMsgInterv1` — đọc ở `0x08176340`
  - `[Config] TireMsgInterv2` — đọc ở `0x08176340`
  - `[Config] UpdateInterval` — đọc ở `0x08176340`
  - `[LimitTime] OfflineResumeTime` — đọc ở `0x08176340`
  - `[LimitTime] OnlineIllHealthTime` — đọc ở `0x08176340`
  - `[LimitTime] OnlineTiredTime` — đọc ở `0x08176340`

## `\settings\playertitle.txt`
- bảng cục bộ `[ebp-0x40]`, nạp ở `0x08157D80`, **7 cột/khoá**
  - `AuraSkill` — đọc ở `0x08157D80`
  - `AuraSkillLevel` — đọc ở `0x08157D80`
  - `FaceId` — đọc ở `0x08157D80`
  - `Memo` — đọc ở `0x08157D80`
  - `SpeicalGraphic` — đọc ở `0x08157D80`
  - `TitleId` — đọc ở `0x08157D80`
  - `TitleName` — đọc ở `0x08157D80`

## `\settings\product_config.ini`
- bảng cục bộ `[ebp-0x2D8]`, nạp ở `0x08051230`, **2 cột/khoá**
  - `[VersionCfg] ProductLanguage` — đọc ở `0x08051230`
  - `[VersionCfg] ProductRegion` — đọc ở `0x08051230`

## `\settings\revivepos.ini`
- đối tượng `0x09777F98`, nạp ở `0x080F7680`, `0x080F7CC0`, **1 cột/khoá**
  - `[(section ghép lúc chạy)] (khoá ghép lúc chạy)` — đọc ở `0x080F6D20`

## `\settings\shop\type.txt`
- đối tượng `0x097AC940`, nạp ở `0x081F4920`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x081F2700`, `0x081F49E0`

## `\settings\Skills.txt`
- đối tượng `0x0830AE00`, nạp ở `0x0805EA10`, `0x080E7200`, **60 cột/khoá**
  - `AttackRadius` — đọc ở `0x080E9200`
  - `BaseSkill` — đọc ở `0x080E9200`
  - `ByMissle` — đọc ở `0x080E9200`
  - `CharAnimId` — đọc ở `0x080E9200`
  - `CharClass` — đọc ở `0x080E9200`
  - `ChildSkillId` — đọc ở `0x080E9200`
  - `ChildSkillLevel` — đọc ở `0x080E9200`
  - `ChildSkillNum` — đọc ở `0x080E9200`
  - `ClientSend` — đọc ở `0x080E9200`
  - `CollideEvent` — đọc ở `0x080E9200`
  - `CollidSkillId` — đọc ở `0x080E9200`
  - `CostValue` — đọc ở `0x080E9200`
  - `DoHurt` — đọc ở `0x080E9200`
  - `EqtLimit` — đọc ở `0x080E9200`
  - `EventSkillLevel` — đọc ở `0x080E9200`
  - `FlyEvent` — đọc ở `0x080E9200`
  - `FlyEventTime` — đọc ở `0x080E9200`
  - `FlySkillId` — đọc ở `0x080E9200`
  - `HeelAtParent` — đọc ở `0x080E9200`
  - `HorseLimit` — đọc ở `0x080E9200`
  - `IsAura` — đọc ở `0x080E9200`
  - `IsExpSkill` — đọc ở `0x080E9200`
  - `IsMelee` — đọc ở `0x080E9200`
  - `IsPhysical` — đọc ở `0x080E9200`
  - `IsUseAR` — đọc ở `0x080E9200`
  - `LevelUpScript` — đọc ở `0x080E9200`
  - `LvlSetScript` — đọc ở `0x080E9200`
  - `MaxLevel` — đọc ở `0x080E7200`
  - `MaxShadowNum` — đọc ở `0x080E9200`
  - `MisslesForm` — đọc ở `0x080E9200`
  - `MslsGenerate` — đọc ở `0x080E9200`
  - `MslsGenerateData` — đọc ở `0x080E9200`
  - `Param1` — đọc ở `0x080E9200`
  - `Param2` — đọc ở `0x080E9200`
  - `PeaceCanUse` — đọc ở `0x080E9200`
  - `RelativePosType` — đọc ở `0x080E9200`
  - `ReqLevel` — đọc ở `0x080E9200`
  - `Series` — đọc ở `0x080E9200`
  - `SkillCostType` — đọc ở `0x080E9200`
  - `SkillId` — đọc ở `0x080A1D80`, `0x080E7200`, `0x080E9200`, `0x0811C3A0`
  - `SkillName` — đọc ở `0x080A1D80`, `0x080E9200`
  - `SkillStyle` — đọc ở `0x080E7200`, `0x080E9200`
  - `StartEvent` — đọc ở `0x080E9200`
  - `StartSkillId` — đọc ở `0x080E9200`
  - `StatePriority` — đọc ở `0x080E9200`
  - `StateSpecialId` — đọc ở `0x080E9200`
  - `StopWhenMove` — đọc ở `0x080E9200`
  - `TargetAlly` — đọc ở `0x080E9200`
  - `TargetEnemy` — đọc ở `0x080E9200`
  - `TargetNoNpc` — đọc ở `0x080E9200`
  - `TargetObj` — đọc ở `0x080E9200`
  - `TargetOnly` — đọc ở `0x080E9200`
  - `TargetOther` — đọc ở `0x080E9200`
  - `TargetSelf` — đọc ở `0x080E9200`
  - `TimePerCast` — đọc ở `0x080E9200`
  - `TimePerCastOnHorse` — đọc ở `0x080E9200`
  - `VanishedEvent` — đọc ở `0x080E9200`
  - `VanishedSkillId` — đọc ở `0x080E9200`
  - `WaitTime` — đọc ở `0x080E9200`
  - `WeaponSkill` — đọc ở `0x080E9200`

## `\settings\stallscript_permit.ini`
- bảng cục bộ `[ebp-0x7C]`, nạp ở `0x080DEEA0`, **2 cột/khoá**
  - `[(section ghép lúc chạy)] Function` — đọc ở `0x080DEEA0`
  - `[Global] Count` — đọc ở `0x080DEEA0`

## `\settings\Station.txt`
- đối tượng `0x08BB8620`, nạp ở `0x0805D580`, **2 cột/khoá**, đọc theo chỉ số cột ở `0x08106070`
  - `COUNT` — đọc ở `0x08106070`
  - `DESC` — đọc ở `0x08110BD0`

## `\settings\StationPrice.txt`
- đối tượng `0x08BB8660`, nạp ở `0x0805D580`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x0805D580`, `0x080F9620`

## `\settings\systemtimetask.txt`
- đối tượng `0x09780AC0`, nạp ở `0x0805D390`, **3 cột/khoá**
  - `HOUR` — đọc ở `0x0805D390`, `0x080F9130`
  - `MIN` — đọc ở `0x0805D390`, `0x080F9130`
  - `SCRIPT` — đọc ở `0x080F6F50`

## `\settings\task\120skill\newskill_explimit.txt`
- bảng cục bộ `[ebp-0x7C]`, nạp ở `0x080E7200`, **1 cột/khoá**
  - `MAXEXP_PERDAY` — đọc ở `0x080E7200`

## `\settings\task\missions.txt`
- đối tượng `0x09780B00`, nạp ở `0x0805D580`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x0808C2E0`, `0x08131890`, `0x08131D70`, `0x08132380`, `0x081327E0`, `0x08132E50`, `0x081332F0`, `0x081372A0`, `0x08137E40`

## `\settings\task\task_id.txt`
- bảng cục bộ `[ebp-0xB4]`, nạp ở `0x081725F0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08171390`

## `\settings\task\task_type.txt`
- bảng cục bộ `[ebp-0x5C]`, nạp ở `0x081725F0`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x08170990`

## `\settings\taxrates.ini`
- bảng cục bộ `[ebp-0xA0]`, nạp ở `0x0805EA10`, **4 cột/khoá**
  - `[CityRates] (khoá ghép lúc chạy)` — đọc ở `0x080C8FD0`
  - `[Main] CommonRate` — đọc ở `0x080C8FD0`
  - `[Main] MaxCityCount` — đọc ở `0x080C8FD0`
  - `[Main] MinPlayerLevelAllowed` — đọc ở `0x080C8FD0`

## `\SETTINGS\THIEFSKILL.TXT`
- bảng cục bộ `[ebp-0x5C]`, nạp ở `0x080E7200`, **2 cột/khoá**
  - `MaxLevel` — đọc ở `0x080E7200`
  - `SkillId` — đọc ở `0x080E7200`
- bảng cục bộ `[ebp-0x38]`, nạp ở `0x080E6E10`, **13 cột/khoá**
  - `AttackRadius` — đọc ở `0x080FA0F0`
  - `Cost` — đọc ở `0x080FA0F0`
  - `CostUsePrecent` — đọc ở `0x080FA0F0`
  - `Param1` — đọc ở `0x080FA0F0`
  - `SkillCostType` — đọc ở `0x080FA0F0`
  - `SkillName` — đọc ở `0x080FA0F0`
  - `TargetAlly` — đọc ở `0x080FA0F0`
  - `TargetEnemy` — đọc ở `0x080FA0F0`
  - `TargetObj` — đọc ở `0x080FA0F0`
  - `ThiefPercent` — đọc ở `0x080FA0F0`
  - `ThiefStyle` — đọc ở `0x080FA0F0`
  - `TimePerCast` — đọc ở `0x080FA0F0`
  - `TimePerCastOnHorse` — đọc ở `0x080FA0F0`

## `\settings\timertask.txt`
- đối tượng `0x09780B20`, nạp ở `0x080F9500`, **1 cột/khoá**
  - `SCRIPT` — đọc ở `0x080F9250`

## `\settings\tong\tong_setting.ini`
- bảng cục bộ `[ebp-0x90]`, nạp ở `0x0805FAE0`, **30 cột/khoá**
  - `[Debug] DebugOutOnLoad` — đọc ở `0x0805FAE0`
  - `[DefaultCall] Director` — đọc ở `0x0805FAE0`
  - `[DefaultCall] Manager` — đọc ở `0x0805FAE0`
  - `[DefaultCall] Master` — đọc ở `0x0805FAE0`
  - `[DefaultCall] Normal` — đọc ở `0x0805FAE0`
  - `[DefaultCall] Retire` — đọc ở `0x0805FAE0`
  - `[InstateCheck] InstateDirectorCheck` — đọc ở `0x0805FAE0`
  - `[InstateCheck] InstateManagerCheck` — đọc ở `0x0805FAE0`
  - `[InstateCheck] InstateMasterCheck` — đọc ở `0x0805FAE0`
  - `[InstateCheck] InstateMemberCheck` — đọc ở `0x0805FAE0`
  - `[InstateCheck] InstateRetireCheck` — đọc ở `0x0805FAE0`
  - `[LevelExp] (khoá ghép lúc chạy)` — đọc ở `0x0805FAE0`
  - `[LevelExp] MaxLevel` — đọc ở `0x0805FAE0`
  - `[LevelUnionNum] (khoá ghép lúc chạy)` — đọc ở `0x0805FAE0`
  - `[MoneyToExp] LimitExp` — đọc ở `0x0805FAE0`
  - `[MoneyToExp] LimitMoney` — đọc ở `0x0805FAE0`
  - `[MoneyToExp] MoneyLimit` — đọc ở `0x0805FAE0`
  - `[MoneyToExp] NormalExp` — đọc ở `0x0805FAE0`
  - `[MoneyToExp] NormalMemberLimit` — đọc ở `0x0805FAE0`
  - `[MoneyToExp] NormalMemberLimitMoney` — đọc ở `0x0805FAE0`
  - `[MoneyToExp] NormalMoney` — đọc ở `0x0805FAE0`
  - `[MoneyToExp] TestExp` — đọc ở `0x0805FAE0`
  - `[MoneyToExp] TimeLong` — đọc ở `0x0805FAE0`
  - `[TongCreate] LeadLeval` — đọc ở `0x0805FAE0`
  - `[TongCreate] Level` — đọc ở `0x0805FAE0`
  - `[TongScript] MixScript` — đọc ở `0x0805FAE0`
  - `[TongScript] TongScript` — đọc ở `0x0805FAE0`
  - `[TongTest] ActiveTime` — đọc ở `0x0805FAE0`
  - `[TongTest] MinMember` — đọc ở `0x0805FAE0`
  - `[TongTest] TimeLong` — đọc ở `0x0805FAE0`

## `\settings\tong\TongSet.ini`
- bảng cục bộ `[ebp-0xE4]`, nạp ở `0x080C5980`, **2 cột/khoá**
  - `[TongCreate] LeadLevel` — đọc ở `0x080C5980`
  - `[TongCreate] Level` — đọc ở `0x080C5980`

## `\settings\tong\workshop\workshops.txt`
- bảng cục bộ `[ebp-0x3C]`, nạp ở `0x080602F0`, **8 cột/khoá**
  - `CLOSE_ICON` — đọc ở `0x080602F0`
  - `COEFFICIENT` — đọc ở `0x080602F0`
  - `DESC` — đọc ở `0x080602F0`
  - `NAME` — đọc ở `0x080602F0`
  - `OPEN_ICON` — đọc ở `0x080602F0`
  - `SCRIPT` — đọc ở `0x080602F0`
  - `TYPE` — đọc ở `0x080602F0`
  - `UNFOUNDED_ICON` — đọc ở `0x080602F0`

## `\settings\trip_config.ini`
- bảng cục bộ `[ebp-0xD0]`, nạp ở `0x08051230`, **1 cột/khoá**
  - `[System] Type` — đọc ở `0x08051230`

## `\settings\utilities.ini`
- đối tượng `0x0830AE40`, nạp ở `0x0805EA10`, **1 cột/khoá**
  - `[DisguiseMask] ForbitFeature` — đọc ở `0x08096F50`

## `\settings\WayPoint.txt`
- đối tượng `0x08BB8600`, nạp ở `0x0805D580`, **3 cột/khoá**, đọc theo chỉ số cột ở `0x08105F30`
  - `DESC` — đọc ở `0x08110AE0`
  - `FightState` — đọc ở `0x08110910`
  - `SECT` — đọc ở `0x08105F30`

## `\settings\WayPointPrice.txt`
- đối tượng `0x08BB8680`, nạp ở `0x0805D580`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x0805D580`, `0x080F96E0`

## `\settings\Weather\Weather.ini`
- bảng cục bộ `[ebp-0x98]`, nạp ở `0x080FB430`, **2 cột/khoá**
  - `[(section ghép lúc chạy)] LifeTimeMax` — đọc ở `0x080FB430`
  - `[(section ghép lúc chạy)] LifeTimeMin` — đọc ở `0x080FB430`

## `\settings\Wharf.txt`
- đối tượng `0x08BB8640`, nạp ở `0x0805D580`, **2 cột/khoá**, đọc theo chỉ số cột ở `0x08105D30`
  - `COUNT` — đọc ở `0x08105D30`
  - `DESC` — đọc ở `0x08106250`

## `\settings\WharfPrice.txt`
- đối tượng `0x08BB86A0`, nạp ở `0x0805D580`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x0805D580`, `0x080F9560`

## `\settings\武器物理攻击对照表.txt`
- bảng cục bộ `[ebp-0x50]`, nạp ở `0x0805EA10`, **3 cột/khoá**, đọc theo chỉ số cột ở `0x0805EA10`
  - `DetailType` — đọc ở `0x0805EA10`
  - `ParticularType` — đọc ở `0x0805EA10`
  - `PhysicsSkillID` — đọc ở `0x0805EA10`

## `\SysAlarmConfig.ini`
- đối tượng `0x0978304C`, nạp ở `0x0815A3C0`, **3 cột/khoá**, ghi 3 khoá
  - `[DupedItem] DelAllInOne` — đọc ở `0x0815A3C0`
  - `[DupedItem] ForbitLevel` — đọc ở `0x0815A3C0`
  - `[DupedItem] OpLevel` — đọc ở `0x0815A3C0`
  - `[DupedItem] DelAllInOne` — **ghi** ở `0x08101F40`, `0x0815A3C0`
  - `[DupedItem] ForbitLevel` — **ghi** ở `0x0815A3C0`
  - `[DupedItem] OpLevel` — **ghi** ở `0x08102000`, `0x0815A3C0`

## `\ui\ui3\摆摊广告条.ini`
- bảng cục bộ `[ebp-0x90]`, nạp ở `0x080C27B0`, **0 cột/khoá**

## `package.ini`
- bảng cục bộ `[ebp-0xA0]`, nạp ở `0x0804C910`, **2 cột/khoá**
  - `[Package] (khoá ghép lúc chạy)` — đọc ở `0x08225DD0`
  - `[Package] Path` — đọc ở `0x08225DD0`

## `servercfg.ini`
- bảng cục bộ `[ebp-0x94]`, nạp ở `0x0804C910`, **6 cột/khoá**
  - `[(section ghép lúc chạy)] (khoá ghép lúc chạy)` — đọc ở `0x0804C320`
  - `[(section ghép lúc chạy)] BufferSize` — đọc ở `0x0804C320`
  - `[(section ghép lúc chạy)] Port` — đọc ở `0x0804C320`
  - `[FixIp] IntranetIp` — đọc ở `0x0804C910`
  - `[GameServer] Port` — đọc ở `0x0804C910`
  - `[Overload] MaxPlayer` — đọc ở `0x0804C910`

## `ServerCfg.ini`
- bảng cục bộ `[ebp-0x60]`, nạp ở `0x08051230`, **0 cột/khoá**, ghi 1 khoá
  - `[GameServer] GatewayID` — **ghi** ở `0x08051230`

## `settings/meridian/meridian.txt`
- bảng cục bộ `[ebp-0x3C]`, nạp ở `0x081E4F20`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x081E4F20`

## `settings/meridian/meridian_level.txt`
- bảng cục bộ `[ebp-0x5C]`, nạp ở `0x081E4F20`, **0 cột/khoá**, đọc theo chỉ số cột ở `0x081E4F20`

## Cột đọc qua đối tượng chưa nối được tên tệp (theo hàm đọc)

- `0x080688B0`: `(đọc theo chỉ số cột)`
- `0x080723B0`: `[Descript] (khoá ghép lúc chạy)`
- `0x080F1110`: `[List] (khoá ghép lúc chạy)`, `[List] Default_NewWorldScript`
- `0x0814AAF0`: `[?] (khoá ghép lúc chạy)`
- `0x0814ABD0`: `[?] (khoá ghép lúc chạy)`