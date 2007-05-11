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

/** @file time.h
*   @brief The header file for the Time classes and functions
*/

namespace csp
{	
/** 
*	@defgroup time The Time Functions
*
*	These functions deal with converting to and from the csp::Time type.
*	
*	@see csp::Time
*	@see csp::RelTimeoutGuard
*	@see csp::TimeoutGuard
*
*	@{
*/

	/**	
	*	C++CSP2 uses this typedef to represent times for the purposes of timeouts and waits.
	*	On Windows it is a LARGE_INTEGER, for use with the QueryPerformance functions.
	*	On Posix operating systems, Time is a timeval and uses gettimeofday.  The effective
	*	resolution of the timer depends on the underlying machine and operating system, so
	*	C++CSP2 can offer no guarantees about precision.
	*
	*	In particular, these timing mechanisms should not be used for hard real-time applications
	*	where precise timing is critical.  That said, they should work well for most purposes.
	*	After all, neither Windows nor standard Linux are OSes that are intended for hard real-time
	*	applications.
	*
	*	The following binary operators are defined for Time: \verbatim +  -  <  >  <=  >=  ==  != \endverbatim
	*	The following assignment-like operators are defined for Time: \verbatim +=  -= \endverbatim
	*
	*	All of those operators are defined in the global namespace, not in namespace csp
	*/
	
	#ifdef CPPCSP_DOXYGEN
		struct Time {};
	#endif

	#ifdef CPPCSP_WINDOWS
		typedef LARGE_INTEGER Time;
	#endif
	#ifdef CPPCSP_POSIX
		typedef timeval Time;
	#endif
	

	/**
	*	Puts the specified integer amount of microseconds into a Time structure
	*
	*	@param micros The (integer) amount of microseconds (millionths of a second)
	*	@param val The Time structure to put the time into
	*/
	void MicroSeconds(const long micros,Time* const val);

	/**
	*	Puts the specified integer amount of milliseconds into a Time structure
	*
	*	@param millis The (integer) amount of milliseconds (thousandths of a second)
	*	@param val The Time structure to put the time into
	*/
	void MilliSeconds(const long millis,Time* const val);

	/**
	*	Puts the specified floating-point amount of seconds into a Time structure
	*
	*	Note that this function takes a double, not an int like the
	*	other two functions.  This number may be truncated/rounded during the course
	*	of the conversion into the Time structure.
	*
	*	@param secs The (floating-point) amount of seconds
	*	@param val The Time structure to put the time into
	*/
	void Seconds(const double secs,Time* const val);

	/**
	*	Puts the specified integer amount of microseconds into a Time structure
	*
	*	@param micros The (integer) amount of microseconds (millionths of a second)
	*	@return The Time structure with the value in it
	*/
	inline Time MicroSeconds(const long micros)
		{Time t;MicroSeconds(micros,&t);return t;};

	/**
	*	Puts the specified integer amount of milliseconds into a Time structure
	*
	*	@param millis The (integer) amount of milliseconds (thousandths of a second)
	*	@return The Time structure with the value in it
	*/
	inline Time MilliSeconds(const long millis)
		{Time t;MilliSeconds(millis,&t);return t;};

	/**
	*	Puts the specified floating-point amount of seconds into a Time structure
	*
	*	Note that this function takes a double, not an int like the
	*	other two functions.  This number may be truncated/rounded during the course
	*	of the conversion into the Time structure.
	*
	*	@param secs The (floating-point) amount of seconds
	*	@return The time structure with the value in it
	*/
	inline Time Seconds(const double secs)
		{Time t;Seconds(secs,&t);return t;};

	/**
	*	Gets the number of seconds from the specified Time.  
	*
	*	The precision will be based upon the precision of the underlying mechanism as well as the precision of the double type
	*
	*	@param val A pointer to the Time
	*/
	double GetSeconds(const Time& val);	
	
	/**
	*	@overload
	*	Included for compatibility with C++CSP v1.x
	*/
	inline double GetSeconds(const Time* val)
	{
		return GetSeconds(*val);
	}
	
	/**
	*	Gets the number of milliseconds (truncated) in the specified time
	*
	*	@param val A pointer to the Time
	*/
	sign32 GetMilliSeconds(const Time& val);

	/**
	*	Gets the current time.
	*
	*	The value of this time may vary from operating system to operating system.  It
	*	may be the number of seconds since the 1970 epoch, or it may be the number of seconds
	*	since the machine booted up.  No guarantees are offered as to the relevance of
	*	its absolute value.
	*
	*	Instead, it should be used only for relative purposes - working out a specified time
	*	before or since now.	
	*
	*	@param val The place to put the current time
	*/
	void CurrentTime(Time* const val);
	
	/**
	*	Gets the current time.
	*
	*	The value of this time may vary from operating system to operating system.  It
	*	may be the number of seconds since the 1970 epoch, or it may be the number of seconds
	*	since the machine booted up.  No guarantees are offered as to the relevance of
	*	its absolute value.
	*
	*	Instead, it should be used only for relative purposes - working out a specified time
	*	before or since now.	
	*
	*	@return The current time
	*/
	inline Time CurrentTime()
		{Time t;CurrentTime(&t);return t;}

