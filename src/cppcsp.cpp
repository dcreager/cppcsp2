/*
*	The Kent C++CSP Library 
*	Copyright (C) 2002-2007 Neil Brown
*
*	This library is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	This library is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with this library; if not, write to the Free Software
*	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "cppcsp.h"
#include <math.h>

namespace
{
#ifdef CPPCSP_WINDOWS
	double PERF_FREQ,PERF_FREQ_MILLI,PERF_FREQ_MICRO;	

	class SetupTimes
	{
	public:
		inline SetupTimes()
		{
			LARGE_INTEGER lint;
			QueryPerformanceFrequency(&lint);
			PERF_FREQ = static_cast<double>(lint.QuadPart);
			PERF_FREQ_MILLI = PERF_FREQ / 1000.0;
			PERF_FREQ_MICRO = PERF_FREQ / 1000000.0;
		}
	};

	SetupTimes __setupTimes;
#endif

}
	

namespace csp
{	

	void Start_CPPCSP()
	{		
		internal::Kernel* kernel = internal::Kernel::AllocateThreadKernel();
		kernel->data.initialProcess = internal::Process::CreateInitialProcess();
		kernel->data.currentProcess = kernel->data.initialProcess;
		internal::Kernel::KernelData::originalThreadKernelData = &(kernel->data);
		kernel->initNewThread();
	}

	void End_CPPCSP()
	{
		internal::GetKernel()->destroyInThread();
		internal::Kernel::DestroyThreadKernel();
	}
	
	void SleepFor(const Time& time)
	{
		if (time > Seconds(0))
		{
			Time t;
			CurrentTime(&t);
			t += time;
			internal::Kernel* k = internal::GetKernel();
			k->getTimeoutQueue()->addTimeoutNoAlt(k->currentProcess(),&t);
			k->reschedule();
		}
	}
	
	void SleepUntil(const Time& time)
	{
		if (time > CurrentTime())
		{			
			internal::Kernel* k = internal::GetKernel();
			k->getTimeoutQueue()->addTimeoutNoAlt(k->currentProcess(),&time);
			k->reschedule();
		}
	}

} //namespace csp



//From C++CSPv1:

#ifdef CPPCSP_WINDOWS

void csp::CurrentTime(Time* val)
{
	QueryPerformanceCounter(val);
}

void csp::MicroSeconds(long micros,Time* val)
{
	val->QuadPart = static_cast<LONGLONG>(micros * PERF_FREQ_MICRO);
}

void csp::MilliSeconds(long millis,Time* val)
{
	val->QuadPart = static_cast<LONGLONG>(millis * PERF_FREQ_MILLI);
}
void csp::Seconds(double secs,Time* val)
{
	val->QuadPart = static_cast<LONGLONG>(secs * PERF_FREQ);
}

double csp::GetSeconds(const Time& val)
{
	return (static_cast<double>(val.QuadPart) / static_cast<double>(PERF_FREQ));
}

csp::sign32 csp::GetMilliSeconds(const Time& val)
{
	return static_cast<usign32>(GetSeconds(val) * 1000);
}

#else

void csp::CurrentTime(Time* val)
{
	gettimeofday(val,NULL);
}

void csp::MicroSeconds(long micros,Time* val)
{
	if (micros >= 1000000)
	{
		val->tv_sec = micros / 1000000;
		val->tv_usec = micros % 1000000;
	}
	else if (micros < 0)
	{
		val->tv_sec = -((-micros) / 1000000);
		val->tv_usec = -((-micros) % 1000000);
	}
	else	
	{
		val->tv_sec = 0;
		val->tv_usec = micros;
	}
}

void csp::MilliSeconds(long millis,Time* val)
{
	if (millis >= 1000)
	{
		val->tv_sec = millis / 1000;
		val->tv_usec = 1000 * (millis % 1000);
	}
	else if (millis < 0)
	{
		val->tv_sec = -((-millis) / 1000);
		val->tv_usec = -1000 * ((-millis) % 1000);
	}	
	else
	{
		val->tv_sec = 0;
		val->tv_usec = 1000 * millis;
	}
}

void csp::Seconds(double secs,Time* val)
{
	double sec,microsec;
	microsec = modf(secs,&sec);
	val->tv_sec = static_cast<time_t>(sec);
	val->tv_usec = static_cast<suseconds_t>(microsec * 1000000.0);
}

double csp::GetSeconds(const Time& val)
{
	return static_cast<double>(val.tv_sec) + (static_cast<double>(val.tv_usec) / 1000000.0);
}

csp::sign32 csp::GetMilliSeconds(const Time& val)
{
	return (val.tv_sec * 1000) + (val.tv_usec / 1000);
}

#endif

