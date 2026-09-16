// Adobe Photoshop .PSD bitmap file loader
//
//	Copyright (c) 2002, Brad P. Taylor, LLC
//
//	All rights reserved, unauthorized reproduction prohibited
//
//	-- FILE NOTES --
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TPSDFILECODE_H__9D2B4C1A_1D42_4364_A93E_65D59EF0F741__INCLUDED_)
#define AFX_TPSDFILECODE_H__9D2B4C1A_1D42_4364_A93E_65D59EF0F741__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// ----------------------------------------------------------------------------
#pragma warning(disable: 4786)

namespace BPT {

	// ------------------------------------------------------------------------

	//
	//	SPSD_COMMemoryPolicy
	//

	template< class T = void * >
	struct SPSD_COMMemoryPolicy 
	{
		typedef T pointer_type;

		static pointer_type Allocate( const size_t nBytes ) 
		{
			return reinterpret_cast<pointer_type>( ::CoTaskMemAlloc( nBytes ) );
		}

		static void Free( pointer_type ptr ) 
		{
			::CoTaskMemFree( reinterpret_cast<void *>( ptr ) );
		}
	}; /* struct SPSD_COMMemoryPolicy */

	//
	//	SPSD_NewMemoryPolicy
	//

	template< class T = void * >
	struct SPSD_NewMemoryPolicy 
	{
		typedef T pointer_type;

		static pointer_type Allocate( const size_t nBytes ) 
		{
			return reinterpret_cast<pointer_type>( new BYTE [ nBytes ] );
		}

		static void Free( pointer_type ptr ) 
		{
			if ( ptr ) delete [] ptr;
		}
	}; /* struct SPSD_NewMemoryPolicy */

	// ------------------------------------------------------------------------

	//
	//	TFileIO<>
	//

	template< class MEMORY_POLICY = SPSD_NewMemoryPolicy<> >
   	class TFileIO 
	{
	public:
		typedef MEMORY_POLICY memory_policy_type;

   	private:
		MEMORY_POLICY m_MemoryPolicy;
   		FILE * m_Handle;
   	public:
  		TFileIO() : m_Handle( 0 ) { /* Empty */ }
   		TFileIO( MEMORY_POLICY memPolicy ) : 
			m_MemoryPolicy( memPolicy), m_Handle( 0 ) { /* Empty */ }
   		~TFileIO() 
		{
   			if ( m_Handle ) 
   				Close();
   		}

   		bool Open( const char * filename, const char * access ) 
		{
   			m_Handle = fopen( filename, access );
   			return (0 != m_Handle);
   		}
   
   		void Close() 
		{
   			if ( m_Handle ) 
			{
   				fclose( m_Handle );
   				m_Handle = 0;
   			}
   		}
   
   		void Seek( const int pos, const int mode ) 
		{
   			if ( m_Handle ) 
   				fseek( m_Handle, pos, mode );
   		}
   
   		int Read_Byte() 
		{ 
   			if ( m_Handle ) 
   				return static_cast<unsigned>(fgetc( m_Handle )) & 255;
   			return 0;
   		}
   
   		int Read_m16() 
		{
   			int hi = Read_Byte();
   			int lo = Read_Byte();  
   			return ((hi << 8) | lo);
   		}
   
   		int Read_m32() 
		{
   			int a = Read_Byte();
   			int b = Read_Byte();
   			int c = Read_Byte();
   			int d = Read_Byte();
   
   			return (int)((a << 24) | (b << 16) | (c << 8) | d); 
   		}

   		BYTE * LoadData( const int nBytes ) 
		{
   			if ( m_Handle ) 
			{
				MEMORY_POLICY::pointer_type pMemory = m_MemoryPolicy.Allocate( nBytes );
   				if ( 0 == pMemory )
   					return 0;
   				fread( reinterpret_cast<void *>( pMemory ), 1, nBytes, m_Handle );
   				return reinterpret_cast<BYTE *>( pMemory );
   			}

   			return static_cast<BYTE *>( 0 );
   		}

