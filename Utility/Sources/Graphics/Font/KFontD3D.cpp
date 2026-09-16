/*******************************************************************************
// FileName			:	KFontD3D.cpp
// FileAuthor		:	Wooy
// FileCreateDate	:	2001-9-13 10:19:21
// FileDescription	:	字体类
// Revision Count	:	
*******************************************************************************/
#include "KFontD3D.h"
#include "string.h"

//Handle to a vertex shader to KFontVertex vertex format
#define KF_FONTPOLY (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)

/*!*****************************************************************************
// Purpose		:  构造函数
*****************************************************************************/
KFontD3D::KFontD3D()
{
	m_pTexture		= NULL;
	m_pd3dDevice	= NULL;
	m_nFontWidth	= 16;
	m_nFontHeight	= 16;
	m_uOrigStateBlock	  = 0;
	m_uDrawTextStateBlock = 0;
	m_nLastPosH		= 0;
	m_nLastPosV		= 0;
	m_bLoaded		= false;
}

/*!*****************************************************************************
// Purpose		:  析构函数
*****************************************************************************/
KFontD3D::~KFontD3D()
{
	Terminate();
}

/*!*****************************************************************************
// Function		: KFontD3D::Init
// Purpose		: 初始化
// Return		: bool 是否成功
// Argumant		: void* pDrawDevice -> directx 3d device接口的实例的指针
*****************************************************************************/
bool KFontD3D::Init(void* pDrawDevice)
{
	Terminate();
	if ((m_pd3dDevice = (LPDIRECT3DDEVICE8)pDrawDevice) == NULL)
		return false;
	RestoreDeviceObjects();
	return true;
}

/*!*****************************************************************************
// Function		: KFontD3D::Init
// Purpose		: 释放接口实例对象
*****************************************************************************/
void KFontD3D::Release()
{
	delete this;
}

/*!*****************************************************************************
// Function		: KFontD3D::Load
// Purpose		: 载入字库
// Return		: bool 是否成功
// Argumant		: cosnt char *pszFontFile 字库文件名
*****************************************************************************/
bool KFontD3D::Load(const char* pszFontFile)
{
	m_Resources.Terminate();
	m_pTexture = NULL;
	m_bLoaded = false;
	if (m_pd3dDevice)
	{
	//初始化字体字库资源
		if (m_Resources.Init(pszFontFile, m_pd3dDevice))
		{
			m_Resources.GetInfo(m_nFontWidth, m_nFontHeight);
			m_pTexture = m_Resources.GetTexture();
			m_bLoaded = true;
			return true;
		}
	}
	return false;
}

/*!*****************************************************************************
// Function		: KFontD3D::Terminate
// Purpose		: 结束，清除操作
*****************************************************************************/
void KFontD3D::Terminate()
{
	if (m_pd3dDevice)
    {
        if(m_uOrigStateBlock)
			m_pd3dDevice->DeleteStateBlock(m_uOrigStateBlock );			
        if( m_uDrawTextStateBlock )
            m_pd3dDevice->DeleteStateBlock(m_uDrawTextStateBlock);
		m_pd3dDevice = NULL;
    }

	m_uOrigStateBlock = 0;
    m_uDrawTextStateBlock = 0;
	m_bLoaded = false;
	m_pTexture = NULL;
	m_Resources.Terminate();
}

