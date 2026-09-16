
function main()
	Say("Ng­¬i t×m ta cã viÖc g×?",4,
	"PhÇn th­ëng môc tiªu tuÇn c¸ nh©n/personalprice",
	"ThiÕt lËp ®é khã môc tiªu tuÇn/tong_levelchoose",
	"PhÇn th­ëng môc tiªu tuÇn bang héi/tong_award",
	"Hñy bá /tong_cancel")
end

function tong_levelchoose()
	Say("ThiÕt lËp ®é khã môc tiªu tuÇn hiÖn t¹i sÏ cã t¸c dông vµo tuÇn sau, muèn thiÕt lËp l¹i ®é khã cña môc tiªu tuÇn kh«ng?",
	4,
	"§é khã cÊp 0/select_level",
	"§é khã cÊp 1/select_level",
	"§é khã cÊp 2/select_level",
	"Kh«ng cÇn ®©u/tong_cancel")
end

function select_level(sellv)
	local nTongID = GetTongID()
	if(nTongID == 0) then
		Say("Ng­êi trong bang héi míi cã thÓ qu¶n lı môc tiªu tuÇn, ng­¬i muèn gia nhËp vµo mét bang héi chø!", 0)
	return end
	local nRet = TSetWeekGoalLevel(nTongID, sellv);
	if(nRet == 0) then
		Say("Ng­¬i kh«ng cã quyÒn h¹n qu¶n lı môc tiªu tuÇn, h·y mêi bang chñ ®Õn ®©y!", 0)
	elseif(nRet == 1) then
		Say("<#>§é khã môc tiªu tuÇn hiÖn t¹i lµ: <color=yellow>"..sellv.." cÊp", 0)
	end
end

function personalprice()
	if(GetTongID() == 0) then
		Say("Ng­¬i kh«ng gia nhËp bang héi, kh«ng lµm g× th× sao cã thÓ nhËn th­ëng môc tiªu tuÇn!", 0)
	return end
	TPlayerPrice()
end

function tong_award()
	local nTongID = GetTongID()
	if(nTongID == 0) then
		Say("Ng­¬i kh«ng gia nhËp bang héi, cã quyÒn g× mµ nhËn th­ëng môc tiªu tuÇn!", 0)
	return end
	if(TWeekGoalPrice(nTongID) == 0) then
		Say("Ng­¬i kh«ng cã quyÒn h¹n qu¶n lı môc tiªu tuÇn, kh«ng thÓ l·nh th­ëng!", 0);
	end
end

function tong_cancel()
end

--goi tu server co' dinh.
function ReceiveTongPrice(nResult)
	if(nResult == 0) then
		Say("§· nhËn phÇn th­ëng råi, cßn muèn nhËn n÷a sao!",0)
	elseif(nResult == 1) then
		Say("TuÇn tr­íc kh«ng cã môc tiªu tuÇn, kh«ng thÓ nhËn th­ëng!",0)
	elseif(nResult == 2) then
		Say("Quİ bang ch­a hoµn thµnh môc tiªu tuÇn tr­íc, kh«ng thÓ nhËn th­ëng!",0)
	else
		Msg2Player("NhËn ®­îc phÇn th­ëng môc tiªu tuÇn bang héi")
	end
end

--goi tu server co' dinh.
function ReceivePlayerPrice(nPrice)
	AddOffer(nPrice)
	Msg2Player("<#>§· hoµn thµnh môc tiªu tuÇn bang héi, nhËn ®­îc phÇn th­ëng ®iÓm cèng hiÕn: "..nPrice.." ®iÓm")
	AddOwnExp(nPrice*10000)
end

--goi tu server co' dinh.
function FailPlayerPrice(nResult)
	if(nResult == 1) then
		Say("TuÇn tr­íc quİ bang kh«ng cã môc tiªu tuÇn, kh«ng thÓ nhËn th­ëng!",0)
	elseif(nResult == 2) then
		Say("§· nhËn phÇn th­ëng môc tiªu tuÇn tr­íc råi, cÇn tiÕp tôc cèng hiÕn cho bang héi, tuÇn sau h·y ®Õn nhĞ.",0)
	elseif(nResult == 3) then
		Say("TuÇn tr­íc ch­a lµm viÖc g× c¶, lµm g× cã chuyÖn kh«ng c«ng ®­îc th­ëng chø?",0)
	elseif(nResult == 4) then
		Say("Ch­a hoµn thµnh môc tiªu tuÇn tr­íc, kh«ng thÓ nhËn th­ëng, cÇn tiÕp tôc cè g¾ng cho bang héi, sím muén g× còng ®­îc ®Òn ®¸p th«i.",0)
	end
end
