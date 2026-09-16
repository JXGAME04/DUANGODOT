--Á½ºþÇø °ÍÁêÏØ æäÕ¾³µ·ò¶Ô»°
-- Update: Dan_Deng(2003-08-21) ½µµÍ³ö´åµÈ¼¶ÒªÇóÎª5¼¶

--CurStation = 10;
--Include("\\script\\global\\station.lua")
--Include("\\script\\global\\skills_table.lua")

function main(sel)
	--NewcomerStationCommon("§©y tuy lµ khu vùc hå n­íc, nh­ng ®i ®­êng bé còng rÊt thuËn tiÖn, ng­¬i muèn ®i ®©u kh«ng? ");
	Say("Xa phu",4,
	"Nh÷ng n¬i ®· ®i qua/WayPointFun",
	"Trë l¹i ®Þa ®iÓm cò/TownPortalFun",
	"§i ®Õn n¬i lµm nhiÖm vô D· TÈu/tl_moveToTaskMap",
	"Kh«ng cÇn ®©u/no"
	)
end;

function WayPointFun()
	Say("N¬i ®i qua",4,
	"N¬i ®i qua 1/p1",
	"N¬i ®i qua 2/p2",
	"N¬i ®i qua 2/p3",
	"Tho¸t/no"
	)
end

function p1()
	Msg2Player("N¬i ®i qua 1")
end
function p2()
	Msg2Player("N¬i ®i qua 2")
end
function p3()
	Msg2Player("N¬i ®i qua 3")
end

function TownPortalFun()
	Msg2Player("trë l¹i ®iÓm cò")
end

function tl_moveToTaskMap()
	Msg2Player("nhiÖm vô")
end

function no()
end