/*!*****************************************************************************
// Function		: KFontD3D::TextOut
// Purpose		: 显示字符串
// Argumant		: cosnt char *pszText 字符串
// Argumant		: int nCount  字符串的长度(BYTE)，默认值为-1，表示此字符串是以'\0'结尾。
// Argumant		: int nX	  字符串显示起点坐标X，如果传入值为KF_FOLLOW，
//							  则此字符串紧接在上次字符串的输出位置之后。
// Argumant		: int nY      字符串显示起点坐标Y, 如果传入值为KF_FOLLOW，
//							  则此字符串与前一次输出字符串在同一行的位置。
// Argumant		: uint nColor 字符串显示颜色，默认为黑色，用32bit数以ARGB的格
//							  式表示颜色，每个分量8bit。
// Argumant     : nLineWidth  自动换行的行宽限制，如果其值小于一个全角字符宽度
//							  则不做自动换行处理。默认值为0，既不做自动换行处理。
*****************************************************************************/
void KFontD3D::TextOut(const char *pszText, int nCount/*= KF_ZERO_END*/,
					   int nX/*= KF_FOLLOW*/, int nY/*= KF_FOLLOW*/,
					unsigned int nColor /*=0xff000000*/, int nLineWidth/*= 0*/)
{
	if (!pszText || !m_bLoaded)
		return;

	unsigned int	nNumChars = 0;
	unsigned char*	lpByte = (unsigned char*) pszText;
	int				nL, h;
	bool			bShowChar = false;
	int				nPos = 0;
	int				nDx = 0;

	//要提交的字符串
	unsigned short	nCommitChars[KCS_CHAR_NUM_LIMIT];
	//用于绘制字符的多边形
	KFontVertex	vCharPolys[KCS_CHAR_NUM_LIMIT * 6];

	if (nCount == KF_ZERO_END)
		nCount = strlen(pszText);
	if (nX == KF_FOLLOW)
		nX = m_nLastPosH;
	if (nY == KF_FOLLOW)
		nY = m_nLastPosV;

	h = 0;
	if (nLineWidth < m_nFontWidth + m_nFontWidth)
		nLineWidth = 0; //不做自动换行处理

	while (nPos < nCount)
	{
		bShowChar = false;

		//*********字符的判断与处理*********
		if (lpByte[nPos] > 0x80 && nPos + 1 < nCount)
		{
			nCommitChars[nNumChars] = *(unsigned short*)(lpByte + nPos);
			nPos += 2;
			bShowChar = true;
			nDx = m_nFontWidth;
		}
		else	//西文字符
		{
			nL = lpByte[nPos++];
			if (nL > 0x20 && nL < 0x5F)
			{
				nCommitChars[nNumChars] = 0xa3 + ((0x20 + nL) << 8);
				bShowChar = true;
				nDx = m_nFontWidth / 2;
			}
			else if (nL >= 0x5F && nL < 0x7F)
			{
				nCommitChars[nNumChars] = 0xa3 + ((0x21 + nL) << 8);
				bShowChar = true;
				nDx = m_nFontWidth / 2;
			}
			else if (nL == 0x0a)
				nDx = -1;
		}
		//遭遇要显示的字符，构造一个四边形，用于后面绘制此字符。
		if (bShowChar)
		{
			int n = nNumChars * 6;
			//设置绘制字符的三角形的信息
			vCharPolys[n].x = (float)(nX + h) - 0.5f;
			vCharPolys[n].y = (float)nY - 0.5f;
			
			vCharPolys[n + 4].x = vCharPolys[n + 1].x = vCharPolys[n].x + m_nFontWidth;
			vCharPolys[n + 4].y = vCharPolys[n + 1].y = vCharPolys[n].y;
		
			vCharPolys[n + 3].x = vCharPolys[n + 2].x = vCharPolys[n].x;
			vCharPolys[n + 3].y = vCharPolys[n + 2].y = vCharPolys[n].y + m_nFontHeight;

			vCharPolys[n + 5].x = vCharPolys[n + 4].x;
			vCharPolys[n + 5].y = vCharPolys[n + 2].y;
			
			for (int i = 0; i < 6; i++)
			{
				vCharPolys[n + i].z = 0.9f;
				vCharPolys[n + i].w = 1.0f;
				vCharPolys[n + i].color = nColor;
			}

			nNumChars ++;

			if (nNumChars >= KCS_CHAR_NUM_LIMIT)
			{
				//向字库资源提交字符串
				m_Resources.CommitText(nCommitChars, nNumChars, vCharPolys);
				//绘制字符串
				RenderText(vCharPolys, nNumChars * 2);
				nNumChars = 0;
			}
		}
		if (nDx == -1 || (nLineWidth && h > nLineWidth))
		{
			if (nPos < nCount && lpByte[nPos] == 0x0a && nDx != -1)	//处理自动换行位置后恰好紧接一个换行符号
				nPos++;
			h = 0;
			nY += m_nFontHeight;
		}
		else
			h+= nDx;
	}

	if (nNumChars)
	{
		//向字库资源提交字符串
		m_Resources.CommitText(nCommitChars, nNumChars, vCharPolys);
		//绘制字符串
		RenderText(vCharPolys, nNumChars * 2);
		nNumChars = 0;
	}
	m_nLastPosH = nX + h;
	m_nLastPosV = nY;
}

