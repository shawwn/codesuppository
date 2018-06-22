#include "safestdio.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "UserMemAlloc.h"
#pragma warning(disable:4267)

/*!
**
** Copyright (c) 2007 by John W. Ratcliff mailto:jratcliff@infiniplex.net
**
** Portions of this source has been released with the PhysXViewer application, as well as
** Rocket, CreateDynamics, ODF, and as a number of sample code snippets.
**
** If you find this code useful or you are feeling particularily generous I would
** ask that you please go to http://www.amillionpixels.us and make a donation
** to Troy DeMolay.
**
** DeMolay is a youth group for young men between the ages of 12 and 21.
** It teaches strong moral principles, as well as leadership skills and
** public speaking.  The donations page uses the 'pay for pixels' paradigm
** where, in this case, a pixel is only a single penny.  Donations can be
** made for as small as $4 or as high as a $100 block.  Each person who donates
** will get a link to their own site as well as acknowledgement on the
** donations blog located here http://www.amillionpixels.blogspot.com/
**
** If you wish to contact me you can use the following methods:
**
** Skype Phone: 636-486-4040 (let it ring a long time while it goes through switches)
** Skype ID: jratcliff63367
** Yahoo: jratcliff63367
** AOL: jratcliff1961
** email: jratcliff@infiniplex.net
** Personal website: http://jratcliffscarab.blogspot.com
** Coding Website:   http://codesuppository.blogspot.com
** FundRaising Blog: http://amillionpixels.blogspot.com
** Fundraising site: http://www.amillionpixels.us
** New Temple Site:  http://newtemple.blogspot.com
**
**
** The MIT license:
**
** Permission is hereby granted, MEMALLOC_FREE of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/


//********************************************************************************************
//***
//*** A wrapper interface for standard FILE IO services that provides support to read and
//** write 'files' to and from a buffer in memory.
//***
//********************************************************************************************


#include "FileInterface.h"

#pragma warning(disable:4996) // Disabling stupid .NET deprecated warning.

#define DEFAULT_BUFFER_SIZE 8192
#define BUFFER_GROW_SIZE    1000000 // grow in 1 MB chunks

namespace NVSHARE
{

class MemoryBlock : public Memalloc
{
public:
  MemoryBlock(size_t size)
  {
    mNextBlock = 0;
    mMemory    = (char *)MEMALLOC_MALLOC(size);
    mSize      = size;
    mLen       = 0;
  }

  ~MemoryBlock(void)
  {
    MEMALLOC_FREE(mMemory);
  }

  const char * write(const char *mem,size_t len,size_t &remaining)
  {
    const char *ret = 0;

    if ( (len+mLen) <= mSize )
    {
      char *dest = &mMemory[mLen];
      memcpy(dest,mem,len);
      mLen+=len;
    }
    else
    {
      NxU32 slen = mSize-mLen;
      if ( slen )
      {
        char *dest = &mMemory[mLen];
        memcpy(dest,mem,slen);
        mLen+=slen;
      }
      ret = mem+slen;
      remaining = len-slen;
      assert( remaining != 0 );
    }
    return ret;
  }

  char * getData(char *dest)
  {
    memcpy(dest,mMemory,mLen);
    dest+=mLen;
    return dest;
  }

  MemoryBlock    *mNextBlock;
  char           *mMemory;
  NxU32    mLen;
  NxU32    mSize;

};

class _FILE_INTERFACE : public Memalloc
{
public:
	_FILE_INTERFACE(const char *fname,const char *spec,void *mem,size_t len)
	{
    mHeadBlock = 0;
    mTailBlock = 0;
		mMyAlloc   = false;
		mRead      = true; // default is read access.
		mFph       = 0;
		mData      = (char *) mem;
		mLen       = len;
		mLoc       = 0;

		if ( spec && _stricmp(spec,"wmem") == 0 )
		{
			mRead = false;
			if ( mem == 0 || len == 0 )
			{
        mHeadBlock = MEMALLOC_NEW(MemoryBlock)(DEFAULT_BUFFER_SIZE);
        mTailBlock = mHeadBlock;
				mData = 0;
				mLen  = 0;
				mMyAlloc = true;
			}
		}

		if ( mData == 0 && mHeadBlock == 0 )
		{
			mFph = fopen(fname,spec);
		}

  	strncpy(mName,fname,512);
	}

  ~_FILE_INTERFACE(void)
  {
  	if ( mMyAlloc )
  	{
  		MEMALLOC_FREE(mData);

      MemoryBlock *mb = mHeadBlock;
      while ( mb )
      {
        MemoryBlock *next = mb->mNextBlock;
        delete mb;
        mb = next;
      }

  	}
  	if ( mFph )
  	{
  		fclose(mFph);
  	}
  }

  size_t read(char *data,size_t size)
  {
  	size_t ret = 0;
  	if ( (mLoc+size) <= mLen )
  	{
  		memcpy(data, &mData[mLoc], size );
  		mLoc+=size;
  		ret = 1;
  	}
    return ret;
  }

  void validateLen(void)
  {
    if ( mHeadBlock )
    {
      NxU32 slen = 0;

      MemoryBlock *mb = mHeadBlock;
      while ( mb )
      {
        slen+=mb->mLen;
        mb = mb->mNextBlock;
      }
      assert( slen == mLoc );
    }
  }

  size_t write(const char *data,size_t size)
  {
  	size_t ret = 0;

    if ( mMyAlloc )
    {
#ifdef _DEBUG
      validateLen();
#endif
      size_t remaining;
      data = mTailBlock->write(data,size,remaining);
      while ( data )
      {
        size_t _size = remaining;
        MemoryBlock *block = MEMALLOC_NEW(MemoryBlock)(BUFFER_GROW_SIZE);
        mTailBlock->mNextBlock = block;
        mTailBlock = block;
        data = mTailBlock->write(data,_size,remaining);
      }
      mLoc+=size;
#ifdef _DEBUG
      validateLen();
#endif
      ret = 1;
    }
    else
    {
    	if ( (mLoc+size) <= mLen )
    	{
    		memcpy(&mData[mLoc],data,size);
    		mLoc+=size;
    		ret = 1;
    	}
    }
  	return ret;
  }

	size_t read(void *buffer,size_t size,size_t count)
	{
		size_t ret = 0;
		if ( mFph )
		{
			ret = fread(buffer,size,count,mFph);
		}
		else
		{
			char *data = (char *)buffer;
			for (size_t i=0; i<count; i++)
			{
				if ( (mLoc+size) <= mLen )
				{
					read(data,size);
					data+=size;
					ret++;
				}
				else
				{
					break;
				}
			}
		}
		return ret;
	}

  size_t write(const void *buffer,size_t size,size_t count)
  {
  	size_t ret = 0;

  	if ( mFph )
  	{
  		ret = fwrite(buffer,size,count,mFph);
  	}
  	else
  	{
  		const char *data = (const char *)buffer;
  		for (size_t i=0; i<count; i++)
  		{
    		if ( write(data,size) )
				{
    			data+=size;
    			ret++;
    		}
    		else
    		{
    			break;
    		}
  		}
  	}
  	return ret;
  }

  size_t writeString(const char *str)
  {
  	size_t ret = 0;
  	if ( str )
  	{
  		size_t len = strlen(str);
  		ret = write(str,len, 1 );
  	}
  	return ret;
  }


  size_t  flush(void)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = fflush(mFph);
  	}
  	return ret;
  }


  size_t seek(size_t loc,size_t mode)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = fseek(mFph,loc,mode);
  	}
  	else
  	{
  		if ( mode == SEEK_SET )
  		{
  			if ( loc <= mLen )
  			{
  				mLoc = loc;
  				ret = 1;
  			}
  		}
  		else if ( mode == SEEK_END )
  		{
  			mLoc = mLen;
  		}
  		else
  		{
  			assert(0);
  		}
  	}
  	return ret;
  }

  size_t tell(void)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = ftell(mFph);
  	}
  	else
  	{
  		ret = mLoc;
  	}
  	return ret;
  }

  size_t myputc(char c)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = fputc(c,mFph);
  	}
  	else
  	{
  		ret = write(&c,1);
  	}
  	return ret;
  }

  size_t eof(void)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = feof(mFph);
  	}
  	else
  	{
  		if ( mLoc >= mLen )
  			ret = 1;
  	}
  	return ret;
  }

  size_t  error(void)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = ferror(mFph);
  	}
  	return ret;
  }

  void * getMemBuffer(size_t &outputLength)
  {
    outputLength = mLoc;

    if ( mHeadBlock && mLoc > 0 )
    {
      assert(mData==0);
      mData = (char *)MEMALLOC_MALLOC(mLoc);
      char *dest = mData;
      MemoryBlock *mb = mHeadBlock;
      while ( mb )
      {
        dest = mb->getData(dest);
        MemoryBlock *next = mb->mNextBlock;
        delete mb;
        mb = next;
      }

      mHeadBlock = 0;
      mTailBlock = 0;
    }
    return mData;
  }

  void  myclearerr(void)
  {
    if ( mFph )
    {
      clearerr(mFph);
    }
  }

  FILE 	            *mFph;
  char              *mData;
  size_t             mLen;
  size_t             mLoc;
  bool               mRead;
	char               mName[512];
	bool               mMyAlloc;
  MemoryBlock       *mHeadBlock;
  MemoryBlock       *mTailBlock;

};

FILE_INTERFACE * fi_fopen(const char *fname,const char *spec,void *mem,size_t len)
{
	_FILE_INTERFACE *ret = 0;

	ret = MEMALLOC_NEW(_FILE_INTERFACE)(fname,spec,mem,len);

	if ( mem == 0 && ret->mData == 0 && ret->mHeadBlock == 0 )
  {
  	if ( ret->mFph == 0 )
  	{
      delete ret;
  		ret = 0;
  	}
  }

	return (FILE_INTERFACE *)ret;
}

size_t  fi_fclose(FILE_INTERFACE *_file)
{
  size_t ret = 0;

  if ( _file )
  {
    _FILE_INTERFACE *file = (_FILE_INTERFACE *)_file;
    delete file;
  }
  return ret;
}

void  fi_clearerr(FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
  if ( fph )
  {
    fph->myclearerr();
  }
}

size_t        fi_fread(void *buffer,size_t size,size_t count,FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->read(buffer,size,count);
	}
	return ret;
}

size_t        fi_fwrite(const void *buffer,size_t size,size_t count,FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->write(buffer,size,count);
	}
	return ret;
}

size_t        fi_fprintf(FILE_INTERFACE *_fph,const char *fmt,...)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;

	char buffer[2048];
  buffer[2047] = 0;
	_vsnprintf(buffer,2047, fmt, (char *)(&fmt+1));

	if ( fph )
	{
		ret = fph->writeString(buffer);
	}

	return ret;
}


size_t        fi_fflush(FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->flush();
	}
	return ret;
}


size_t        fi_fseek(FILE_INTERFACE *_fph,size_t loc,size_t mode)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->seek(loc,mode);
	}
	return ret;
}

size_t        fi_ftell(FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->tell();
	}
	return ret;
}

size_t        fi_fputc(char c,FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->myputc(c);
	}
	return ret;
}

size_t        fi_fputs(const char *str,FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->writeString(str);
	}
	return ret;
}

size_t        fi_feof(FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->eof();
	}
	return ret;
}

size_t        fi_ferror(FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->error();
	}
	return ret;
}

void *     fi_getMemBuffer(FILE_INTERFACE *_fph,size_t *outputLength)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	*outputLength = 0;
	void * ret = 0;
	if ( fph && outputLength )
	{
    ret = fph->getMemBuffer(*outputLength);
	}
	return ret;
}

}; // end of namespace