	/**
	*	Makes the current process sleep for the specified amount of time.
	*
	*	There are no guarantees as to exactly how early the process will be scheduled again.  In the mean-time,
	*	other processes in the thread may run (or other threads in the system) so this cannot be known exactly.
	*	However, the call will never return early - timeToSleepFor will always have elapsed.
	*
	*	@post CurrentTime() >= (timeToSleepFor + StartT), where StartT is the result of CurrentTime() when the function is called
	*/
	void SleepFor(const Time& timeToSleepFor);
	
	/**
	*	Makes the current process sleep for the specified amount of time.
	*
	*	There are no guarantees as to exactly how early the process will be scheduled again.  In the mean-time,
	*	other processes in the thread may run (or other threads in the system) so this cannot be known exactly.
	*	However, the call will never return early - it will always be timeToSleepUntil or later.
	*
	*	@post CurrentTime() >= timeToSleepUntil
	*/
	void SleepUntil(const Time& timeToSleepUntil);

/** @} */

}	//namespace csp

#ifdef CPPCSP_WINDOWS
	inline bool operator <(const LARGE_INTEGER& a,const LARGE_INTEGER& b)
		{return a.QuadPart < b.QuadPart;};
	inline bool operator >(const LARGE_INTEGER& a,const LARGE_INTEGER& b)
		{return a.QuadPart > b.QuadPart;};
	inline bool operator <=(const LARGE_INTEGER& a,const LARGE_INTEGER& b)
		{return a.QuadPart <= b.QuadPart;};
	inline bool operator >=(const LARGE_INTEGER& a,const LARGE_INTEGER& b)
		{return a.QuadPart >= b.QuadPart;};		

	inline void operator +=(LARGE_INTEGER& a,const LARGE_INTEGER& b)
		{a.QuadPart += b.QuadPart;};
	inline void operator -=(LARGE_INTEGER& a,const LARGE_INTEGER& b)
		{a.QuadPart -= b.QuadPart;};
	inline bool operator ==(const LARGE_INTEGER& a,const LARGE_INTEGER& b)
		{return a.QuadPart == b.QuadPart;};
	inline bool operator !=(const LARGE_INTEGER& a,const LARGE_INTEGER& b)
		{return a.QuadPart != b.QuadPart;};

	inline LARGE_INTEGER operator+(const LARGE_INTEGER& a,const LARGE_INTEGER& b)
		{LARGE_INTEGER r;r.QuadPart = a.QuadPart + b.QuadPart;return r;};

	inline LARGE_INTEGER operator-(const LARGE_INTEGER& a,const LARGE_INTEGER& b)
		{LARGE_INTEGER r;r.QuadPart = a.QuadPart - b.QuadPart;return r;};				
	
#endif //WIN32

#ifdef CPPCSP_POSIX
//Stolen from the linux glibc header <sys/time.h>:
	#define csp_timercmp(a, b, CMP)      \
		(((a).tv_sec == (b).tv_sec) ?    \
		((a).tv_usec CMP (b).tv_usec) :  \
		((a).tv_sec CMP (b).tv_sec))

	inline bool operator <(const timeval& a,const timeval& b)
		{return csp_timercmp(a,b,<);};
	inline bool operator >(const timeval& a,const timeval& b)
		{return csp_timercmp(a,b,>);};
	inline bool operator <=(const timeval& a,const timeval& b)
		{return csp_timercmp(a,b,<=);};
	inline bool operator >=(const timeval& a,const timeval& b)
		{return csp_timercmp(a,b,>=);};		
	inline bool operator ==(const timeval& a,const timeval& b)
		{return csp_timercmp(a,b,==);};	
	inline bool operator !=(const timeval& a,const timeval& b)
		{return ! ( csp_timercmp(a,b,==) );};	

	#define csp_timeradd(a, b, result)                       \
		do {                                                 \
			(result).tv_sec = (a).tv_sec + (b).tv_sec;       \
			(result).tv_usec = (a).tv_usec + (b).tv_usec;    \
			if ((result).tv_usec >= 1000000)                 \
			{                                                \
				++(result).tv_sec;                           \
				(result).tv_usec -= 1000000;                 \
			}                                                \
		} while (0)

	#define csp_timersub(a, b, result)                       \
		do {                                                 \
			(result).tv_sec = (a).tv_sec - (b).tv_sec;       \
			(result).tv_usec = (a).tv_usec - (b).tv_usec;    \
			if ((result).tv_usec < 0) {                      \
				--(result).tv_sec;                           \
				(result).tv_usec += 1000000;                 \
			}                                                \
		} while (0)

	inline void operator +=(timeval& a,const timeval& b)
		{timeval r;csp_timeradd(a,b,r);a = r;};
	inline void operator -=(timeval& a,const timeval& b)
		{timeval r;csp_timersub(a,b,r);a = r;};
		
	inline timeval operator +(const timeval& a,const timeval& b)
		{timeval r;csp_timeradd(a,b,r);return r;};
	inline timeval operator -(const timeval& a,const timeval& b)
		{timeval r;csp_timersub(a,b,r);return r;};		
	
	#undef csp_timercmp
	#undef csp_timeradd
	#undef csp_timersub

#endif //UNIX

