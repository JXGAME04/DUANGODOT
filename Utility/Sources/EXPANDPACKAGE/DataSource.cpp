// DataSource.cpp: implementation of the DataSource class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DataSource.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
namespace PackageCQ
{
	DataSource::DataSource()
		:m_CompressMethod(0)
	{
		
	}
	
	DataSource::~DataSource()
	{
		
	}
	
	void DataSource::InsertBlock(const BlockSource & Block )
	{
		try
		{
			m_BlockSet.insert(Block) ;
		
		}
		catch(exception& Error )
		{
			string ErrorString = string("Error when DataSource Insert Block system Info:") +string(Error.what()); 
		    throw  exception(ErrorString.c_str());
		}
		
		
	}
	
	int DataSource::GetBlockCount() const
	{
		return m_BlockSet.size() ;
		
	}
	BlockSource &  DataSource::operator [](size_t Index)
	{
		
		if(Index >= 0 && Index< GetBlockCount())
		{
			set<BlockSource>::iterator ContainerPointer;
			
			ContainerPointer = m_BlockSet.begin();
			
			for(size_t i = 0; i < Index ; i ++)
			{
				ContainerPointer++;
			}

		
			
			return  * ContainerPointer;
			
			
			
		}
		else
		{
			throw exception("DataSource  Index is  overflow!!");
			
		}
		
	}
	
	const DataSource DataSource::operator +(DataSource &Source) const
	{
		if(Source.m_CompressMethod != m_CompressMethod )
		{
			throw exception("Data CompressMethod is Diffrence!!");
			
		}
		
		DataSource Result = * this;
		
		
		int SourceCount = Source.GetBlockCount();
		
		for(int i =0 ;i<SourceCount;i++)
		{
			BlockSource SourceElem = Source[i];
			
			set <BlockSource> :: iterator SourceElemInSelfPosition;
			
			SourceElemInSelfPosition = Result.m_BlockSet.find(SourceElem);
			
			if(SourceElemInSelfPosition == Result.m_BlockSet.end())
			{
				Result.m_BlockSet.insert(SourceElem) ;
				
				
				
			}
			else
			{
				Result.m_BlockSet.erase(SourceElemInSelfPosition) ;
				
				Result.m_BlockSet.insert(SourceElem) ;
				
				
				
			}
			
			
			
			
			
			
			
			
		}
		
		return Result;
		
		
		
		
		
		
		
		
		
	}
	
	const DataSource& DataSource::operator =(const DataSource &Source) 
	{
		m_BlockSet = Source.m_BlockSet ;
		m_CompressMethod = Source.m_CompressMethod; 
		
		return * this;
		
		
	}
	
	DataSource::DataSource(int Compress)
		:m_CompressMethod(Compress)
	{
		
	}
	
	void DataSource::SetCompressMethod(int CompressID)
	{
		m_CompressMethod = CompressID;
		
		
	}
	
	int DataSource::GetCompressMethod()
	{
		return m_CompressMethod;
		
	}
	
	DataSource& DataSource::operator >>(iostream &DataStream)
	{
		for(int i=0;i<GetBlockCount();i++)
		{
			(* this)[i]>>DataStream;
			
			
		}
		
		return * this;
		
	}
}
