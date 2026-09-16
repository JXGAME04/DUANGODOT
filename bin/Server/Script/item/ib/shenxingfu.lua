tab_lv90map = {
		{875,1576,3177	,"H¾c Sa ®éng",},
		{322,1589,3164	,"Tr­êng B¹ch S¬n B¾c",},
		{321,967,2313	,"Tr­êng B¹ch S¬n Nam",},
		{75,1811,3012	,"Kho¶ Lang ®éng",},
		{225,1474,3275	,"Sa M¹c Mª Cung 1",},
		{226,1560,3184	,"Sa M¹c Mª Cung 2",},
		{227,1588,3237	,"Sa M¹c Mª Cung 3",},
		{336,1124,3187	,"Phong L¨ng ®é",},
		{340,1845,3438	,"M¹c Cao QuËt",},
		{144,1691,3020	,"D­îc V­¬ng ®éng tÇng 4",},
		{93,1529,3166	,"TiÕn Cóc §éng MËt Cung",},
		{124,1675,3418	,"C¸n Viªn §éng Mª Cung",},
		{152,1672,3361	,"TuyÕt B¸o §éng TÇng 8",},
};

function GotoMapId(idx)
	if(GetItemCount(6,1,1266) == 0) then
		Msg2Player("§Ó sö dông ThÇn Hµnh Chi ThuËt trªn ng­êi cÇn cã ThÇn Hµnh Phï, cã ph¶i ®Ó quªn trong r­¬ng kh«ng?")
	return end
	--local nCurTime = tonumber(GetLocalDate("%y%m%d%H%M"))
	--if(nCurTime < SERVER_OPEN) then
	--	Msg2Player("§óng 20h míi Open Server .")
	--	return
	--end
	local nSubWorldID = GetWorldPos()
	
	if (GetTaskTemp(99) == 1 ) or ( nSubWorldID >= 387 and nSubWorldID <= 395)then
		Msg2Player("HiÖn t¹i ng­¬i kh«ng thÓ sö dông thÇn hµnh phï!")
		return
	end
	
	if (nSubWorldID >= 375 and nSubWorldID <= 386) then
		Msg2Player("B¶n ®å hiÖn t¹i ng­¬i ®ang ®øng thuéc khu vùc ®Æc thï, kh«ng thÓ sö dông thÇn hµnh phï.")
		return
	end
	
	if (nSubWorldID >= 416 and nSubWorldID <= 511) then
		Msg2Player("B¶n ®å hiÖn t¹i ng­¬i ®ang ®øng thuéc khu vùc ®Æc thï, kh«ng thÓ sö dông thÇn hµnh phï.")
		return
	end
	
	if (nSubWorldID == 995 or nSubWorldID == 324 or nSubWorldID == 44 or nSubWorldID == 197 or nSubWorldID == 208 or nSubWorldID == 209 or nSubWorldID == 210 or nSubWorldID == 211 or nSubWorldID == 212 or (nSubWorldID >= 213 and nSubWorldID <= 223)	or nSubWorldID == 336 or nSubWorldID == 341 or nSubWorldID == 342	or nSubWorldID == 175	or nSubWorldID == 337	or nSubWorldID == 338	or nSubWorldID == 339 or ( nSubWorldID >= 387 and  nSubWorldID <= 395 ) )then 
		Msg2Player("B¶n ®å hiÖn t¹i ng­¬i ®ang ®øng thuéc khu vùc ®Æc thï, kh«ng thÓ sö dông thÇn hµnh phï.")
		return
	end

	if (CheckAllMaps(nSubWorldID) == 1) then
		Msg2Player("B¶n ®å hiÖn t¹i ng­¬i ®ang ®øng thuéc khu vùc ®Æc thï, kh«ng thÓ sö dông thÇn hµnh phï.")
		return
	end;
	
	if (GetLevel() < 10) then
		Msg2Player("Ng­êi ch¬i ph¶i ®¹t ®¼ng cÊp 10 trë lªn míi cã thÓ sö dông thÇn hµnh phï.")
		return
	end

	local ar = 0;
	for i=1,getn(tab_lv90map) do
		if(tab_lv90map[i][1] == idx) then
		ar = i
		break end
	end
	if(ar > 0) then
		gopos_step3lv90(ar)
	return end
	for i=1,getn(tab_lv80map) do
		if(tab_lv80map[i][1] == idx) then
		ar = i
		break end
	end
	if(ar > 0) then
		gopos_step3lv80(ar)
	return end
	for i=1,getn(tab_lv70map) do
		if(tab_lv70map[i][1] == idx) then
		ar = i
		break end
	end
	if(ar > 0) then
		gopos_step3lv70(ar)
	return end
	for i=1,getn(tab_lv60map) do
		if(tab_lv60map[i][1] == idx) then
		ar = i
		break end
	end
	if(ar > 0) then
		gopos_step3lv60(ar)
	return end
	for i=1,getn(tab_lv50map) do
		if(tab_lv50map[i][1] == idx) then
		ar = i
		break end
	end
	if(ar > 0) then
		gopos_step3lv50(ar)
	return end
	for i=1,getn(tab_lv40map) do
		if(tab_lv40map[i][1] == idx) then
		ar = i
		break end
	end
	if(ar > 0) then
		gopos_step3lv40(ar)
	return end
	for i=1,getn(tab_lv30map) do
		if(tab_lv30map[i][1] == idx) then
		ar = i
		break end
	end
	if(ar > 0) then
		gopos_step3lv30(ar)
	return end
	for i=1,getn(tab_lv20map) do
		if(tab_lv20map[i][1] == idx) then
		ar = i
		break end
	end
	if(ar > 0) then
		gopos_step3lv20(ar)
	return end
end;

function gopos_step3lv90(nIdx)
	NewWorld(tab_lv90map[nIdx][1], tab_lv90map[nIdx][2], tab_lv90map[nIdx][3])
	SetFightState(1);
	Msg2Player("Ngåi cho ch¾c nhÐ! ta sÏ ®­a ng­¬i ®Õn "..tab_lv90map[nIdx][4]);
end

function main()
	Say("thÇn hµnh phï",2,
	"tíi ®©u/no",
	"Tho¸t/no")
	return 1
end

function no()

end
