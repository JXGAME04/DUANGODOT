function GetSkillLevelData(levelname, data, level)

if (levelname == "magic_fastwalkrun_p") then
return Getmagic_fastwalkrun_p(level)
end;


str1 = ""
return str1
end;


function Getmagic_fastwalkrun_p(level)
result = 35+floor(level*9.5)
str1 = ""..result
return str1
end;
