#ifndef KMagicDescH
#define	KMagicDescH

#include "KIniFile.h"

class KMagicDesc
{
private:
	KIniFile	m_IniFile;
	char		m_szDesc[256];
public:
	KMagicDesc();
	~KMagicDesc();
	BOOL		Init();
	const char*	GetDesc(void* pData);
};

extern KMagicDesc	g_MagicDesc;
#endif
