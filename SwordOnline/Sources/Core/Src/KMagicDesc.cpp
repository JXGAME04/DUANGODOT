#include "KCore.h"

#include "KEngine.h"
#include "KMagicAttrib.h"
#include "KMagicDesc.h"
#include "GameDataDef.h"
#include "KFaction.h"

#define		MAGICDESC_FILE		"\\settings\\MagicDesc.Ini"
extern const char * g_MagicID2String(int nAttrib);
const char MAGIC_ATTRIB_STRING[][100] = 
{
//¸Ä±ä¼¼ÄÜµÄÊôÐÔ
/*0  */	"skill_begin",
/*1  */	"skill_cost_v",							// ÏûºÄMANA
/*2  */	"skill_costtype_v",		//	
/*3  */	"skill_mintimepercast_v",		// Ã¿´Î·¢Ä§·¨µÄ¼ä¸ôÊ±¼ä
/*4  */	"skill_misslenum_v",		// 
/*5  */	"skill_misslesform_v",		
/*6  */	"skill_param1_v",		
/*7  */	"skill_param2_v",
/*8  */	"skill_attackradius",
/*9  */	"skill_reserve2",
/*10 */	"skill_reserve3",
/*11 */	"skill_reserve4",
/*12 */	"skill_eventskilllevel",
/*13 */	"skill_end",
/*14 */	"missle_begin",
/*15 */	"missle_movekind_v",
/*16 */	"missle_speed_v",
/*17 */	"missle_lifetime_v",
/*18 */	"missle_height_v",
/*19 */	"missle_damagerange_v",
/*20 */	"missle_radius_v",
/*21 */	"missle_reserve1",
/*22 */	"missle_reserve2",
/*23 */	"missle_reserve3",
/*24 */	"missle_reserve4",
/*25 */	"missle_reserve5",
/*26 */	"missle_end",
/*27 */	"item_begin",
/*28 */	"weapondamagemin_v",
/*29 */	"weapondamagemax_v",
/*30 */	"armordefense_v",
/*31 */	"durability_v",
/*32 */	"requirestr",
/*33 */	"requiredex",
/*34 */	"requirevit",
/*35 */	"requireeng",
/*36 */	"requirelevel",
/*37 */	"requireseries",
/*38 */	"requiresex",
/*39 */	"requiremenpai",
/*40 */	"weapondamageenhance_p",
/*41 */	"armordefenseenhance_p",
/*42 */	"requirementreduce_p",
/*43 */	"indestructible_b",
/*44 */	"item_reserve1",
/*45 */	"item_reserve2",
/*46 */	"require_translife",
/*47 */	"require_fortune_value",
/*48 */	"item_reserve5",
/*49 */	"item_reserve6",
/*50 */	"item_reserve7",
/*51 */	"item_reserve8",
/*52 */	"item_reserve9",
/*53 */	"item_reserve10",
/*54 */	"item_end",
/*55 */	"damage_begin",		
/*56 */	"attackrating_v",
/*57 */	"attackrating_p",
/*58 */	"ignoredefense_p",
/*59 */	"physicsdamage_v",
/*60 */	"colddamage_v",
/*61 */	"firedamage_v",
/*62 */	"lightingdamage_v",
/*63 */	"poisondamage_v",
/*64 */	"magicdamage_v",
/*65 */	"physicsenhance_p",
/*66 */	"steallife_p",
/*67 */	"stealmana_p",
/*68 */	"stealstamina_p",
/*69 */	"knockback_p",
/*70 */	"deadlystrike_p",
/*71 */	"fatallystrike_p",
/*72 */	"stun_p",
/*73 */	"damage_reserve1",
/*74 */	"damage_reserve2",
/*75 */	"damage_reserve3",
/*76 */	"damage_reserve4",
/*77 */	"damage_reserve5",
/*78 */	"damage_reserve6",
/*79 */	"damage_reserve7",
/*80 */	"damage_reserve8",
/*81 */	"damage_reserve9",
/*82 */	"damage_reserve10",
/*83 */	"damage_end",
/*84 */	"normal_begin",
/*85 */	"lifemax_v",
/*86 */	"lifemax_p",
/*87 */	"life_v",
/*88 */	"lifereplenish_v",
/*89 */	"manamax_v",
/*90 */	"manamax_p",
/*91 */	"mana_v",
/*92 */	"manareplenish_v",
/*93 */	"staminamax_v",
/*94 */	"staminamax_p",
/*95 */	"stamina_v",
/*96 */	"staminareplenish_v",
/*97 */	"strength_v",
/*98 */	"dexterity_v",
/*99 */	"vitality_v",
/*100*/	"energy_v",
/*101*/	"poisonres_p",
/*102*/	"fireres_p",
/*103*/	"lightingres_p",
/*104*/	"physicsres_p",
/*105*/	"coldres_p",
/*106*/	"freezetimereduce_p",
/*107*/	"burntimereduce_p",
/*108*/	"poisontimereduce_p",
/*109*/	"poisondamagereduce_v",
/*110*/	"stuntimereduce_p",
/*111*/	"fastwalkrun_p",
/*112*/	"visionradius_p",
/*113*/	"fasthitrecover_v",
/*114*/	"allres_p",
/*115*/	"attackspeed_v",
/*116*/	"castspeed_v",
/*117*/	"meleedamagereturn_v",
/*118*/	"meleedamagereturn_p",
/*119*/	"rangedamagereturn_v",
/*120*/	"rangedamagereturn_p",
/*121*/	"addphysicsdamage_v",
/*122*/	"addfiredamage_v",
/*123*/	"addcolddamage_v",
/*124*/	"addlightingdamage_v",
/*125*/	"addpoisondamage_v",
/*126*/	"addphysicsdamage_p",
/*127*/	"slowmissle_b",
/*128*/	"changecamp_b",
/*129*/	"physicsarmor_v",
/*130*/	"coldarmor_v",
/*131*/	"firearmor_v",
/*132*/	"poisonarmor_v",
/*133*/	"lightingarmor_v",
/*134*/	"damage2addmana_p",
/*135*/	"lucky_v",
/*136*/	"steallifeenhance_p",
/*137*/	"stealmanaenhance_p",
/*138*/	"stealstaminaenhance_p",
/*139*/	"allskill_v",
/*140*/	"metalskill_v",
/*141*/	"woodskill_v",
/*142*/	"waterskill_v",
/*143*/	"fireskill_v",
/*144*/	"earthskill_v",
/*145*/	"knockbackenhance_p",
/*146*/	"deadlystrikeenhance_p",
/*147*/	"stunenhance_p",
/*148*/	"badstatustimereduce_v",
/*149*/	"manashield_p",
/*150*/	"adddefense_v",
/*151*/	"adddefense_p",
/*152*/	"fatallystrikeenhance_p",
/*153*/	"lifepotion_v",
/*154*/	"manapotion_v",
/*155*/	"physicsresmax_p",
/*156*/	"coldresmax_p",
/*157*/	"fireresmax_p",
/*158*/	"lightingresmax_p",
/*159*/	"poisonresmax_p",
/*160*/	"allresmax_p",
/*161*/	"coldenhance_p",
/*162*/	"fireenhance_p",
/*163*/	"lightingenhance_p",
/*164*/	"poisonenhance_p",
/*165*/	"magicenhance_p",
/*166*/	"attackratingenhance_v",
/*167*/	"attackratingenhance_p",
/*168*/ "addphysicsmagic_v",
/*169*/ "addcoldmagic_v",
/*170*/ "addfiremagic_v",
/*171*/ "addlightingmagic_v",
/*172*/ "addpoisonmagic_v",
/*173*/ "fatallystrikeres_p",
/*174*/ "magicreserve",
/*175*/ "magicreserve",
/*176*/ "expenhance_p",
/*177*/ "magicreserve",
/*178*/ "magicreserve",
/*179*/ "magicreserve",
/*180*/ "magicreserve",
/*181*/ "dynamicmagicshield_v",
/*182*/ "magicreserve",
/*183*/ "magicreserve",
/*184*/ "magicreserve",
/*185*/ "magicreserve",
/*186*/ "magicreserve",
/*187*/ "addstealfeatureskill",
/*188*/ "lucky_v_partner",
/*189*/ "magicreserve",
/*190*/ "lifereplenish_p",
/*191*/ "ignoreskill_p",
/*192*/ "returnskill_p",
/*193*/ "poisondamagereturn_v",
/*194*/ "poisondamagereturn_p",
/*195*/ "autoreplyskill",
/*196*/ "magicreserve",
/*197*/ "magicreserve",
/*198*/ "magicreserve",
/*199*/ "magicreserve",
/*200*/ "hide",
/*201*/ "magicreserve",
/*202*/ "poison2decmana_p",
/*203*/ "magicreserve",
/*204*/ "magicreserve",
/*205*/ "returnres_p",
/*206*/ "magicreserve",
/*207*/ "magicreserve",
/*208*/ "magicreserve",
/*209*/ "magicreserve",
/*210*/ "magicreserve",
/*211*/ "magicreserve",
/*212*/ "magicreserve",
/*213*/ "magicreserve",
/*214*/ "magicreserve",
/*215*/ "magicreserve",
/*216*/ "magicreserve",
/*217*/ "magicreserve",
/*218*/ "sorbdamage_p",
/*219*/ "anti_hitrecover",
/*220*/ "anti_stuntimereduce_p",
/*221*/ "anti_poisonres_p",
/*222*/ "anti_fireres_p",
/*223*/ "anti_lightingres_p",
/*224*/ "anti_physicsres_p",
/*225*/ "anti_coldres_p",
/*226*/ "block_rate",
/*227*/ "enhancehit_rate",
/*228*/ "poisonres_yan_p",
/*229*/ "lightingres_yan_p",
/*230*/ "fireres_yan_p",
/*231*/ "physicsres_yan_p",
/*232*/ "coldres_yan_p",
/*233*/ "lifemax_yan_v",
/*234*/ "lifemax_yan_p",
/*235*/ "manamax_yan_v",
/*236*/ "manamax_yan_p",
/*237*/ "sorbdamage_yan_p",
/*238*/ "fastwalkrun_yan_p",
/*239*/ "attackspeed_yan_v",
/*240*/ "castspeed_yan_v",
/*241*/ "allres_yan_p",
/*242*/ "anti_maxres_p",
/*243*/ "skill_enhance",
/*244*/ "magicdamage_p",
/*245*/ "fasthitrecover_yan_v",
/*246*/ "five_elements_enhance_v",
/*247*/ "five_elements_resist_v",
/*248*/ "manareplenish_p",
/*249*/ "add_damage_p",
/*250*/ "magicreserve",
/*251*/ "magicreserve",
/*252*/ "magicreserve",
/*253*/ "magicreserve",
/*254*/ "not_add_pkvalue_p",
/*255*/ "add_boss_damage",
/*256*/ "pk_punish_weaken",
/*257*/ "pk_punish_enhance",
/*258*/ "anti_poisontimereduce_p",
/*259*/ "do_hurt_p",
/*260*/ "anti_do_hurt_p",
/*261*/ "do_stun_p",
/*262*/ "anti_do_stun_p",
/*263*/ "anti_physicsres_yan_p",
/*264*/ "anti_poisonres_yan_p",
/*265*/ "anti_coldres_yan_p",
/*266*/ "anti_fireres_yan_p",
/*267*/ "anti_lightingres_yan_p",
/*268*/ "anti_allres_yan_p",
/*269*/ "anti_sorbdamage_yan_p",
/*270*/ "anti_block_rate",
/*271*/ "anti_enhancehit_rate",
/*272*/ "magicreserve",
/*273*/ "magicreserve",
/*274*/ "magicreserve",
/*275*/ "enhancehiteffect_rate",

/*end*/	"normal_end",
};



