////////////////////////////////////////////////////////////////////////
// AdVantage Terrain SDK, Copyright (C) 2004 - 2008 Filip Strugar.
// 
// Distributed as a FREE PUBLIC part of the SDK source code under the following rules:
// 
// * This code file can be used on 'as-is' basis. Author is not liable for any damages arising from its use.
// * The distribution of a modified version of the software is subject to the following restrictions:
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote the original 
//     software. If you use this software in a product, an acknowledgment in the product documentation would be 
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such and must not be misrepresented as being the original
//     software.
//  3. The license notice must not be removed from any source distributions.
////////////////////////////////////////////////////////////////////////

#ifndef __TiledBitmap_H__
#define __TiledBitmap_H__

#include <stdio.h>

#include <queue>

using namespace std;

namespace AdVantage
{

#ifndef int64
   typedef __int64            int64;
#endif

   enum TiledBitmapLockMode
   {
      TBLM_Read,
      TBLM_Write,
      TBLM_ReadWrite
   };

   enum TiledBitmapPixelFormat
   {
      // Never change existing values when enabling new formats as they are probably used!
      TBPF_Format16BitGrayScale        = 0,
      TBPF_Format8BitGrayScale         = 1,
      TBPF_Format24BitRGB              = 2,
      TBPF_Format32BitARGB             = 3,
      TBPF_Format16BitA4R4G4B4         = 4,
      TBPF_Format64BitD32VY16VX16      = 5,  // Watermap
      // Never change existing values when enabling new formats as they are probably used!
      //TBPF_Format32BitX1R10G10B10,
      //TBPF_Format16BitRGB,
      //TBPF_Format16BitInt,
      //TBPF_Format16BitUInt,
      //TBPF_Format32BitInt,
      //TBPF_Format32BituInt,
      //TBPF_Format32BitFloat,
      //TBPF_Format64BitFloat,
   };

   class TiledBitmapBlock
   {
   private:
      void *                  m_pBuffer;

      TiledBitmapPixelFormat  m_PixelFormat;
      int                     m_BytesPerPixel;
      int                     m_Width;
      int                     m_Height;


   public:
      TiledBitmapBlock(TiledBitmapPixelFormat pixelFormat, int width, int height);
      ~TiledBitmapBlock();

      TiledBitmapPixelFormat     PixelFormat() const     { return m_PixelFormat; }
      int                        BytesPerPixel() const   { return m_BytesPerPixel; }
      int                        Width() const           { return m_Width; }
      int                        Height() const          { return m_Height; }
      void *                     BufferPtr() const       { return m_pBuffer; }

   };

   /// <summary>
   /// BFB is a simple bitmap format that supports unlimited image sizes and
   /// where data is organized into tiles to enable fast reads/writes of random
   /// image sub-regions.
   /// 
   /// Current file format version is 1 (specified in FormatVersion field): supports 
   /// reading and writing of versions 0, 1.
   /// </summary>
   class TiledBitmap
   {
   public:
      static int                    GetPixelFormatBPP( TiledBitmapPixelFormat pixelFormat );

      static const int              c_FormatVersion = 1;
      static const int              c_MemoryLimit = 256 * 1024 * 1024; // allow 256Mb of memory usage
      static const int              c_UserHeaderSize = 224;
      static const int              c_TotalHeaderSize = 256;

   private:
    
      struct DataBlock
      {
         char *              pData;
         unsigned short      Width;
         unsigned short      Height;
         bool                Modified;
      };

      struct DataBlockID
      {
         int                 Bx;
         int                 By;

         DataBlockID( int bx, int by ) { this->Bx = bx; this->By = by; }
      };

      int                           m_UsedMemory;
      queue<DataBlockID>            m_UsedBlocks;

      FILE *                        m_File;

      bool                          m_ReadOnly;

      int                           m_BlockDimBits;
      int                           m_BlocksX;
      int                           m_BlocksY;
      int                           m_EdgeBlockWidth;
      int                           m_EdgeBlockHeight;
      DataBlock **                  m_DataBlocks;

      TiledBitmapPixelFormat        m_PixelFormat;
      int                           m_Width;
      int                           m_Height;
      int                           m_BlockDim;
      int                           m_BytesPerPixel;

   public:
      TiledBitmapPixelFormat        PixelFormat() const     { return m_PixelFormat; }
      int                           BytesPerPixel() const   { return m_BytesPerPixel; }
      int                           Width() const           { return m_Width; }
      int                           Height() const          { return m_Height; }
      //const char * string                    FileName       { get { return file.Name; } }
      bool                          Closed() const          { return m_File == 0; }

      //private readonly byte[] tempBuffer;

   protected:
      TiledBitmap( FILE * file, TiledBitmapPixelFormat pixelFormat, int width, int height, int blockDim, bool readOnly );

   public:
      ~TiledBitmap();

   public:
      static TiledBitmap * Create( const char * pPath, TiledBitmapPixelFormat pixelFormat, int width, int height );
      static TiledBitmap * Open( const char * pPath, bool readOnly );

      void                 Close();

   private:
      void           ReleaseBlock( int bx, int by );
      void           LoadBlock( int bx, int by );
      void           SaveBlock( int bx, int by );
      int64          GetBlockStartPos( int bx, int by );

   public:
      void           GetPixel( int x, int y, void* pPixel );
      void           SetPixel( int x, int y, void* pPixel );
      
      int            ReadHeader(void * pDestBuffer, int size);
      int            WriteHeader(void * pDestBuffer, int size);

      bool           Read( TiledBitmapBlock & destination, int posX, int posY );
      bool           Write( TiledBitmapBlock & source, int posX, int posY );

   };

}


#endif __TiledBitmap_H__