   		void UnloadData( BYTE * ptr ) 
		{
   			if ( ptr ) 
				m_MemoryPolicy.Free( reinterpret_cast<MEMORY_POLICY::pointer_type>( ptr ) );
   		}
   	}; /* template TFileIO<> */

	// (PSD-FILE-LOAD-FUNCTIONS) ----------------------------------------------

	//
	//	TPSDFileLoader<>
	//

	template<
		class CHANNEL_SURFACE
		,class CHANNEL_COLLECTION = std::list< CHANNEL_SURFACE * >
		,class INPUT_POLICY = TFileIO<>
		,class SIZE_COLLECTION = std::vector<int>
	> 
	class TPSDFileLoader 
	{
	public:
		// --------------------------------------------------------------------
		typedef TPSDFileLoader<
			CHANNEL_SURFACE
			,CHANNEL_COLLECTION
			,INPUT_POLICY
			,SIZE_COLLECTION
		> this_type;

		typedef CHANNEL_SURFACE channel_surface_type;
		typedef CHANNEL_COLLECTION channel_collection_type;

	private:
		// --------------------------------------------------------------------
		enum {
			PHOTOSHOP_MAGIC_NUMBER		= 0x38425053
			,RGB_COLOR_MODE				= 3
			,VALID_VERSION_NUMBER		= 1
			,VALID_CHANNEL_BIT_SIZE		= 8
			,COMPRESSION_NONE			= 0
			,COMPRESSION_PACKBITS		= 1
		};
		// --------------------------------------------------------------------

		class Unpack8BitChannel 
		{
		public:
			Unpack8BitChannel() { /* Empty */ }

			template< class SURFACE >
			bool operator()( SURFACE * surface, const BYTE * compressedChannelDataPtr ) 
			{	
				int pixelsToProcess = (surface->Width() * surface->Height());
				int resetCount = surface->Width();
				int pixelsLeft = resetCount;
				int y = 0;
	
				SURFACE::pixel_iterator it = surface->Iterator( 0, y++ );
	
				while ( 0 < pixelsToProcess ) 
				{
					int len = static_cast<int>( *compressedChannelDataPtr++ );
					if ( 128 > len ) { // Literal
						++len;
						pixelsToProcess -= len;
						if ( 0 > pixelsToProcess ) 
						{
							len += pixelsToProcess;
							pixelsToProcess = 0;
						}
	
						while ( 0 < len ) 
						{
							// Process as many pixels in this span as possible
							// ------------------------------------------------
							int count = min( pixelsLeft, len );
							pixelsLeft -= count;
							len -= count;

							while ( 0 <= --count ) 
								*it++ = *compressedChannelDataPtr++;
	
							// If we're at the end of the span move
							// ------------------------------------------------

							if ( (0 == pixelsLeft) && pixelsToProcess ) 
							{
								it = surface->Iterator( 0, y++ );
								pixelsLeft = resetCount;
							}
						}
					} 
					else if ( 128 < len ) 
					{ // Run
						len = (len ^ 0xff) + 2;
						pixelsToProcess -= len;
						if ( 0 > pixelsToProcess ) 
						{
							len += pixelsToProcess;
							pixelsToProcess = 0;
						}
	
						int value = static_cast<int>( *compressedChannelDataPtr++ );
						while ( 0 < len ) 
						{
							// Process as many pixels in this span as possible
							// ------------------------------------------------
							int count = min( pixelsLeft, len );
							pixelsLeft -= count;
							len -= count;
							while ( 0 <= --count ) 
								*it++ = value;
				
							// If we're at the end of the span move
							// ------------------------------------------------
	
							if ( (0 == pixelsLeft) && pixelsToProcess ) 
							{
								it = surface->Iterator( 0, y++ );
								pixelsLeft = resetCount;
							}
						}//end while
					}//end if
				}//end while
				return true;	
			}//end operater()
		}; /* internal class Unpack8BitChannel */

