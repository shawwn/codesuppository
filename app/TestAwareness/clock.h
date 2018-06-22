#ifndef CLOCK_H

#define CLOCK_H

/*----------------------------------------------------------------------
		Copyright (c) 2004 Open Dynamics Framework Group
					www.physicstools.org
		All rights reserved.

		Redistribution and use in source and binary forms, with or without modification, are permitted provided
		that the following conditions are met:

		Redistributions of source code must retain the above copyright notice, this list of conditions
		and the following disclaimer.

		Redistributions in binary form must reproduce the above copyright notice,
		this list of conditions and the following disclaimer in the documentation
		and/or other materials provided with the distribution.

		Neither the name of the Open Dynamics Framework Group nor the names of its contributors may
		be used to endorse or promote products derived from this software without specific prior written permission.

		THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND ANY EXPRESS OR IMPLIED WARRANTIES,
		INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
		DISCLAIMED. IN NO EVENT SHALL THE INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
		EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
		LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
		IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
		THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------*/

#include "../../include/common/snippets/UserMemAlloc.h"

namespace CLOCK
{

HeF32 getSystemTime(void);

HeF32       getCurrentTime(void); // current time since startup in seconds.

HeF32        getDeltaTime(void);   // get the delta frame time in seconds. (Times 1000 if you want milliseconds.)

HeF32        doClockFrame(void);   // indicate a new frame, compute and return the delta time value in seconds.
void         getDateTime(char *sdate,char *stime);
const char * getProcessorAscii(void); // get the name of the processor as ascii
HeU64       getClockSpeed(void);     // report the clock speed of the processor.
void         init(void);              // initialize the clock system.
HeU64       getCounter(void); // get the current performance counter.
HeF32        getCounterDifference(HeU64 stime); // compute the difference and return it as a floating point number.


class TimeTracker
{
public:
  TimeTracker(HeU32 frames)
  {
    mTotalFrames = 0;
    mFrameCount = frames;
    mFrameNo    = 0;
    mTimes = MEMALLOC_NEW_ARRAY(float,frames)[frames];
  }

  ~TimeTracker(void)
  {
    delete mTimes;
  }

  void markBegin(void)
  {
    mTime = getCounter();
  }

  void markEnd(void)
  {
    mTimes[mFrameNo] = getCounterDifference(mTime);
    mFrameNo++;
    if ( mFrameNo == mFrameCount )
      mFrameNo = 0;
    mTotalFrames++;
  }

  HeU32  mTotalFrames;  // total number we have tracked altogether.
  HeU32  mFrameCount;   // number of frames we are tracking.
  HeU32  mFrameNo;      // the current frame number.
  HeU64        mTime;         // the start time.
  HeF32        *mTimes;        // the delta frame time for that past N frames.
};

};


#endif
