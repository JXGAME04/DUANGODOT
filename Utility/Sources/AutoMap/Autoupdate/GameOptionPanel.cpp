// GameOptionPanel.cpp : implementation file
//

#include "stdafx.h"
#include "autoupdate.h"
#include "GameOptionPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// GameOptionPanel dialog
const string FullScreenSectorName = "FullScreen";
const string RepresentSectorName  = "Represent";
const string DynaLightSectorName  = "DynamicLight";

const string FullScreenMode    = FullScreenSectorName + "=1";
const string WindowScreenMode  = FullScreenSectorName + "=0";

const string Represent2DMode   = RepresentSectorName + "=2";
const string Represent3DMode   = RepresentSectorName + "=3";

const string DynaLightMode     = DynaLightSectorName + "=1";
const string NotDynaLightMode  = DynaLightSectorName + "=0";

const string ConfileFileName   =  "config.ini";

const string Represent3DModuleFileName ="Represent3.dll";
const string Check_Represent_FunctionName = FN_REPRESENT_IS_MODULE_RECOMMENDED;



const string MessageTitle = "JxoinlineOption";



GameOptionPanel::GameOptionPanel(CWnd* pParent /*=NULL*/)
	: CDialog(GameOptionPanel::IDD, pParent)
{
	//{{AFX_DATA_INIT(GameOptionPanel)
	m_2DOptionValue = -1;
	m_FullScreenValue = -1;
	m_DynaLightEnableValue = FALSE;
	//}}AFX_DATA_INIT
}


void GameOptionPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(GameOptionPanel)
	DDX_Control(pDX, IDC_WindowOption, m_WindowOptionCtl);
	DDX_Control(pDX, IDC_3DOption, m_3DOptionCtl);
	DDX_Control(pDX, IDC_FullScreen, m_FullScreenCtl);
	DDX_Control(pDX, IDC_2DOption, m_2DOptionCtl);
	DDX_Control(pDX, IDC_DynaLightEnable, m_DynaLightEnableCtl);
	DDX_Radio(pDX, IDC_2DOption, m_2DOptionValue);
	DDX_Radio(pDX, IDC_FullScreen, m_FullScreenValue);
	DDX_Check(pDX, IDC_DynaLightEnable, m_DynaLightEnableValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(GameOptionPanel, CDialog)
	//{{AFX_MSG_MAP(GameOptionPanel)
	ON_BN_CLICKED(IDC_OK, OnOk)
	ON_BN_CLICKED(IDC_3DOption, On3DOptionSelected)
	ON_BN_CLICKED(IDC_2DOption, On2DOption)
	ON_BN_CLICKED(IDC_CANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// GameOptionPanel message handlers

void GameOptionPanel::OnOk() 
{
	// TODO: Add your control notification handler code here
	UpdateData();

	try
	{
		
		ifstream ConfigFileIn;
		ConfigFileIn.open(ConfileFileName.c_str());

		if(!ConfigFileIn.is_open())
		{
			throw exception ("打开设置文件失败!");
		}
		
		//通过字段分析器，得到所有选项内容和其他内容
		auto_ptr<ClientGameOptionProcess> ClientGameOptionProcesser (new ClientGameOptionProcess());
		auto_ptr<DefaultProcess> DefaultProcesser (new DefaultProcess());
		RecordProcess InfoProcess;
		
		InfoProcess.AddProcessFunction(ClientGameOptionProcesser.get());
		InfoProcess.AddProcessFunction(DefaultProcesser.get());
		
		InfoProcess.AnalyseRecordStream(ConfigFileIn);

		ConfigFileIn.close();


        TextSet OptionS = ClientGameOptionProcesser->GetOptionS();

		if(find(OptionS.begin(),OptionS.end(),ClientGameOptionID) == OptionS.end())
		{
			throw exception("配置文件损坏:设置文件中没有客户端子段");
		}

		


  

		ofstream ConfigFileOut(ConfileFileName.c_str());
		if(!ConfigFileOut.is_open())
		{
			throw exception ("打开设置文件失败!");
		}
		


		//取得其他选项的内容
		strstream& TextBuffer = DefaultProcesser->GetTextBuffer();
		TextBuffer << ends;
		
		string TextBufferContent(TextBuffer.str());
		ConfigFileOut << TextBufferContent;
       
        //copy(istream_iterator<string>(TextBuffer), istream_iterator<string>(),ostream_iterator<string>(ConfigFileOut,"\n")); 
		
		string OptionIterm;
		TextSet::iterator Pointer;
		//过滤选项,把本程序不能设置的选项放入选项数组中
		for(Pointer = OptionS.begin();Pointer!=OptionS.end();Pointer++)
		{
			OptionIterm = (* Pointer);


			if  (  (OptionIterm.find(FullScreenSectorName) == string::npos) 
				&& (OptionIterm.find(RepresentSectorName) == string::npos)
				&& (OptionIterm.find(DynaLightSectorName) == string::npos)
			    )
				
			{
				NewOptionS.push_back(OptionIterm);

			}

		}

		SetDynaLightOption();
		SetIsFullScreenOption();
		SetRepresentOption();


		copy(NewOptionS.begin(),NewOptionS.end(),ostream_iterator<string>(ConfigFileOut,"\n"));
        
		CDialog::OnOK();
	
		
		
		
		
	}
	catch(exception Error)
	{
		MessageBox(Error.what(),MessageTitle.c_str(),MB_ICONERROR);

	}
	

}

void GameOptionPanel::OnCancel() 
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}

void GameOptionPanel::On3DOptionSelected() 
{
	// TODO: Add your control notification handler code here
	m_DynaLightEnableCtl.EnableWindow();

}

void GameOptionPanel::On2DOption() 
{
	// TODO: Add your control notification handler code here
	m_DynaLightEnableCtl.EnableWindow(FALSE);
	m_DynaLightEnableCtl.SetCheck(0);
	

}

BOOL GameOptionPanel::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

   
	try
	{
		
		HMODULE Represent3DModule  = LoadLibrary(Represent3DModuleFileName.c_str());
		
		
		if(Represent3DModule == NULL)
		{
			string ErrorInfo = string("调入动态库'") +Represent3DModuleFileName + string("'出错");
			throw exception (ErrorInfo.c_str());
			
			
		}
		
		fnRepresentIsModuleRecommended Check_Represent; 
		Check_Represent =(fnRepresentIsModuleRecommended) GetProcAddress(Represent3DModule,Check_Represent_FunctionName.c_str());
		
		if(Check_Represent == NULL)
		{
			FreeLibrary(Represent3DModule);
			string ErrorInfo = string("取得函数地址'") +Check_Represent_FunctionName + string("'出错");
			throw exception (ErrorInfo.c_str());
		}
		
		if((Check_Represent)())
		{
			m_3DOptionCtl.SetCheck(1);
			m_DynaLightEnableCtl.SetCheck(1);
			m_DynaLightEnableCtl.EnableWindow();
			
			
		}
		else
		{
			m_2DOptionCtl.SetCheck(1);
			m_DynaLightEnableCtl.SetCheck(0);
			m_DynaLightEnableCtl.EnableWindow();
		}
		
		m_FullScreenCtl.SetCheck(1);
		
		if(!FreeLibrary(Represent3DModule))
		{
			string ErrorInfo = string("释放模块'") +Represent3DModuleFileName + string("'出错");
			throw exception (ErrorInfo.c_str());
			
			
		}
		
		
		
	}
	catch(exception& Error)
	{
		DisplayErrorInfo(string(Error.what()));
				
		CDialog::OnCancel();
        
	    
		return FALSE;
	
		
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void GameOptionPanel::SetIsFullScreenOption()
{
	switch(m_FullScreenValue)
	{
	case 0:
		{
			NewOptionS.push_back(FullScreenMode);
            break;
		}
	case 1:
		{
			NewOptionS.push_back(WindowScreenMode);
            break;

		}
	}
	



}

void GameOptionPanel::SetRepresentOption()
{

	switch(m_2DOptionValue)
	{
	case 0:
		{
			NewOptionS.push_back(Represent2DMode);
            break;
		}
	case 1:
		{
			NewOptionS.push_back(Represent3DMode);
            break;

		}
		
	

	}
	
	
}

void GameOptionPanel::SetDynaLightOption()
{
	if(m_DynaLightEnableValue)
	{
		NewOptionS.push_back(DynaLightMode);

	}
	else
	{
		NewOptionS.push_back(NotDynaLightMode);

	}


}