		// --------------------------------------------------------------------
		class ReadRaw8BitChannel 
		{
		public:
			ReadRaw8BitChannel() { /* EMPTY */ }
			template< class SURFACE >
			bool operator()( SURFACE * surface, const BYTE * channelDataPtr ) 
			{
				int cx = surface->Width();
				int cy = surface->Height();
	
				for ( int y = 0; y < cy; y++ ) 
				{
					SURFACE::pixel_iterator it = surface->Iterator( 0, y );
					for ( int x = 0; x < cx; x++ ) 
					{
						*it++ = *channelDataPtr++;
					}
				}
				return true;
			}
		}; /* internal class ReadRaw8BitChannel */

		// --------------------------------------------------------------------

		template< class INPUT, class DECODER >
		bool LoadChannelsHandler(
			INPUT & input, DECODER & decode, SIZE_COLLECTION & dataSizeTable,
			CHANNEL_COLLECTION * pChannelCollection, int loadChannelCount
		) 
		{
   			// Prepare for channel data load loop
   			// ----------------------------------------------------------------
   			CHANNEL_COLLECTION::iterator channelIT = pChannelCollection->begin();
   			SIZE_COLLECTION::iterator sizeIT = dataSizeTable.begin();

   			// Read each channel
   			// -------------------------------------------------------------
   			for ( int channel = 0; channel < loadChannelCount; channel++ ) 
			{
   				// read the entire channel into ram
   				// --------------------------------------------------------
   				int channelDataSize = *sizeIT++;
   				BYTE * data = input.LoadData( channelDataSize );
   				if ( !data ) 
				{
					return false;
   				}

   				// Determine which surface & channel to write data into
   				// --------------------------------------------------------
				decode( *channelIT++, data );

   				// Unload the loaded data
   				// --------------------------------------------------------
   				input.UnloadData( data );
   			}
			return true;
		}
		// --------------------------------------------------------------------
		
	public:

		// --------------------------------------------------------------------
		void Unload( CHANNEL_COLLECTION * pLoadedData ) 
		{
			if ( !pLoadedData ) 
				return;

			CHANNEL_COLLECTION::iterator it;
			for ( it = pLoadedData->begin(); it != pLoadedData->end(); it++ ) 
				delete (*it);

			delete pLoadedData;
		}

		// --------------------------------------------------------------------

