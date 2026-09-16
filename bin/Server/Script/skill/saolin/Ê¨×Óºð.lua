function GetSkillLevelData(levelname, data, level)

if (levelname == "stuntime_v") then
return Getstuntime_v(level)
end;

if (levelname == "skill_cost_v") then
return Getskill_cost_v(level)
end;

if (levelname == "physicsdamage_v") then
return Getphysicsdamage_v(level)
end;

str1 = ""
return str1
end;

function Param2String(Param1, Param2, Param3)
return Param1..","..Param2..","..Param3
end;


function Getstuntime_v(level)
result = 3+level
return Param2String(0,result,0)
end;

function Getskill_cost_v(level)
result = 40
return Param2String(result,0,0)
end;

function Getphysicsdamage_v(level)
result = 40+level*5
return Param2String(result,0,result)
end;