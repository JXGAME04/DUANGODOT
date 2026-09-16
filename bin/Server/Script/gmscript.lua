Include("\\script\\tong\\tonghead.lua")

function IsGM(acc, name) --kiem tra khi login co phai la GM hay khong
	if(acc == "a") then
		return 1
	end
	return 0
end

function main(nParam) --nhan' G: chay script nay, khong reload; nhan' L: reload script nay, roi moi' chay.
	--Earn(5000000)
	--AddMagic(210,1)
	--while GetLevel() < 100 do
	--	AddOwnExp(9999999)
	--end
	Msg2Player("pid "..PlayerIndex)
end
--NEXTMAP = 0
function exenpc(nNpcIdx, nPlayerIdx)
	--Msg2Player("Npcdata : "..GetNpcData(nNpcIdx,"Exp",20,4))
	--Msg2Player("Npclvscr:["..GetNpcLvScript(nNpcIdx).."]")
	Msg2Player("NpcId:["..PlayerIndex.."]["..nNpcIdx.."]["..nPlayerIdx.."]")
end