		template< class INPUT > CHANNEL_COLLECTION *
		LoadFromAbstraction( INPUT & input, const int nLoadChannelCountMax = 0 ) 
		{
			// Read & Validate the header
			// ----------------------------------------------------------------
			if ( PHOTOSHOP_MAGIC_NUMBER != input.Read_m32() ) 
				return 0;
			if ( VALID_VERSION_NUMBER != input.Read_m16() ) 
				return 0;

			// Skip 6 bytes of pad
			// ----------------------------------------------------------------
			input.Read_m32(); // Pad4 (should be zero)
			input.Read_m16(); // Pad2 (should be zero)

			// Read the channel count
			// ----------------------------------------------------------------
			int channelCount = input.Read_m16();

			// Get the bitmap dimensions
			// ----------------------------------------------------------------
			SIZE imageSize;

			imageSize.cy = input.Read_m32();
			imageSize.cx = input.Read_m32();

			// Read the size of the channel data
			// ----------------------------------------------------------------
			if ( VALID_CHANNEL_BIT_SIZE != input.Read_m16() ) 
				return 0;
			if ( RGB_COLOR_MODE != input.Read_m16() ) 
				return 0;

			// Skip unprocessed data blocks
			// ----------------------------------------------------------------
			int colorModelDataSize = input.Read_m32();
			if ( colorModelDataSize ) 
				input.Seek( colorModelDataSize, SEEK_CUR );

			int imageResourceDataSize = input.Read_m32();
			if ( imageResourceDataSize ) 
				input.Seek( imageResourceDataSize, SEEK_CUR );

			int layerAndMaskInfoDataSize = input.Read_m32();
			if ( layerAndMaskInfoDataSize )
				input.Seek( layerAndMaskInfoDataSize, SEEK_CUR );

			// Finally get down to business and read some data
			// ----------------------------------------------------------------

			int compressionType = input.Read_m16();

			if ( (COMPRESSION_NONE != compressionType) && 
				(COMPRESSION_PACKBITS != compressionType) ) 
				return 0;

			// Create all of the surfaces to store the bitmap data
			// ----------------------------------------------------------------

			int loadChannelCount = channelCount;
			if ( (0 != nLoadChannelCountMax) && (loadChannelCount > nLoadChannelCountMax) ) 
				loadChannelCount = nLoadChannelCountMax;

			CHANNEL_COLLECTION * pChannelCollection = new CHANNEL_COLLECTION;

			if ( !pChannelCollection ) 
				goto HANDLE_ERROR;

			{
				for ( int i = 0; i < loadChannelCount; i++ ) 
				{
					CHANNEL_SURFACE * pChannel = new CHANNEL_SURFACE;
					if ( !pChannel ) 
						goto HANDLE_ERROR;
	
					if ( !pChannel->Create( imageSize.cx, imageSize.cy ) ) 
					{
						delete pChannel;
						goto HANDLE_ERROR;
					}
	
					pChannelCollection->push_back( pChannel );
				}
			}

			// Get down to business and read each channel
			// ----------------------------------------------------------------

			if ( COMPRESSION_NONE == compressionType ) 
			{
				// Build the dataSizeTable
				// ------------------------------------------------------------

				SIZE_COLLECTION dataSizeTable;

				int channelDataSize = imageSize.cx * imageSize.cy;
				for ( int i = 0; i < channelCount; i++ ) 
					dataSizeTable.push_back( channelDataSize );

				// Call the handler
				// ------------------------------------------------------------

				if ( !LoadChannelsHandler(
					input, ReadRaw8BitChannel(), dataSizeTable,
					pChannelCollection, loadChannelCount ) ) 
					goto HANDLE_ERROR;
			} 
			else 
			{
				// Read in the compressed channel data size table
				// Assume reading entire compressed channel as one big chunk
				// -------------------------------------------------------------
				typedef std::vector<int> SizeCollection;

				SizeCollection dataSizeTable;

				for ( int i = 0; i < channelCount; i++ ) 
				{
					int channelDataSize = 0;
					for ( int y = 0; y < imageSize.cy; y++ ) 
					{
						channelDataSize += input.Read_m16();
					}
					dataSizeTable.push_back( channelDataSize );
				}

				// Call the handler
				// ------------------------------------------------------------
				if ( !LoadChannelsHandler(
					input, Unpack8BitChannel(), dataSizeTable,
					pChannelCollection, loadChannelCount ) ) 
					goto HANDLE_ERROR;
			}

			// Finally return our loaded data
			// ----------------------------------------------------------------
			return pChannelCollection;

			// ----------------------------------------------------------------
		HANDLE_ERROR:
			if ( pChannelCollection ) 
				Unload( pChannelCollection );
			return 0;
		}

		// --------------------------------------------------------------------

		CHANNEL_COLLECTION *
		Load( const char * pInputName, const int nLoadChannelCountMax = 0 ) 
		{
			// Open the file return error if not opened.
			// ----------------------------------------------------------------

			INPUT_POLICY input;

			if ( !input.Open( pInputName, "rb" ) ) 
				return 0;

			CHANNEL_COLLECTION * pChannels = LoadFromAbstraction(
				input, nLoadChannelCountMax);

			input.Close();

			return pChannels;
		}

	}; /* template TPSDFileLoader<> */

}; // namespace BPT


#endif // !defined(AFX_TPSDFILECODE_H__9D2B4C1A_1D42_4364_A93E_65D59EF0F741__INCLUDED_)

