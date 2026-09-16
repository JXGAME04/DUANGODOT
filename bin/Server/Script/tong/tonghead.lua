
TB_WEEKGOAL_TYPE_NAME	=	{ "ChiÕn tr­êng Tèng Kim", "NhiÖm vô TÝn Sø", "Th¸ch thøc thêi gian", "Chuçi nhiÖm vô D· TÈu"}
TB_WEEKGOAL_CHANGE		=	{ 4,	8,		16 }
TB_WEEKGOAL_PRICE_BASE	=	{ 2.5,	4.5,	6.75 }

--ham` co' dinh., khong thay doi~
function GenerateTask(nLevel, nMembers)
	nLevel = nLevel + 1
	if nLevel > 3 then
		nLevel = 3
	end
	local nType = random(getn(TB_WEEKGOAL_TYPE_NAME))
	local nHourValue = random(150, 170)
	local nWeekGoalPlayer = TB_WEEKGOAL_CHANGE[nLevel] * nHourValue --muc tieu cua mem
	local nWeekGoalTotal = floor(0.4 * nMembers * nWeekGoalPlayer) --muc tieu cua bang
	local nWeekGoalPricePlayer = floor(TB_WEEKGOAL_PRICE_BASE[nLevel] * nHourValue) --phan thuong mem
	local nWeekGoalPriceTong = floor(TB_WEEKGOAL_PRICE_BASE[nLevel] * nHourValue * 0.4 * nMembers) --phan thuong bang
	--tra ve 5 thong so'
	return nType,nWeekGoalPlayer,nWeekGoalTotal,nWeekGoalPricePlayer,nWeekGoalPriceTong
end

function RandomSeed(seed)
	randomseed(seed) --lam cho random ngau~ nhien, khong bi lap lai.
	random(8) --chay. khoi~ dong may'
	random(9) --chay. khoi~ dong may'
end

function ContriValueEntry(nValue, nEntry)
	local nTongID = GetTongID()
	if (nTongID == 0) or (GetWeekGoal() ~= nEntry) or (nValue <= 0) then
		return
	end
	TAddWeekGoal(nTongID, GetUUID(), nValue) --send to database
end