/*!*****************************************************************************
// Function		: KFontD3D::RenderText
// Purpose		: 绘制字符串
// Argumant		: KFontVertex* pPolyVertices  用于绘制字符的多边形顶点数据
// Argumant		: unsigned int nNumPolys	  用于绘制字符的多边形数目
*****************************************************************************/
void KFontD3D::RenderText(KFontVertex* pPolyVertices, unsigned int nNumPolys) const
{
	if (m_pd3dDevice && pPolyVertices)
	{	// Set up renderstate
		m_pd3dDevice->CaptureStateBlock(m_uOrigStateBlock);
		m_pd3dDevice->ApplyStateBlock(m_uDrawTextStateBlock);
		m_pd3dDevice->SetVertexShader(KF_FONTPOLY);
	
		//绘制字符多边形
		m_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, nNumPolys,
			pPolyVertices, sizeof(KFontVertex));
	   
		// Restore the modified renderstates
		m_pd3dDevice->ApplyStateBlock(m_uOrigStateBlock);
	}
}

/*!*****************************************************************************
// Function		: KFontD3D::RestoreDeviceObjects
// Purpose		: 使Device状态切换会受影响的绘图设备相关内容失效
*****************************************************************************/
void KFontD3D::InvalidateDeviceObjects()
{
	if(m_pd3dDevice)
	{
		if (m_uOrigStateBlock)
            m_pd3dDevice->DeleteStateBlock(m_uOrigStateBlock);
		if (m_uDrawTextStateBlock)
			m_pd3dDevice->DeleteStateBlock(m_uDrawTextStateBlock);
    }
	m_uOrigStateBlock = 0;
	m_uDrawTextStateBlock = 0;
}

/*!*****************************************************************************
// Function		: KFontD3D::RestoreDeviceObjects
// Purpose		: 在D3d Device由lost state恢复为operational state后KFontD3D作
//					d3d Device相关内容的恢复操作。
*****************************************************************************/
bool KFontD3D::RestoreDeviceObjects()
{
	if (m_bLoaded == false || m_pd3dDevice == NULL)
		return false;
	// Create the state blocks for rendering text
	for (int which = 0; which < 2; which++)
	{
		m_pd3dDevice->BeginStateBlock();
		m_pd3dDevice->SetTexture(0, m_pTexture);
		m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,   D3DBLEND_SRCALPHA );
		m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND,  D3DBLEND_INVSRCALPHA );
		m_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE);
		m_pd3dDevice->SetRenderState( D3DRS_ALPHAREF, 0x08);

		m_pd3dDevice->SetRenderState( D3DRS_ALPHAFUNC,  D3DCMP_GREATEREQUAL );
		m_pd3dDevice->SetRenderState( D3DRS_FILLMODE,   D3DFILL_SOLID );
		m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,   D3DCULL_CCW );
		m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_CLIPPING,         TRUE );
		m_pd3dDevice->SetRenderState( D3DRS_EDGEANTIALIAS,    FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_CLIPPLANEENABLE,  FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_VERTEXBLEND,      FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE );
		m_pd3dDevice->SetRenderState( D3DRS_FOGENABLE,        FALSE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_POINT );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_POINT );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_NONE );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );
		m_pd3dDevice->SetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
		m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
		m_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

        if( which==0 )
            m_pd3dDevice->EndStateBlock(&m_uOrigStateBlock);
        else
            m_pd3dDevice->EndStateBlock(&m_uDrawTextStateBlock);
    }
	return true;
}