KMagicDesc	g_MagicDesc;
KMagicDesc::KMagicDesc()
{
	m_szDesc[0] = 0;
}

KMagicDesc::~KMagicDesc()
{
}

BOOL KMagicDesc::Init()
{
//	g_SetFilePath("\\");
	return (m_IniFile.Load(MAGICDESC_FILE));
}

const char* KMagicDesc::GetDesc(void *pData)
{
	
	char	szTempDesc[256];
	char*	pTempDesc = szTempDesc;
	int		i = 0;

	ZeroMemory(m_szDesc, sizeof(m_szDesc));
	
	if (!pData)
		return NULL;

	KMagicAttrib* pAttrib = (KMagicAttrib *)pData;

	const char	*pszKeyName = g_MagicID2String(pAttrib->nAttribType);
	m_IniFile.GetString("Descript", pszKeyName, "", szTempDesc, sizeof(szTempDesc));
	while(*pTempDesc)
	{
		if (*pTempDesc == '#')
		{
			int	nDescAddType = 0;
			switch(*(pTempDesc + 3))
			{
			case '+':
				nDescAddType = 1;
				break;
			case '~':
				nDescAddType = 2;
				break;
			default:
				nDescAddType = 0;
				break;
			}
			int nValue = 0;
			
			switch(*(pTempDesc + 2))
			{
			case '1':
				nValue = pAttrib->nValue[0];
				break;
			case '2':
				nValue = pAttrib->nValue[1];
				break;
			case '3':
				nValue = pAttrib->nValue[2];
				break;
			case '7':
				nValue = pAttrib->nValue[0]-(pAttrib->nValue[0]/256)*256;
				break;
			case '9':
				nValue = pAttrib->nValue[2]-(pAttrib->nValue[2]/256)*256;
				break;
			default:
				nValue = pAttrib->nValue[0];
				break;
			}
			switch(*(pTempDesc+1))
			{
			case 'm':		// m«n ph¸i
				strcat(m_szDesc, g_Faction.m_sAttribute[nValue].m_szName);
				i += strlen(g_Faction.m_sAttribute[nValue].m_szName);
				break;
			case 's':		// ngò hµnh
				switch(nValue)
				{
				case series_metal:
					strcat(m_szDesc, "Kim");
					i += 3;
					break;
				case series_wood:
					strcat(m_szDesc, "Méc");
					i += 3;
					break;
				case series_water:
					strcat(m_szDesc, "Thñy");
					i += 4;
					break;
				case series_fire:
					strcat(m_szDesc, "Háa");
					i += 3;
					break;
				case series_earth:
					strcat(m_szDesc, "Thæ");
					i += 3;
					break;
				default:
					strcat(m_szDesc, "V« hÖ");
					i += 5;
					break;
				}
				break;
			case 'k':		// lo¹i h×nh tiªu hao
				switch(nValue)
				{
				case 0:
					strcat(m_szDesc, "ÄÚÁ¦");
					break;
				case 1:
					strcat(m_szDesc, "ÉúÃü");
					break;
				case 2:
					strcat(m_szDesc, "ÌåÁ¦");
					break;
				case 3:
					strcat(m_szDesc, "½ðÇ®");
					break;
				default:
					strcat(m_szDesc, "ÄÚÁ¦");
					break;
				}
				i += 4;
				break;
			case 'd':		// sè nguyªn
				{
					switch(nDescAddType)
					{
					case 1:
						if (nValue >= 0)
						{
							strcat(m_szDesc, "+");
							i++;
						}
						break;
					case 2:
						if (nValue >= 0)
						{
							strcat(m_szDesc, "-");
							i++;
						}
						else
						{
							nValue = -nValue;
							strcat(m_szDesc, "+");
							i++;
						}
						break;
					default:
						break;
					}
					char	szMsg[16];
					sprintf(szMsg, "%d", nValue);
					strcat(m_szDesc, szMsg);
					i += strlen(szMsg);
				}
				break;
			case 'l':
				if(nDescAddType == 0)
				{
					if(*(pTempDesc + 2) == 'A')
					{
						nValue = pAttrib->nValue[0]/256;
					}
					if(nValue > 0)
					{
						strcat(m_szDesc, "[ ");
						int nSkillId;
						for(int r = 0; r < g_OrdinSkillsSetting.GetHeight() - 1; ++r)
						{
							g_OrdinSkillsSetting.GetInteger(r+2, 3, 0, &nSkillId);
							if(nValue == nSkillId)
							{
								char	szMsg[80];
								g_OrdinSkillsSetting.GetString(r+2, 1, "", szMsg, sizeof(szMsg));
								strcat(m_szDesc, szMsg);
								i += strlen(szMsg);
								break;
							}
						}
						strcat(m_szDesc, " ]");
						i += 4;
					}
					else
					{
						strcat(m_szDesc, "Vâ c«ng vèn cã");
						i += 14;
					}
				}
				break;
			case 'x':		// giíi tÝnh
				if (nValue)
				{
					strcat(m_szDesc, "N÷");
					i += 2;
				}
				else
				{
					strcat(m_szDesc, "Nam");
					i += 3;
				}
				break;
			case 'f':
				{
					float fValue = (float)nValue;
					if(*(pTempDesc + 2) == '6')
					{
						fValue = (float)(pAttrib->nValue[2]/256)/18.0f;
					}
					switch(nDescAddType)
					{
					case 1:
						if (fValue >= 0.0f)
						{
							strcat(m_szDesc, "+");
							i++;
						}
						break;
					case 2:
						if (fValue >= 0.0f)
						{
							strcat(m_szDesc, "-");
							i++;
						}
						else
						{
							fValue = -fValue;
							strcat(m_szDesc, "+");
							i++;
						}
						break;
					default:
						break;
					}
					char	szMsg[16];
					sprintf(szMsg, "%.2f", fValue);
					strcat(m_szDesc, szMsg);
					i += strlen(szMsg);
				}break;
			default:
				break;
			}
			pTempDesc += 4;
		}
		else
		{
			m_szDesc[i] = *pTempDesc;
			pTempDesc++;
			i++;
		}
	}
	return m_szDesc;
}

const char * g_MagicID2String(int nAttrib)
{
	if ((nAttrib < 0) || nAttrib >= magic_normal_end) return MAGIC_ATTRIB_STRING[magic_normal_end];
	return 	MAGIC_ATTRIB_STRING[nAttrib];
}

int	g_String2MagicID(char * szMagicAttribName)
{
	if ((!szMagicAttribName) || (!szMagicAttribName[0])) return -1;

	//nValue2 µ±ÖµÎª-1Ê±ÎªÓÀ¾ÃÐÔ×´Ì¬£¬0Îª·Ç×´Ì¬£¬ÆäËüÖµÎªÓÐÊ±Ð§ÐÔ×´Ì¬Ä§·¨Ð§¹û
	//ÐèÒª½«×´Ì¬Êý¾ÝÓë·Ç×´Ì¬Êý¾Ý·ÖÀë³öÀ´£¬·ÅÈëÏàÓ¦µÄÊý×éÄÚ£¬²¢¼ÇÂ¼×ÜÊýÁ¿
	
	for (int i  = 0 ; i <= magic_normal_end; i ++)
	{
		if (!strcmp(szMagicAttribName, g_MagicID2String(i)))
			return i;
	}
	return -1;
}

