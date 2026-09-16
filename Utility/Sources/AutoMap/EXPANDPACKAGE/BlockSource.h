// BlockSource.h: interface for the BlockSource class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLOCKSOURCE_H__701243D3_7556_46B9_BB6C_9573FD6E142F__INCLUDED_)
#define AFX_BLOCKSOURCE_H__701243D3_7556_46B9_BB6C_9573FD6E142F__INCLUDED_

#include "DataPointFromStream.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include"DataPointFromStream.h"
#define SliceSize 0x10000L
#include <iterator>
using namespace std;
	
namespace PackageCQ
{	
	
	class BlockSource  
	{
	public:
		void CloneBlockSourceToMe(BlockSource& Source);
		BlockSource(const BlockSource& Source);
		BlockSource(DataPointFromStream & Point);
		BlockSource();
		iostream& GetBlockStream() ;
		int GetLength() const;
		int GetID() const;
		BlockSource(int ID,int Length,DataPointFromStream & Point);
		virtual ~BlockSource();
		bool operator ==(const BlockSource &Source) const;
		bool operator > (const BlockSource &Source) const;
		bool operator < (const BlockSource &Source) const;
		const BlockSource& operator <<(iostream &DataStream);
		const BlockSource& operator = (BlockSource &Source);
		
		const BlockSource& operator >>(iostream &DataStream);
		
	private :
		unsigned ComputeBlockSize(int SourceLength,iostream& Point);
		DataPointFromStream  m_BlockPoint;
		unsigned  m_BlockID;
		unsigned  m_BlockLength;
		unsigned  m_UnCompressBlockLength;
		
	};
	
}
#endif // !defined(AFX_BLOCKSOURCE_H__701243D3_7556_46B9_BB6C_9573FD6E142F__INCLUDED_)
