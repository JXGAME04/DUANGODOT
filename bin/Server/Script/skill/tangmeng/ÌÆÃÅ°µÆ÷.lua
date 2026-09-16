function GetSkillLevelData(levelname, data, level)

if (levelname == "attackratingenhance_p") then
return Getattackratingenhance_p(level)
end;


str1 = ""
return str1
end;

function Param2String(Param1, Param2, Param3)
return Param1..","..Param2..","..Param3
end;

function Getattackratingenhance_p(level)
result = 12+3*level
return Param2String(result,-1,0)
end;