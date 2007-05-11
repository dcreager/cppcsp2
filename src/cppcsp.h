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

#ifndef ALREADY_INCLUDED_CPPCSP_H
#define ALREADY_INCLUDED_CPPCSP_H

#define INCLUDED_FROM_CPPCSP_H

#ifdef _MSC_VER
	#define CPPCSP_MSVC
#endif

#ifdef __GNUC__
	#define CPPCSP_GCC
#endif


//We don't need the config file with Visual C++:
#ifndef CPPCSP_MSVC
	#include "cppcsp_config.h"
#endif

/**
*	@defgroup defines Pre-Processor Options
*
*	C++CSP2 uses a variety of pre-processor defines, mainly for configuring the library for different architectures.
*
*	Relevant options, and how they are defined, are listed in this section
*
*	@{
*/

/** @def CPPCSP_WINDOWS
*
*	This symbol is defined when C++CSP2 is being compiled on a Microsoft Windows system (2000 or XP).
*
*	It is based on the define WIN32.  If WIN32 is defined, the system is assumed to be Windows, and CPPCSP_WINDOWS is defined.
*	Otherwise, the system is assumed to be POSIX and CPPCSP_POSIX is defined
*/

/** @def CPPCSP_MINGW
*
*	This symbol is defined when C++CSP2 is being compiled on a Microsoft Windows system (2000 or XP) using MinGW/MSYS.
*
*	This symbol is defined by the autotools, and put in cppcsp_config.h.
*/

/** @def CPPCSP_FIBERS
*
*	This symbol is defined when C++CSP2 is using fibers as its implementation mechanism for sub-threads.
*
*	It is based on CPPCSP_WINDOWS.  When CPPCSP_WINDOWS is defined, CPPCSP_FIBERS will also be defined, because
*	fibers are the only sub-thread implementation on Windows.  If CPPCSP_WINDOWS is not defined, neither is CPPCSP_FIBERS
*/

/** @def CPPCSP_X86
*
*	This symbol is defined when C++CSP2 is being compiled on an x86 (or x86-64) compatible machine.
*
*	If CPPCSP_WINDOWS is defined, CPPCSP_X86 is always defined (given that C++CSP2 requires at least Windows 2000).
*	Under POSIX systems, this define is written to the cppcsp_config.h file by the autoconf/automake tools.
*
*	In fact, at the time of writing, CPPCSP_X86 should always be defined in C++CSP2 because they are the only
*	supported processor family.  However, this may not remain true in future.
*/

/** @def CPPCSP_POSIX
*
*	This symbol is defined when C++CSP2 is being compiled on a POSIX-compliant operating system (such as Linux, BSD, etc)
*
*	Not all POSIX Operating Systems are supported!  The current list of known working Operating Systems should be found on 
*	the main page of the documentation.  
*
*	This symbol is defined whenever CPPCSP_WINDOWS is not.  If you find that this symbol is defined on a Windows-based system,
*	you probably forgot to define WIN32.
*/

/** @def CPPCSP_LONGJMP
*
*	This symbol is defined when C++CSP2 is using setjmp/longjmp as its implementation mechanism for sub-threads.
*
*	It is based on CPPCSP_POSIX, as it is the only sub-thread implementation on POSIX.  If CPPCSP_POSIX is defined,
*	CPPCSP_LONGJMP will be defined.
*/	

/** @def CPPCSP_MSVC
*
*	This symbol is defined when C++CSP2 is being compiled using Microsoft Visual C++.  This symbol is based
*	on _MSC_VER, which Microsoft state will be automatically defined with their compiler (http://support.microsoft.com/kb/65472).
*	Other compilers may also define this symbol (check your compiler documentation).  This is not a problem
*	as long as the compiler supports the Microsoft C++ intrinsics for thread-local storage, aligned storage and assembly
*	blocks.
*
*	You must compile your program that uses C++CSP2 with the same compiler that you use to compile C++CSP2,
*	due to the way it uses compiler-specific features.  With Microsoft Visual C++/GCC this will always be the case
*	anyway as their library formats are not compatible.  However, if you compile C++CSP2 with, say, Visual C++ 
*	and then compile your program using, say, the Intel compiler then you may run into problems.  In this case,
*	re-compile the C++CSP2 library with the appropriate compiler.  If you need help porting C++CSP2 to a new
*	compiler I may be able to offer some assistance, so contact me.
*/

/** @def CPPCSP_GCC
*
*	This symbol is defined when C++CSP2 is being compiled using the GNU C++ Compiler.  This symbol is based
*	on __GNUC__, which GNU state will be automatically defined with their compiler (http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html).
*
*	You must compile your program that uses C++CSP2 with the same compiler that you use to compile C++CSP2,
*	due to the way it uses compiler-specific features.  With Microsoft Visual C++/GCC this will always be the case
*	anyway as their library formats are not compatible.  However, if you compile C++CSP2 with, say, GCC 
*	and then compile your program using, say, the Intel compiler then you may run into problems.  In this case,
*	re-compile the C++CSP2 library with the appropriate compiler.  If you need help porting C++CSP2 to a new
*	compiler I may be able to offer some assistance, so contact me.
*/

#ifdef CPPCSP_DOXYGEN
	#define CPPCSP_WINDOWS
	#define CPPCSP_FIBERS		
	#define CPPCSP_X86
	#define CPPCSP_POSIX
	#define CPPCSP_LONGJMP
	#define CPPCSP_MSVC
	#define CPPCSP_GCC
#else

	#ifdef WIN32	
		#define CPPCSP_WINDOWS
		#define CPPCSP_FIBERS
		
		#define CPPCSP_X86
	#else	
		#ifndef CPPCSP_POSIX
			#define CPPCSP_POSIX
		#endif
		#ifndef CPPCSP_LONGJMP
			#define CPPCSP_LONGJMP
		#endif
	#endif

#endif

/** @} */ //end of group



#ifdef CPPCSP_X86
	//TODO later (much later!) finish the version for CPUs without atomics
	#define CPPCSP_ATOMICS
#endif

#ifdef CPPCSP_WINDOWS
	#define _WIN32_WINNT 0x0403

    #include <winsock2.h>    

    #ifndef _WINDOWS_
        #include <windows.h>
        #include <winbase.h>
    #endif //_WINDOWS_

	//Remove those crazy defines (they are functions in std, they should not be macros too!)
	#undef max
	#undef min

#endif //WIN32

#ifdef CPPCSP_POSIX
	#include <pthread.h>
	#include <setjmp.h>
    #include <sys/time.h>
    #include <stdarg.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#ifdef CPPCSP_BSD
    #include <netinet/in_systm.h>
#endif
    #include <netinet/ip.h>

    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET (-1)
    #endif    
    
#endif


#include <boost/lexical_cast.hpp>
#include <boost/integer.hpp>
#include <boost/variant.hpp>
#include <boost/array.hpp>
#include <string>
#include <vector>
#include <list>
#include <map>

/** @class boost::noncopyable
*
*	This class is used as a parent for all classes that cannot be copied.
*
*	You will see that it is used for classes like channels and barriers, where copying them
*	does not make any sense.  If you ever see any errors related to boost::noncopyable it is
*	because you have tried to copy a non-copyable class, and so when the compiler has attempted
*	to generate a copy constructor for that class, it has been unable to do so.
*/

/**
*	Namespace containing all the core C++CSP2 functionality
*
*/
namespace csp
{
	/**@internal
	*	Namespace containing all the private internal workings of C++CSP2
	*/	
	namespace internal
	{
	}
	
	/**
	*	A namespace containing all the commonly used simple processes, such as Id.
	*
	*	These processes are commonly used to plumb/plug/glue together your network.
	*
	*	Many of the processes seem to do similar things to each other, and in fact certain combinations
	*	of processes are equivalent to other arrangements:
	*
	*	- N Id processes joined by unbuffered channels are equivalent to a buffered channel using
	*	a FIFOBuffer of size N.  The FIFOBuffer is far more efficient.
	*	- N ExtId processes joined by unbuffered channels are equivalent to one ExtId process.  Naturally,
	*	using only one process is more efficient.  In fact, one ExtId process is effectively equivalent to
	*	having no process at all -- but it is useful for joining two channels that cannot easily be turned
	*	into one channel.
	*	- An ExtMerger process with unbuffered channels is equivalent to using an Any2OneChannel.
	*	
	*	Putting an ExtId process to connect two channels has no effect on the synchronization.  This can be
	*	used to your advantage.  For example, you may find yourself wanting to use a Merger on two channels, but
	*	the input channels to it are One2AnyChannel, which do not support alting.  If you connect each One2AnyChannel
	*	to an ExtId process, and connect the ExtId processes to the Merger process using One2OneChannel, your problem
	*	will be solved.
	*
	*/
	namespace common
	{
	}	
}

namespace csp
{
    #ifdef CPPCSP_POSIX
        typedef int Socket;
    #endif
    #ifdef CPPCSP_WINDOWS
        typedef SOCKET Socket;
    #endif    

	typedef boost::int_t<16>::least sign16;
	typedef boost::uint_t<16>::least usign16;

	typedef boost::int_t<32>::least sign32;
	typedef boost::uint_t<32>::least usign32;
//This bit causes errors on 32-bit Linux:
/*
#ifdef CPPCSP_MSVC
	typedef DWORD64 usign64;
	typedef __int64 sign64;
#else
	typedef boost::int_t<64>::least sign64;
	typedef boost::uint_t<64>::least usign64;	
#endif	
*/
	typedef boost::array<unsigned char,4> IPv4Address;
    typedef boost::array<unsigned char,16> IPv6Address;
    
    typedef boost::variant<IPv4Address,IPv6Address> IPAddress;
    
    const IPAddress IPAddress_Any;
	const IPAddress IPAddress_Localhost;
    
    typedef IPAddress NetworkInterface;
    
    typedef std::pair<IPAddress,usign16> TCPUDPAddress;    
    
    typedef TCPUDPAddress CPPCSPAddress;
    
    namespace internal
    {
		class DeadlockException {};
		class Process;
    }

	/**
	*	Starts the C++CSP2 run-time.  This function should be called before using any part of C++CSP2
	*
	*	The intention is that you should call Start_CPPCSP() before using any C++CSP2 features, and then call
	*	End_CPPCSP() once you have finished.
	*
	*	If, for some reason, you have to start some threads without using C++CSP2 to do so, you should call
	*	this function (and then End_CPPCSP() later) in each of the threads in which you wish to use a feature of C++CSP2.
	*
	*	@section compat C++CSP v1.x Compatibility
	*
	*	The equivalent function in C++CSP v1.x was named Start_CSP() and had various parameters.  None of those
	*	parameters now apply to C++CSP2, so this function does away with all of them.  There was also a Start_CSP_NET()
	*	function - currently the network functionality has not been implemented in C++CSP2.
	*/	
	void Start_CPPCSP();

	/**
	*	Cleans up the C++CSP2 run-time.  This function should be called as a complement to Start_CPPCSP()
	*
	*	@see Start_CPPCSP()
	*/
	void End_CPPCSP();
	
	/**
	*	Yields the processor to another (kernel-)thread.  The other thread may or may not be
	*	a C++CSP2 thread, so you may yield to a thread from another program.  If you want to
	*	yield to other user-threads in this kernel-thread, use csp::CPPCSP_Yield().	
	*/	
	void Thread_Yield();
	
	/**
	*	Yields to another user-thread in this kernel-thread.  If no other user-threads
	*	are ready to run in the current kernel-thread, the caller continues without yielding.
	*	If you wish to yield to other kernel-threads, use csp::Thread_Yield().
	*/		
	void CPPCSP_Yield();
	//Named this way because Windows #defines Yield() !		
	
//TODO later think about what this type should be (I think it is secretly Kernel* at the moment)
	typedef void* ThreadId;
	
	/**
	*	The base class for errors in C++CSP2 programs
	*
	*	CPPCSPError (and its sub-classes) are thrown when an error occurs in a C++CSP2 that 
	*	represents a mis-use of the library.  These errors are not intended to be caught and ignored,
	*	but instead represent an error in your program that needs to be fixed.
	*/
	class CPPCSPError : public std::exception
	{
	private:
		std::string error;
	public:
		inline explicit CPPCSPError(const std::string& _error)
			:	error(_error)
		{
		}
		
		inline virtual ~CPPCSPError() throw()
		{
		}
		
		virtual const char* what() const throw()
		{
			return error.c_str();
		}
	};
	
	/**
	*	An exception that is thrown whenever C++CSP2 cannot allocate enough resources.
	*
	*	Resources usually means Operating System resources; the most likely cause of this error
	*	is that the OS could not allocate enough threads to run the processes that your program
	*	asked to be run.
	*
	*/
	class OutOfResourcesException : public std::exception
	{
	private:
		std::string error;
	public:
		inline explicit OutOfResourcesException(const std::string& _error)
			:	error(_error)
		{
		}
		
		inline virtual ~OutOfResourcesException() throw()
		{
		}
		
		virtual const char* what() const throw()
		{
			return error.c_str();
		}
	};
	
	/**
	*	An error that is thrown when the Barrier or BarrierEnd classes are mis-used.
	*
	*	The most common causes of this exception are that you tried to call BarrierEnd.sync() without being
	*	enrolled on a barrier, that you destroyed a BarrierEnd while it was still enrolled on the barrier,
	*	or that you destroyed a Barrier while there will still processes enrolled on it.
	*
	*	All of the causes of BarrierError being thrown mean that your program is in error.  They should not be captured
	*	and ignored, but instead your program should be changed to make sure this error will not happen again.  Letting
	*	these errors remain may lead to synchronizations not completing, or your program crashing.
	*/
	class BarrierError : public CPPCSPError
	{
	public:
		inline explicit BarrierError(const char* msg)
			:	CPPCSPError(msg)
		{
		}
		
		inline virtual ~BarrierError() throw()
		{
		}
	};
	
	/**
	*	@defgroup poison Poison
	*
	*	Process-oriented programs typically spawn many processes in parallel, connected by typed channels.  Shutting down
	*	such process networks would be a difficult problem if you tried to send special shutdown messages down the channels;
	*	not only would the type of the channels probably need to be altered, but trying to make sure the shutdown message
	*	travels in the right way around the network (such as to avoid deadlock) is often difficult.
	*
	*	Poison solves this problem.  Channels can be poisoned by either the reading or the writing end of a channel.
	*	Once poisoned, a channel remains poisoned forever; there is no cure for poison.  Any attempt to communicate over
	*	a poisoned channel (an output, an input or an extended input) will result in a PoisonException being thrown.
	*	The pending() and inputGuard() methods of AltChanin will not cause a PoisonException to be thrown.  The guard for
	*	a poisoned channel is always ready.
	*
	*	The expected course of action for a process upon catching a PoisonException is for it to poison all its available
	*	channel-ends and then finish cleanly (closing any files, resigning from any barriers, etc).  In this way, poisoning 
	*	one channel should eventually lead to the entire channel network being poisoned, and all the processes finishing cleanly.
	*
	*	Poisoning unbuffered channels always takes effect immediately, from either end.  Poisoning a buffered channel from
	*	a reading end (Chanin or AltChanin) is always noticed by the writing end (Chanout) immediately.  However, poisoning
	*	by a writing end will not be noticed by the reading end <i>until the buffer is empty</i>.  So if a buffered channel
	*	has three items of data in the buffer, and the writer poisons the channel, the reader will not have a PoisonException
	*	thrown until it has read the three items successfully.  When it attempts the fourth read, the PoisonException will be
	*	thrown.
	*
	*	This buffered-poison behaviour may seem odd at first, but otherwise a writer could write some final items of data to
	*	a channel and then want to poison it to signify that it has finished.  It would not want to poison the channel until
	*	the reader had taken all the items of data, but without an acknowledgement channel or somesuch, it would not know 
	*	when that was.  So instead, under the current scheme, a writer can write the items and immediately poison the channel,
	*	safe in the knowledge that the reader will not perceive the poison until it has emptied the buffer.
	*
	*	More information is available in the @ref tut-channelpoison "Poison" section of the guide.
	*
	* @{
	*/
		
	/**
	*	The poison exception.
	*
	*	This exception is thrown when you try to use a channel that is poisoned.  Unlike derivatives of CPPCSPError,
	*	this exception does not mean your program is in error; in fact, it is usual for it to be thrown at the end of
	*	a program's lifetime.
	*
	*	The use of poison, and therefore of PoisonException, is explained on the @ref poison "Poison" page
	*	and also in the @ref tut-channelpoison "Channel Poison" section of the guard.
	*/
	class PoisonException : public std::exception
	{
	public:
		inline virtual ~PoisonException() throw()
		{
		}
		
		virtual const char* what() const throw()
		{
			return "csp::PoisonException";
		}
	};
	
	/** @} */ //end of group

	class Guard;

}

#include "mobile.h"
#include "time.h"

#ifdef CPPCSP_ATOMICS
	#include "atomic.h"
	#include "atomic_impl.h"
#endif

#include "process.h"
#include "mutex.h"
#include "kernel.h"
#include "barrier.h"
#include "csprocess.h"
#include "run.h"

#include "bucket.h"

#include "alt.h"

#include "channel_base.h"
#include "channel_ends.h"
#include "channel.h"

#include "channel_buffers.h"
#include "buffered_channel.h"
#include "channel_factory.h"

#include "net_channels.h"

namespace csp
{
	/**
	*	Thrown when deadlock occurs in a C++CSP2 program; it is a fatal, unrecoverable error.
	*	If you do catch the error, your only course of action is to try and terminate the program as tidily as possible.
	*	Do not try to continue using C++CSP2, by communicating over channels or starting new processes, as the behaviour
	*	of this will be undefined (this error may be thrown again, for example).
	*
	*	Deadlock occurs when all C++CSP2 processes are blocked with the C++CSP2 system (waiting for channel communication,
	*	barriers or buckets).  It will only occur if your program has a latent error in it that allows deadlock to arise.
	*	If you receive this error, examine the structure of your process network to see how this could have occurred.
	*
	*	This error is always thrown inside the original process that called Start_CPPCSP().  This is for your convenience,
	*	so that when you want to track down the cause of this error, you do not have to catch DeadlockError in all your processes,
	*	just in case that process "encounter" the deadlock (i.e. is the last process to block).  
	*
	*	To aid in this post-mortem examination, you may want to use the recentBlocks variable (and/or the translate() method).
	*
	*	C++CSP2 will only throw a DeadlockError when all processes are blocked on C++CSP2 synchronisation.
	*	For example, if you have two processes in your system, one waiting for a channel communication and one blocked
	*	waiting for user input, that is not deadlock -- because as far as C++CSP2 is concerned, the process waiting for user input
	*	is not blocked.	
	*/
	class DeadlockError : public CPPCSPError, private internal::Primitive
	{
	public:
		/**
		*	A list of the most recent blocks (waits) by processes in your C++CSP2 system.  Processes are identified by pointer/address.
		*	You may encounter duplicates in this list for two reasons:
		*	- Most obviously, the process blocked twice recently.  It may have blocked, been freed, and then blocked again.
		*	- Two processes may have been given the same block of memory to use.  If you are repeatedly starting short-lived
		*	processes, this is a possibility.
		*
		*	The pointers may not still be valid, so do not try to dereference them -- it is quite possible that they point
		*	to a since-deleted process.
		*
		*	These pointers are to an internal class -- you will need to use the translate() method to make sense of them.		
		*/
		const std::list< internal::Process const *> recentBlocks;
		
		inline explicit DeadlockError(const std::list< internal::Process const *>& _recentBlocks)
			:	CPPCSPError("DEADLOCK"),recentBlocks(_recentBlocks)
		{
		}
		
		inline virtual ~DeadlockError() throw()
		{
		}
		
		/**
		*	Translates the recentBlocks list using a dictionary of process names.
		*
		*	You pass to this method a std::map that maps process pointers to names (std::string objects,
		*	which can contain whatever text you wish to use to label the process).  The method then tries
		*	to look-up the name for each recent block.  If the pointer is in the map, the corresponding name
		*	is used.  If the pointer is not in the map, it is simply converted into text (e.g. 0x12345678).
		*
		*	A list is returned that is the same size as recentBlocks, with each entry translated as described above.
		*
		*	@param names The map between process pointers and names.
		*	@return The list of names/pointers of processes.
		*/
		inline std::list< std::string > translate(const std::map<ThreadCSProcess const *, std::string>& names)
		{
			std::list< std::string > r;			
			for (std::list< internal::Process const *>::const_iterator it = recentBlocks.begin();it != recentBlocks.end();it++)
			{
				std::map<ThreadCSProcess const *, std::string>::const_iterator nameIt;
				nameIt = names.find(static_cast<ThreadCSProcess const *>(*it));
			
				if (nameIt == names.end())
				{
					r.push_back(boost::lexical_cast<std::string>(*it));
				}
				else
				{
					r.push_back(nameIt->second);
				}
			}
			return r;
		}
	};
}


#undef INCLUDED_FROM_CPPCSP_H

#endif //ALREADY_INCLUDED_CPPCSP_H


/**
*	@mainpage 
*
*	@section Introduction
*
*	C++CSP2 is a process-oriented programming library for C++, based on ideas derived from CSP (Communicating Sequential Processes).
*	It supports Windows and Linux, on x86 and x86-64 compatible processors.
*
*	To get started with C++CSP2, please read the pages on @ref installation "Installing C++CSP2" and @ref guide1 "The Guide to C++CSP2"
*
*	@section Documentation
*
*	This documentation provides details on the API of C++CSP2 and how to use it.  It is roughly divided into the following sections:
*
*	- @ref processes "Processes" (sub-classes of CSProcess and ThreadCSProcess) are the main units of code in C++CSP2, and this page gives an overview of them.
*	- Processes can be run sequentially or concurrently; the page on @ref run "Running Processes" details how to do this.
*	- Process primarily communicate using @ref channels "Channels", typed one-directional communication pathways between processes.  The multiple types of channels available in C++CSP2 are explained on the @ref channels "Channels" page.
*		- Channels are accessed using @ref channelends "Channel Ends"; how to communicate over channels is described on this page.
*		- Buffered @ref channels "Channels" can used a variety of different buffers, and these are described on the @ref channelbuffers "Channel Buffers" page.
*		- C++CSP2 supports channel @ref poison "Poison", a mechanism for easily shutting down concurrent programs.
*	- @ref time "Time Functions" are available for dealing with time; measuring time, and waiting for specific amounts of time to elapse.
*	- An @link csp::Alternative Alternative @endlink is a powerful construct for expressing choice, that allows you to wait for the first of a specified set of events to occur.
*	- A @link csp::Barrier Barrier @endlink is a synchronization construct that allows an arbitrary number of proceses to synchronize, as well as dynamic enrolling and resigning from the barrier.
*	- A @link csp::Mobile Mobile @endlink is a smart pointer that prevents aliasing.
*
*	There are also some other useful pages available:
*
*	- A list of @ref faq "Frequently Asked Questions" that tries to answer any questions you may have about C++CSP2.  This should be your first port of call if you have a problem with C++CSP2 that you cannot understand.
*	- A @ref glossary "Glossary" for unfamiliar terms.
*	- A page on the @ref defines "Pre-Processor Flags" that configure C++CSP2.
*	- A page on @ref installation "Installing C++CSP2"
*	- Guide pages:
*		- @ref guide1 
*		- @ref guide2 
*		- @ref guide3 
*		- @ref guide4 
*		- @ref guide5 
*
*/

/**
*	@page faq Frequently Asked Questions
*
*	@section Question Categories
*
*	- @ref faqsegfault
*	- @ref faqdesign
*	- @ref faqparallel
*	- @ref faqporting
*	- @ref faqinstall
*	- @ref faqmisc
*	- @ref faqcontact
*
*	@section faqsegfault Segmentation Faults in C++CSP2 Programs
*
 *	<b>Question:</b> When I try to run my application in Windows I get a popup message that reads: 
*	"Unable to find the entry point for procedure CreateFiberEx on the dynamic link library KERNEL32.dll"
*	What is wrong?
*
*	<b>Answer:</b> C++CSP2 requires Windows 2000 (with Service Pack 4) or later to run.  If you are getting this message, it is because
*	you are trying to run a C++CSP2 application on an earlier version of Windows.  If the machine is Windows 2000, install
*	service pack 4.  If it is older than Windows 2000, you will need to upgrade to a more recent versions of Windows.  
*	Windows 2000 itself is seven years old at the time of writing, and Windows 98 and Me are even older (and less stable),
*	so I don't consider this to be an onerous requirement.
*
*	<b>Question:</b> I get weird errors or segmentation faults when running C++CSP2 with an older kernel on Linux.
*	The problem occurs when C++CSP2 performs a context switch.
*
*	<b>Answer:</b> Earlier Linux kernels sometimes had a problem with Pthreads combining with setjmp - Pthreads would
*	store the thread identifier using the current stack (at its base?) and setjmp would change stacks and therefore
*	change this value.  Newer versions of the old LinuxThreads library, and the new Native POSIX Threading Library (NPTL)
*	do not suffer this problem.  I believe this problem can occur under these configurations:
*	- A pre-2.4 Linux kernel
*	- A 2.4 Linux kernel with an old version of glibc
*
*	However, I do not have a machine with old versions of this software to test this out.  Certainly, the problem should
*	not occur under a 2.6 kernel with a recent version of glibc that includes NPTL.  To see if you are using the NPTL,
*	run "getconf GNU_LIBPTHREAD_VERSION".  If you do experience this problem, please contact me.
*
*	<b>Question:</b> I am experiencing strange segmentation faults/general protection faults during my application, 
*	either consistently or just	on certain occasions.  I suspect it is because of C++CSP2 - what could be wrong?
*
*	<b>Answer:</b> There are three categories of common misuses of C++CSP2 that are likely to be causing such problems.
*
*	The first (and simplest) is that you may have forgotten to call Start_CPPCSP() before your C++CSP2 code begins.
*	Similarly, if you have started new threads manually, you will need to place a Start_CPPCSP() call in those threads
*	before you use C++CSP2.  This is done automatically if C++CSP2 starts the thread for you.
*
*	The second possibility is that certain C++CSP2 components, such as the Run functions and the Alternative class, take
*	a list of pointers to objects (to processes, and guards, respectively) that they will delete once they finish.
*	If your program keeps pointers to any of these objects, and either accesses them after they have been deleted,
*	or tries to delete them itself, a segmentation fault will probably result.  You should either stop deleting them
*	yourself, or stop accessing them after you have "handed them over" to the Run/Alternative.  In the case of guards,
*	there is no practical use to accessing them.  In the case of processes, you should use channels to communicate with
*	the barrier rather than accessing the process directly to perform a function call or access data.
*
*	The third possibility is that a process is using communication primitives (or, if you are using shared data, then shared data)
*	that have been deleted/fallen out of scope, usually because of their parent process.  Consider the following code:
*	@code
	{
		ScopedForking forking;
		...
		{
			One2OneChannel<int> c,d;
			forking.fork (new Id<int>(c.reader(),d.writer()));
			...
		}
		... //MARKED SECTION!
	}
	@endcode
*
*	In the marked section, the channels "c" and "d" will have been deleted, as they have fallen out of scope.  However,
*	the Id process may not have finished.  If the Id process tries to communicate on its channels after they have fallen
*	out of scope, it will probably cause a segmentation fault.  
*
*	One common mistake is to do something like the following:
*	@code
	{
		ScopedForking forking;
		...
		{
			One2OneChannel<int> c,d;
			Chanout<int> dIn(d.reader());
			forking.fork (new Id<int>(c.reader(),d.writer()));
			...			
			dIn.poison();
		}
		... 
	}
	@endcode
*
*	The user might assume that because they have poisoned the channel, the Id process will finish, and so the object
*	can fall out of scope safely.  However, C++CSP2 is, of course, concurrent!  The parent process poisons the channel,
*	but Id needs to wake up and detect this poison, and poison its other channels, all before the channel can drop out of 
*	scope.  The only way to be <i>sure</i> that this has happened is to wait for the destruction of the ScopedForking object.
*
*	To avoid such problems, always make sure that the objects a process uses will not fall out of scope before the process
*	will have finished.   A good practice is to declare all the channels before the ScopedForking, so that the ScopedForking
*	will be destroyed (and wait for its children to finish) before the channels are destroyed:
*	@code
	{
		One2OneChannel<int> c,d;
		...
		{
			ScopedForking forking;
			...
			forking.fork (new Id<int>(c.reader(),d.writer()));
			...			
		}
		... 
	}
	@endcode
*
*	There is more discussion on this topic on the page about the @ref scoped "Scoped Objects".
*
*	@section faqdesign Library Design
*
*	<b>Question:</b> Specifying a stack size to the CSProcess/ThreadCSProcess constructor is annoying.  Neither JCSP
*	nor occam requires this, so why does C++CSP2?
*
*	<b>Answer:</b> This problems is not unique to C++CSP2, but it is one of the libraries that exposes this problem
*	to the user if they wish to change it.  As far as I understand it, Java effectively uses a default "large enough" stack size
*	for each thread, which is what C++CSP2 does if you do not specify a stack size.  Occam has the advantage that the compiler
*	can work out the needed stack size, and it does not have dynamic allocation on the stack (no recursion, no local variables
*	of unknown size) so it can use very small stacks that are known to be safe.
*
*	<b>Question:</b> You mention in various parts of the documentation that something has undefined behaviour.  What
*	exactly does this mean?
*
*	<b>Answer:</b> Usually the meaning of undefined behaviour is "your application will crash immediately or will behave
*	strangely and crash eventually".  Any time that something has undefined behaviour, it is because using it in that
*	manner is an error, and usually one that cannot be detected simply or efficiently.  In some circumstances, it may
*	not cause a problem on a particular OS on a particular machine - but that does not mean it will always work, or
*	that the behaviour will work on other systems.  As you might expect, avoid undefined behaviour!
*
*	@section faqparallel Parallelism
*
*	<b>Question:</b> All my procsses seem to stop when I make a blocking system call in one of them, as if the entire program is blocked.  
*	I thought C++CSP2 was supposed to be concurrent and parallel - what gives?
*
*	<b>Answer:</b> This situation can occur if you have started all your processes in the same thread (using RunInThisThread()
*	or csp::ScopedForking::forkInThisThread()).  When one process makes a blocking call, it blocks the whole thread.  To avoid this
*	happening, start your processes in other threads (using Run() or csp::ScopedForking::fork()).
*
*	<b>Question:</b> My program does not seem to be making use of multiple cores, even though I have multiple processes
*	running in parallel.  Why is that?
*
*	<b>Answer:</b> There are two reasons why your program may not be making full use of multiple cores when you expect it to.
*	The first reason is the same answer as the question above; you may have started all your processes in the same thread.
*	This would mean that they would all run in turn on a single core at a time, which would explain why multiple cores
*	were not being used.
*
*	The second reason is that the processes may be interdependent such that they may not make full use of all the cores.
*	Consider a Prefix and an Id process that formed a circuit.  Only one of these processes would be running at once,
*	while the other was forced to wait for a communication.  This sort of tight dependency prevents both processes running
*	all the time.
*
*	<b>Question:</b> I can't create more than around 2,000 or 3,000 processes on a 32-bit machine before I get an OutOfResourcesError thrown.  Why
*	is the limit so low, and how can I increase it?  I want to be able to run many lightweight processes in parallel.
*
*	<b>Answer:</b> By default the stack size of a process is 1 megabyte.  2,000 processes would therefore require nearly 2 gigabytes
*	of memory.  On 32-bit systems, this represents the whole of the memory space available to a Windows program.  On Linux
*	the limit can be 2 gigabytes or 3 gigabytes (depending on your kernel configuration).  The solution is to reduce the 
*	stack size of the process.  Details about stack sizes can be found on the @ref processes "Processes" page.
*
*	<b>Question:</b> You used to have the GNU pth library as an option for user-threading.  Why is this not the case any more?
*
*	<b>Answer:</b> There are a couple of reasons.  The library now uses atomic instructions, and these are only provided
*	for x86 (and x86-64) processors.  On these CPUs, GNU pth is not needed.  If in future, C++CSP2 is again able
*	to work on any processor (by providing alternative algorithms alongside all the atomic-based algorithms) then GNU pth
*	might be worth looking at again - although I believe using GNU pth with Pthreads may be problematic/not possible.  See
*	the next question if you are interested in porting C++CSP2.
*
*	@section faqporting Library Compatibility and Porting
*
*	<b>Question:</b> You state that C++CSP2 works on x86 (and x86-64) compatible processors, on a specified range of versions
*	of Windows and Linux (see the main page for more information on the latest compatibility).  I want to run my application
*	on that processor on another OS (such as Mac OS X, or one of the BSDs) or I want to run my application on a different
*	processor (such as PowerPC).  Can you port it to this system, or can I help you to do so?
*
*	<b>Answer:</b> There are three reasons why I might have not ported C++CSP2 to a particular system, which may apply:
*	- I do not have such a system available to me to perform and test the port.
*	- I do not have enough familiarity with the system to be able to do the port in a reasonably quick amount of time.
*	- C++CSP2's design is at odds with the system, and a port would not be possible.
*
*	Naturally, I often don't know if the third restriction applies until the second has been overcome.  Ports to other
*	OSes on x86 processors should usually be straightforward.  I expect the BSDs to only require the adjustment of a few header
*	includes, and OS X may be just as easy but I would not be as confident.  
*
*	Other CPUs would probably encounter all three restrictions.  On x86 processors on Linux, setjmp and longjmp can be used with a
*	single assembly instruction to change the stack pointer, and all the necessary atomic instructions (such as atomic compare-and-swap)
*	are definitely available.  Other processors do threading differently such that a context switch becomes a difficult affair
*	that would require learning the processor design.  This is why porting to other CPUs is something I do not have the time
*	to do.  If you strongly wish to port to another CPU and know enough about the CPU design I would be happy to try to help,
*	but I do not have the time and/or interest to learn the CPU design myself.
*
*	<b>Question:</b> Is C++CSP2 suitable for use in embedded systems?
*
*	<b>Answer:</b> Two main issues result from considering using C++CSP2 in embedded systems.  
*
*	Firstly, C++CSP2 currently
*	only runs on x86 and x86-64 CPUs.  This means your embedded system will either need to be running such a CPU (which are 
*	relatively rare in an embedded context) or you will need to port C++CSP2 (in which case, see the answer to the above question).
*	
*	Secondly, C++CSP2 uses relatively advanced C++ features.  Some have had trouble in the past compiling it under
*	custom C++ compilers created by hardware manufacturers because not all the advanced templating etc is available.  
*	However, if GCC (or a similar major compiler) can cross-compile to your desired platform then this should not be a problem.
*
*	An ancillary consideration is the memory use of C++CSP2.  Because each process needs its own stack, running many processes
*	requires many stacks, which requires much memory.  Stack sizes can be customised, but you will need to consider whether
*	the lowest stack size you can manage, multiplied by the number of processes you want to run, is less than the memory
*	available to you in your embedded system.
*
*	@section faqinstall Installation
*
*	<b>Question:</b> Boost is very annoying to install, because of its custom build process.   Why do you have it as a
*	dependency, and is there any way to make installing it easier?
*
*	<b>Answer:</b> As a C++ programmer, I find aspects of boost such as lexical_cast, list_of, tuples and many more features
*	very useful.  The build process is annoying, but <i>you do not need to go through with it to compile C++CSP2</i>.  C++CSP2
*	does not use any of the features in the boost compiled libraries - it only needs the boost header files available to it.
*	So just download the latest boost distribution from boost.org, unzip the boost directory in there into somewhere that
*	lies on your include path, and C++CSP2 will compile.
*
*	Having said that, under Linux you may find that boost is available as a package in your package manager.  Be warned that
*	some distributions may have two versions of boost (e.g. boost and boost-devel) where one contains only the compiled
*	libraries and the other has the libraries and header files.  If this is still the case
*	on your distribution then you'll need the development package.
*
*	@section faqmisc Miscellaneous
*
*	<b>Question:</b> In the section DATA_TYPE requirements for a class you say the type "must support a default constructor"
*	or "must support a copy constructor".  I just want to use it with a primitive type, like int or float.  Can I do so?
*
*	<b>Answer:</b> Yes.  Primitive types effectively have a default and copy constructor and support assignment.  Technically
*	it's not a constructor, but it all pans out.  I believe all templated DATA_TYPE objects in this library support
*	all primitive types, unless there is an obvious special requirement (such as DATA_TYPE needing to be an iterator, or channel end).
*
*	@section faqcontact My Question Is Not In Here
*
*	<b>Question:</b> Why is my question not on here?
*
*	<b>Answer:</b> If your question is not answered by the documentation or by this page, then feel free to contact me.  
*	My email address is: neil at twistedsquare dot com.  If you do not receive an answer within a day or two, email me
*	again to make sure it didn't get buried amongst other messages.  
*/

/** @page glossary Glossary
*
*	If you came to this page hoping to find the definition of a term, and did not find it, then let me know
*	so that I can define the term in future versions of this glossary (which is currently quite sparse). You 
*	can find my contact details at the foot of @ref faqcontact "this page".
*
*	- ALT/Alt [noun]: An ALT is shorthand for a csp::Alternative.  
*	- ALTing/Alting [verb]: To ALT is to use a csp::Alternative.  To ALT over some channel inputs is to use a csp::Alternative
*	with the channel input as @link csp::Guard guards @endlink.
*
*	- Blocking [verb]: To block is to wait until some other event occurs.  For example, we say that a reader blocks on
*	an empty channel until a writer arrives, and a process blocks on a barrier until all enrolled processes are ready to synchronise
*	- Blocking (system) calls: These are function calls or operations that may block for an indefinite amount of time.
*	This could be because they require something external - for example, network data or user input.  It may also be
*	because the operation <i>could</i> take a incredibly long time - for example, reading a file.  Because these calls can take such a long
*	time, it is a bad idea to run two processes in the same thread where one of them will be making blocking system calls.
*
*	- Channels: Channels are used to communicate between processes.  They have a specific type, and are used via channel-ends.
*	- Channel-ends: Channel-ends (which may be reading ends, from which data is read, or writing ends to which data is written)
*	are used to actually communicate on channels.  Typically you create a channel and pass the two different ends
*	to different processes ("wiring" them together) so that they can communicate.
*
*	- Empty channel: A channel is deemed empty if there is no process (neither a writer nor a reader) waiting to use it.
*	- FIFO: First-In First-Out.  This is usually used to describe a buffering strategy.  If you feed the value 1 then the value 2 into
*	a FIFO buffer, you will get out the value 1 then the value 2 - that is, the order they come out is the same order
*	that they went in.  Examples of such buffers include csp::FIFOBuffer and csp::InfiniteFIFOBuffer.
*	- Freeing [verb]: Free is usually used as a complement to block.  A reader will block on an empty channel until a writer
*	frees it.
*
*	- Kernel-thread: A kernel-thread is a pre-emptible OS-level thread.  See the @ref run "Running Processes" page.
*
*	- Polling [verb]: To poll is to repeatedly perform a check until an event happens that changes the result.  For
*	example, this code polls a Chanin, "in", until there is a writer ready:
	@code
	while (false == in.pending())
	{
	}
	@endcode
*	Polling is inefficient and can usually be avoided in C++CSP2.  For example the above code could simply read from the channel, which
*	would make it wait until there was a writer ready.  If you need to wait for one of multiple events, a csp::Alternative usually
*	suffices.
*
*	- Release [verb]: Release is usually used to mean the same as free; as a complement to blocking/waiting.
*
*	- Spinning [verb]: Spinning is a form of polling.  It is usually used in terms of mutexes -- if a process cannot
*	immediately claim a mutex it spins (continually re-tries to claim the mutex) until it succeeds.
*
*	- Thread: When used without a qualifier, a thread refers to a kernel-thread (rather than a user-thread)
*
*	- User-thread: A user-thread is a co-operatively scheduled light-weight thread.  See the @ref run "Running Processes" page.
*/

/**	@page installation Installing C++CSP2
*
*	- @ref installlinux 
*	- @ref installwindows
*	- @ref installwindowsrecomp
*	- @ref installmingw 
*
*	@section installlinux Installing on Linux
*
*	@subsection installlinuxboost Boost
*
*	The first step is to install the boost libraries.  There are two easy ways to do this, and one harder way.
*
*	The first easy way is to use your package manager to install boost.  If there are two versions available (often boost
*	and boost-devel), you want the development version (boost-devel).  
*
*	The second easy way is to download the latest boost zip/tar from <a href="http://www.boost.org/">http://www.boost.org/</a>
*	(at the time of writing, follow the Download link on the top right of the page - then on the SourceForge page,
*	select simply "boost").  Inside this zip (at the time of writing) is a versioned boost directory (e.g. boost_1_33_1).
*	Inside that is a boost directory.  Copy this boost directory to somewhere on your include path (e.g. /usr/include).
*	That's finished - C++CSP2 only uses parts of boost that are entirely contained within the header files, so there
*	is no need to compile the full libraries.
*
*	The harder way is to do a proper full install of boost.  To do so, follow the instructions on the boost website.  At
*	the time of writing, the relevant page is <a href="http://www.boost.org/more/getting_started.html">here</a>.
*
*	@subsection installlinuxcppcsp C++CSP2
*	
*	Once boost is installed, download the C++CSP2 tar, and unpack it into a temporary location.  Then perform the usual
*	autotools steps - run "./configure" followed by "make".  This will compile the library (for which you will need, obviously,
*	a C++ compiler such as the GCC C++ compiler).  It should not give any warnings or errors.  If you do receive an error, please contact
*	me so that I can help to resolve it.  Although C++CSP2 compiles correctly on my machine, different versions of
*	Linux or of GCC could cause some warnings or errors.
*
*	After make has completed successfully, it is wise to run "make test".  This will compile and run the C++CSP2 in-built
*	test suite.  You should see a lot of green, and no red!  The summary at the bottom shows how many tests have passed
*	and failed.  None should fail.  If you have any failures, please contact me so that we can resolve the problem.
*	You may also be interested in running "make testperf" which will run the normal tests and a group of performance tests.
*	These tests may take a minute or so to complete, but they may be of interest to you (although currently their meaning
*	is not well-documented).
*
*	After you have tested that the library is working, the command "make install" will then install the files
*	to the appropriate directory (you may want to run that as root or using sudo, to install for all users).  
*
*	Then you will be ready to start writing a C++CSP2 program.  Please read the @ref guide1 "Guide"
*	for a brief guide to getting started.
*
*	@section installboostwindows Installing Boost on Windows
*
*	Whichever method you use to install C++CSP2 on Windows, you will still need to install the Boost libraries first,
*	if you do not already have them on your system.
*
*	The easy way is to download the latest boost zip/tar from <a href="http://www.boost.org/">http://www.boost.org/</a>
*	(at the time of writing, follow the Download link on the top right of the page - then on the SourceForge page,
*	select simply "boost").  Inside this zip (at the time of writing) is a versioned boost directory (e.g. boost_1_33_1).
*	Inside that is a boost directory.  Copy this boost directory to somewhere already on your include path or a new directory
*	that you will add to your include path.  For details on adding a directory to your include path, see the 
*	@ref installwindows "next section".
*	That's finished - C++CSP2 only uses parts of boost that are entirely contained within the header files, so there
*	is no need to compile the full libraries.
*
*	The harder way is to do a proper full install of boost.  To do so, follow the instructions on the boost website.  At
*	the time of writing, the relevant page is <a href="http://www.boost.org/more/getting_started.html">here</a>.  If you
*	are going to use Visual C++ for C++CSP2, you will need to use Visual C++ to compile the boost library, and similarly
*	if you want to use MinGW, you will need to compile boost using MinGW.
*
*	@section installwindows Installing on Windows (Visual C++, using pre-compiled library)
*
*	Using the given pre-compiled libraries is by far the easiest way for Visual C++ users to install
*	C++CSP2.  You will still need to install boost (see the section on @ref installboostwindows "installing boost on Windows".
*	If you download the zip file from the main website, you should find inside the 
*	header files (in the "include" directory) and two .lib files (in the "lib" directory).  
*
*	Copy the cppcsp directory from inside the include directory to somewhere that is in your include path,
*	and put the two lib files (or just one, if you only want x86 or only want x64) somewhere in your library path.
*	If you want to put it somewhere new - for example, you unzip the whole zip to c:\cppcsp2 - then
*	go to (in VS2005):
*	- Click on the Tools menu
*	- Select Options... at the bottom
*	- Expand Projects and Solutions in the left-hand list
*	- Select VC++ Directories
*	- From the platform list (top middle), select "Win32" for 32-bit projects or "x64" for 64-bit projects.  You may
*	want to repeat this procedure for both settings:
*		- Select include files from the "Show directories for" drop-down list in the top-right.
*			- Click the New Line (a folder icon with a star) button.  In the text-box type "c:\cppcsp2\include" (no quotes),
*			or whatever your chosen directory is
*		- Select library files from the "Show directories for" drop-down list in the top-right.
*			- Click the New Line (a folder icon with a star) button.  In the text-box type "c:\cppcsp2\lib" (no quotes),
*			or whatever your chosen directory is
*
*	That is all you need to do to have installed C++CSP2.  In each new project, you will want to include
*	<cppcsp/cppcsp.h> and link with either cppcsp2-x86.lib or cppcsp2-x64.lib, depending on whether you
*	are compiling a 32-bit or 64-bit program.  If you don't know which you are compiling, you are almost
*	certainly compiling a 32-bit program (link with cppcsp2-x86.lib).  To add a library to link with (in VS2005):
*	- Click on the Project menu
*	- Select the bottom item (ProjectName Properties...)
*	- Expand Configuration Properties on the left
*	- Expand the Linker sub-entry on the left
*	- Click on the Input item on the left
*	- In the Additional Dependencies text box, add "cppcsp2-x86.lib" (no quotes).  Items in this box are space-separated.
*
*	@section installwindowsrecomp Installing on Windows (Visual C++, re-compiling the library)
*
*	If you do want to install C++CSP2 by recompiling the code yourself under Visual C++, this section describes
*	how to set up the project.  Installing without re-compiling (see the previous section) is much easier,
*	but if you really want to compile it yourself for some reason, here's how to do it (instructions are for VS2005).
*
*	In the source distribution on the <a href="http://www.cppcsp.net/">C++CSP2 website</a> you should find a 
*	zip with two directories in - "src" and "test".  Unzip that to somewhere where the files can reside 
*	permanently (e.g. "c:\work\cppcsp2").
*
*	In Visual Studio, select File->New->Project...   A window will appear.  On the left hand side, select
*	Visual C++ and then the Win32 sub-item.  On the right, select "Win32 Project".  Name the project
*	as you please (I'm going to assume you name it "C++CSP2") and put in whatever directory you want.
*	You will probably want to create a new solution for it, but it is up to you.  
*
*	Click OK, and a Wizard will appear.  Click Next to get past the first information screen.  Select
*	"Static Library" from the list of options on the next screen, and make sure that "Precompiled header"
*	is NOT ticked.  Then click Finish.  You will now have an empty C++CSP2 project.  Go to the Project menu 
*	and select "Add Existing Item...".  Navigate to the source directory (e.g. "c:\work\cppcsp2\src") and
*	add all the items inside it.  Then "Add Existing Item..." again and select everything inside the src/common
*	directory (e.g. "c:\work\cppcsp2\src\common").
*
*	You should now have a project in the "Solution Explorer" window, containing files 
*	such as run.h, process.h, alt.h, alt.cpp, barrier_bucket.h.  You should not have files like alt_test.cpp or barrier_test.cpp.
*	Go to the Project menu and select "C++CSP2 Properties" (the first word will be the name of your project).  You should 
*	now have a window of properties displayed.  On the left-hand bar, under "Configuration Properties", then 
*	under "C/C++" click on Preprocessor.  Look at the (semi-colon-separated) list of "Preprocessor Definitions".  
*	If you are running on 32-bit Windows XP (if you don't know, then you're almostly certainly running 32-bit, 
*	even on a 64-bit CPU) "WIN32" should be in the list.  If it is not, add it.  If you are running on 64-bit 
*	Windows XP, make sure that <b>both</b> WIN32 and WIN64 are in the list.
*
*	Under "Configuration Properties", "C/C++", "General" you will see "Additional Include Directories".  
*	You should add the boost directory here if you have not set it as a system-wide include directory.  
*	If you look in the boost headers, you will find a "boost" directory with "version.hpp" and many other headers in it.  
*	On my system this is "C:\boost\include\boost-1_33_1\boost".  You do not want to set this directory in your 
*	include path - instead use its parent directory, e.g. "C:\boost\include\boost-1_33_1".  This is because 
*	everything includes "boost/blah.hpp", so it is not the boost directory you want to add, but its parent.
*	Also under "Configuration Properties", "C/C++", "General", set "Detect 64-bit portability
*	warnings" to "No".  There are no portability issues, but Visual Studio spuriously detects some.
*
*	Under "Configuration Properties", "C/C++", "Precompiled Headers", make sure the "Create/Use Precompiled 
*	Headers" option is set to "Not Using Precompiled Headers".  Under "Configuration Properties", "C/C++", 
*	"Code Generation", for "Runtime Library" select "Multi-Threaded DLL" or "Multi-Threaded Debug DLL" (the 
*	former in the Release configuration, the latter in the Debug configuration).
*
*	All the above properties will need to be changed for both the Debug and the Release configurations.  Hopefully, 
*	if you are familiar with Visual Studio you should be familiar with this.
*
*	If you then Build Solution, the C++CSP2 library should build with many warnings but no errors.  If 
*	you get any errors, please email them to me so I can help fix them.  
*
*	Now you should have the C++CSP2 library ready to be used.  A good idea now is to also build the C++CSP 
*	test program, to check that the library is working properly on your machine.  This can be done as follows.
*
*	Once more select "File->New->"Project..."  Select "Win32 Project" again (just as for the first project).  Call the 
*	project something different, such as "C++CSP2-Test" and place the project file where you like.  
*	Select "Add to Solution" (NOT "Create new solution").  Then click OK.
*	On the Wizard that comes up, click Next, then Select "Windows application" and tick "Empty Project".
*
*	Right click on the C++CSP2-Test project in the Solution Explorer and select "Add" then "Existing Item...".
*	Navigate to the test directory (e.g. "c:\work\cppcsp2\test") and add everything in there EXCEPT
*	for the "test_perf.cpp" file.  
*
*	Right click on the C++CSP2-Test project in the Solution Explorer, and select Properties.  Under "Common Properties" select 
*	"References".  Select "Add New Reference..." in the middle of the window.  A list should appear that 
*	only contains your C++CSP2 project.  Select this, and press OK.
*
*	Under "Configuration Properties", "C/C++", "General" you will see "Additional Include Directories".  
*	Fill this in with the directory where your C++CSP2 source files live.  Using the earlier example, this 
*	would be "c:\work\cppcsp2\src".  Also add the directory for the test sources (e.g. "c:\work\cppcsp2\test")
*	You may also need to add the boost libraries to this setting (all entries separated by a semi-colon), 
*	for example "C:\boost\include\boost-1_33_1".
*
*	Just as in the C++CSP2 project: On the left-hand bar, under "Configuration Properties", then under 
*	"C/C++" click on Preprocessor.  Look at the (semi-colon-separated) list of "Preprocessor Definitions".  
*	If you are running on 32-bit Windows XP (if you don't know, then you're almostly certainly running 32-bit, 
*	even on a 64-bit CPU) "WIN32" should be in the list.  If it is not, add it.  If you are running on 64-bit 
*	Windows XP, make sure that both WIN32 and WIN64 are in the list.
*
*	Just as in the C++CSP2 project: Under "Configuration Properties", "C/C++", "Precompiled Headers", make 
*	sure the "Create/Use Precompiled Headers" option is set to "Not Using Precompiled Headers".  Under 
*	"Configuration Properties", "C/C++", "Code Generation", for "Runtime Library" select "Multi-Threaded DLL" 
*	or "Multi-Threaded Debug DLL" (the former in the Release configuration, the latter in the Debug 
*	configuration).
*
*	Finally, look under "Configuration Properties", "Linker", "Advanced".  If you are running 32-bit Windows, 
*	the "Target Machine" setting should be "MachineX86".  If you are running 64-bit Windows, the setting 
*	should be "MachineX64".  Note that the setting is related (as far as I know) to the Windows version you 
*	are using, not to the actual underlying CPU.  
*
*	If you now Build Solution, the C++CSP2-Test (or whatever the name of your project is) EXE should be 
*	built without errors (but with some warnings).  You can then run the EXE (pressing F5 should do it).  
*	The EXE should run invisibly in the background for about 2-3 seconds.  Then a webpage should open up 
*	in your default browser, showing the results.  All the tests should pass, and at the time of writing 
*	there should be 151 of them.  If any fail, or if the program does not finish within about 10 seconds, 
*	something has gone wrong!  Email me the list of failed tests if possible.
*
*	If the compilation fails complaining about a redefined main function, you probably included
*	"test_perf.cpp" in your project by accident.  Remove this file and re-build.
*
*	@section installmingw Installing on Windows using MinGW/MSYS or Cygwin
*
*	Installing using MinGW/MSYS or Cygwin should be the same process as installing under Linux, except
*	that you have less options on how to install boost (no package manager!) - see the @ref installboostwindows section.
*/



/**
*	@page guide1 Guide part 1: Processes, Channels, Poison and Deadlock
*
*	@section tut-boilerplate Boilerplate
*
*	The bare skeleton of a new C++CSP2 program usually looks as follows:
	@code
	#include <cppcsp/cppcsp.h>
	
	using namespace csp;
	
	int WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow) //<-- For Windows
	int main(int argc,char** argv) //<-- For everything else
	{
		Start_CPPCSP();
		
		...
		
		End_CPPCSP();
		return 0;
	}
	@endcode
*
*	- Only one of those two function headers is needed - the top one for Windows, the second for all other systems.
*	- The Start_CPPCSP() and End_CPPCSP() calls are always required.  
*	- The "using namespace csp;" is optional, but it stops you having to prefix everything with "csp::".  
*	- The "cppcsp2/cppcsp.h" header file includes all the parts of the C++CSP2 library except the common processes, which can be found (currently) in "cppcsp2/common/basic.h"
*	and "cppcsp2/common/barrier.h".  
*
*	It is assumed that all C++CSP2 function calls (such as SleepFor, and Run) take place after Start_CPPCSP() has been
*	called, and before End_CPPCSP() is called.
*
*	In the rest of this guide, it is assumed that "using namespace csp;" has already been declared.
*
*	To compile your programs, link with the C++CSP2 library.  On GCC this can be achieved by using the "-lcppcsp2" option
*	on the command-line.  
*
*	@section tut-processes Processes
*
*	Process are the central concept of C++CSP2.  Processes are self-contained active pieces of code.
*	Various analogies can be attempted (e.g. processes are active objects, processes are like threads), but it is perhaps
*	best illustrated by example.  A process in C++CSP2 is typically a subclass of csp::CSProcess:
	@code	
	class WaitProcess : public CSProcess
	{
	private:
		Time t;
	protected:
		void run()
		{
			SleepFor(t);
		}
	public:
		WaitProcess(const Time& _t) : t(_t)
		{
		}
	};
	@endcode
*	- The process inherits from csp::CSProcess
*	- csp::CSProcess has a default constructor that can be used
*	- The protected void csp::CSProcess::run() function contains the process's code/functionality.
*	- csp::Time and csp::SleepFor are part of the C++CSP2 library.  csp::Time is a datatype containing a time (either an
*	absolute time, or a length of time), and csp::SleepFor waits for a specified length of time.
*
*	It should be obvious that the behaviour of this process is to wait for a specified amount of time.  A process
*	on its own is merely a declaration -- it needs to be run.  Running one process is straightforward:
*
	@code
	Time t = Seconds(3);
	Run(new WaitProcess(t));
	@endcode
*	- csp::Seconds is a C++CSP2 function that returns a csp::Time that has a value of the specified amount of seconds.
*	- The process is allocated on the heap using new.  C++CSP2 takes care of deleting the process once it has finished
*	running.  <b>Never</b> delete a process yourself, and <b>never</b> allocate a process on the stack.  Both of these
*	actions will lead to memory corruption.
*	- The csp::Run call will not return until the process has finished running.
*
*	The overall effect of running the process will be to wait for three seconds.  
*
*	Running a single process is of limited use.  Processes can be composed -- either sequentially or in parallel.  
*	Sequential and parallel composition are just as easy as each other:
	@code
	Run(InSequence
		( new WaitProcess(Seconds(3)) )
		( new WaitProcess(Seconds(3)) )
	);
	
	Run(InParallel
		( new WaitProcess(Seconds(3)) )
		( new WaitProcess(Seconds(3)) )
	);
	@endcode
*	- The syntax is similar to that of <a href="http://www.boost.org/libs/assign/doc/index.html#list_of">boost::assign::list_of</a>.
*	The csp::InSequence and csp::InParallel markers are followed by lists where each item is individually bracketed.
*
*	The effect of the first csp::Run call is to wait for six seconds (wait for three seconds, then for another three).  The
*	effect of the second is to wait for three seconds (wait for three seconds in parallel with an identical wait).
*
*	As you would expect, when processes are run in sequence, each process is only started when the process before it
*	finishes.  When processes are run in parallel, all the processes are started at once, and the csp::Run call only
*	returns when <i>all</i> the processes have finished.
*
*	These sequential/parallel compositions can be composed together, to any depth:
*	@code
	Run(InSequence
		(InParallel
			( new WaitProcess(Seconds(1)) )
			( new WaitProcess(Seconds(2)) )
		)
		( new WaitProcess(Seconds(3)) )
		(InParallel
			( new WaitProcess(Seconds(4)) )
			(InSequence
				( new WaitProcess(Seconds(5)) )
				( new WaitProcess(Seconds(6)) )
			)
		)
	);
	@endcode
*	- Hopefully you should be able to work out the overall duration of this csp::Run call: 16 seconds
*
*	@section tut-channels Channels
*	
*	So far, the processes we have been using worked in isolation.  This is quite rare -- usually the processes
*	will need to interact.  The most common need is to pass data between processes.  The design philosophy behind
*	CSP systems is that processes should not directly share data.  This can lead to problems when two parallel processes
*	both try to change data at the same time.  Instead, no data should be shared, and channels should be used to pass
*	data between processes.
*
*	Imagine that we had a simple process that continually writes increasing integers to the screen:
	@code
	class IncreasingNumberPrinter : public CSProcess
	{
	protected:
		void run()
		{
			for (int i = 1; ;i++)
			{
				std::cout << i << std::endl;
			}
		}
	};
	@endcode
	
*	Although it seems trivial (most simple examples are...), we could split this process in two; one that prints
*	numbers to the screen, and another that produces increasing numbers.  The two processes could then be connected
*	by a channel.  This is how we would write the number-producing process:
	@code
	class IncreasingNumbers : public CSProcess
	{
	private:
		Chanout<int> out;
	protected:
		void run()
		{
			for (int i = 1; ;i++)
			{
				out << i;
			}
		}
	public:
		IncreasingNumbers(const Chanout<int>& _out)
			:	out(_out)
		{
		}
	};
	@endcode
*	- csp::Chanout is the writing-end of a channel.  You write data to a channel using a csp::Chanout -- you read
*	data from a channel using a csp::Chanin.  This may seem counter-intuitive to some, but remember that we program
*	in a process-oriented fashion -- from the process's perspective, a Chanout is the output, whereas a Chanin is an
*	input.
*	- csp::Chanout has a single template parameter, which is the type that it carries.  In this case, it is a channel
*	of type "int".
*	- The channel is written to by using the operator "<<", just like streams.  This is by far the easiest and
*	clearest method of writing to channels.  See csp::Chanout::write for another way.
*	- The csp::Chanout is passed to the constructor as a const reference.  This is mainly for efficiency;
*	it could also have been safely been written as simply: "IncreasingNumbers(Chanout<int> _out)".  Pick whichever
*	approach you prefer.
*
*	The above process will produce a stream of increasing integers on its output channel.  Next we need its companion
*	process:
	@code
	class NumberPrinter : public CSProcess
	{
	private:
		Chanin<int> in;
	protected:
		void run()
		{
			while (true)
			{
				int n;
				in >> n;
				std::cout << n << std::endl;
			}
		}
	public:
		NumberPrinter(const Chanin<int>& _in)
			:	in(_in)
		{
		}
	};
	@endcode
*	- csp::Chanin is used in a corresponding way to csp::Chanout.  The >> operator is used for input.
*
*	Now that we have these two processes, we need to run them both, and "plumb" the two channel-ends together
*	using an actual channel:
	@code
	One2OneChannel<int> c;
	Run(InParallel
		( new IncreasingNumbers( c.writer() ) )
		( new NumberPrinter( c.reader() ) )
	);
	@endcode
*	- The actual channel is csp::One2OneChannel. It has a template parameter that matches the channel-ends.  It will
*	not be clear yet why it is a <i>One2One</i>Channel.  Simply, there is one process writing to it, and one process
*	reading from it.  Channels can also be shared (which we will see later on).
*	- The writing-end of the channel is returned from the csp::One2OneChannel::writer() method, and similarly
*	the reading-end is returned from csp::One2OneChannel::reader().
*
*	These processes will have the same effect as our original IncreasingNumberPrinter process.  But now we can change
*	the behaviour of the system <i>without</i> changing our new IncreasingNumbers and NumberPrinter processes.
*	Imagine that we now wanted to print out all the <a href="http://en.wikipedia.org/wiki/Triangular_numbers">triangular numbers</a>.
*	We could use this process:
	@code
	class Accumulator : public CSProcess
	{
	private:
		Chanin<int> in;
		Chanout<int> out;
	protected:
		void run()
		{
			int total = 0;			
			while (true)
			{
				int n;
				in >> n;
				total += n;
				out << total;
			}
		}
	public:
		Accumulator(const Chanin<int>& _in,const Chanout<int>& _out)
			:	in(_in),out(_out)
		{
		}
	};
	@endcode
*
*	This process takes in numbers, and sends out the running total.  If this is combined with our earlier two processes,
*	it will start sending out the triangular numbers:
	@code
	One2OneChannel<int> sequentialNumbers;
	One2OneChannel<int> triangularNumbers;
	Run(InParallel
		( new IncreasingNumbers( sequentialNumbers.writer() ) )
		( new Accumulator( sequentialNumbers.reader(), triangularNumbers.writer() ) )
		( new NumberPrinter( triangularNumbers.reader() ) )
	);
	@endcode
*
*	This illustrates -- in a very small way -- the power of processes.  Their channels are their interface in much the
*	same way that a function list represents the interface of a class (assuming all data is private).  The processes
*	here do not need to know what is at the other end of the channel; they just read and write numbers to their channel-ends,
*	and then we have composed these processes together into a meaningful chain.
*
*	@section tut-channelpoison Poison
*
*	You may have noticed that our simple processes run forever.  This is only acceptable for a small subset of programs;
*	most programs are intended to run for a set duration and then finish.  One way to achieve this would be to 
*	fix a duration in each process.  They could each take a parameter to their constructor to specify how many numbers
*	they should produce/process before finishing.  This method would be annoying to program and would not scale well.
*	If we later decided that to add a filtering process that masked out the odd triangular numbers, we'd have to change
*	various limits differently.  There are many other settings where running for a set number of iterations would not
*	be applicable (for example, processing GUI events).  
*
*	Sending a special "terminator" value would be another solution, but that does not always fit either.  If a process
*	at the end of a chain (in the above case, NumberPrinter) wants to tell the others to quit, it cannot (without extra,
*	ugly "quit" channels being added in the opposite direction).
*
*	C++CSP2 therefore has the notion of poisoning channels.  Once a channel is poisoned, it forever remains so (there is
*	no antidote!).  Poisoning can be done by either the reader or writer (poison can flow upstream).  If an attempt
*	is made to use a poisoned channel, a csp::PoisonException will be thrown.  The processes can be rewritten to use
*	poison as follows:
	@code
	class Accumulator : public CSProcess
	{
	private:
		Chanin<int> in;
		Chanout<int> out;
	protected:
		void run()
		{
			try
			{
				int total = 0;
				while (true)
				{
					int n;
					in >> n;
					total += n;
					out << total;
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out.poison();
			}
		}
	public:
		Accumulator(const Chanin<int>& _in,const Chanout<int>& _out)
			:	in(_in),out(_out)
		{
		}
	};
	@endcode
*
*	The easiest place to put the catch statement for poison is at the outermost layer of the run() function.  The
*	usual way to handle poison is to poison all of the process's channels and then finish (by returning from the
*	run() function).  Poisoning an already-poisoned channel has no effect (and will never result in a second
*	PoisonException being thrown) so regardless of whether it was "in" or "out" that was poisoned, both of them
*	are poisoned and then the process finishes.
*
*	The initial poison is spread using the poison() method on a csp::Chanin or csp::Chanout.  Poison naturally spreads
*	throughout a process network along the channels, which will usually easily cause all processes to finish.
*
*	@section tut-deadlock Deadlock
*
*	Deadlock is literally when there are no C++CSP2 processes to run.  This usually occurs because processes are all
*	waiting for each other to do perform different actions.  For example, consider two processes that you want to send
*	numbers back and forth between them:
	@code
	class NumberSwapper : public CSProcess
	{
	private:
		csp::Chanin<int> in;
		csp::Chanout<int> out;
	protected:
		void run()
		{
			int n = 0;
			while (true)
			{
				out << n;
				in >> n;
			} 
		}
	public:
		NumberSwapper(const Chanin<int>& _in,const Chanout<int>& _out)
			:	in(_in),out(_out)
		{
		}
	};
	@endcode
*
*	Once you have written this process, you connect two of them together:
	@code
	One2OneChannel<int> c,d;
	Run(InParallel(
		( new NumberSwapper( c.writer(), d.reader() ) )
		( new NumberSwapper( d.writer(), c.reader() ) )
	); //DEADLOCK!
	@endcode 
*
*	The outcome is deadlock.  Both processes start by trying to write on their channel.  The process on the other side
*	is also trying to write, so neither of them read a value, and hence both processes are forever stuck.  With
*	no processes left that can do anything, we are in a state of deadlock.
*
*	In the above scenario, this could be solved by making one process write-then-read, and the other read-then-write.
*	In other cases the cause may be subtler than this (it may also involve barriers, which will be introduced later on).
*	
*	When deadlock is encountered, a csp::DeadlockError is thrown in the original process (which will usually be in the
*	main() function), that first called csp::Start_CPPCSP().  This is not an exception to be dealt with casually -- deadlock is a terminal program error in
*	much the same way as a memory access violation, and it represents a latent error in your program.  You may handle
*	it to perform emergency clear-up, but it indicates that the design of your program is broken.
*
*	Whenever you use barriers, or build a process network that contains a cycle of channels, consider whether deadlock
*	could occur. 
*
*	This guide is continued in @ref guide2.	
*
*/

/**
*	@page guide2 Guide part 2: Alternatives and Extended Input
*
*	@section tut-alt Alternatives
*
*	In @ref guide1, we saw how to use channels for input and output.  There are often occasions when you will
*	find that you want to wait for one of two (or more) channels.  For example, you may have a process that needs to
*	wait for input on either a data channel or a quit-signal channel.  This can be achieved using a csp::Alternative:
	@code
	class DataOrQuit : public CSProcess
	{
	private:
		AltChanin<Data> dataIn;
		AltChanin<QuitSignal> quitIn;
	protected:
		void run()
		{
			try
			{
				list<Guard*> guards;
				guards.push_back(quitIn.inputGuard());
				guards.push_back(dataIn.inputGuard());
				Alternative alt(guards);
							
				QuitSignal qs;
				Data data;
							
				while (true)
				{
					switch (alt.priSelect())
					{
						case 0:													
							quitIn >> qs;
							dataIn.poison();
							quitIn.poison();
							return;
						case 1:
							dataIn >> data;
							// ... Handle data ...
							break;							
					}
				}
			}
			catch (PoisonException&)
			{
				quitIn.poison();
				dataIn.poison();
			}
		}
	public:
		DataOrQuit(const AltChanin<Data>& _dataIn,const AltChanin<QuitSignal>& _quitIn)
			:	dataIn(_dataIn),quitIn(_quitIn)
		{
		}
	};
	@endcode
*	- Because we need to use an csp::Alternative (alting for short) on the two channel-ends, we specify the channel-type
*	csp::AltChanin rather than csp::Chanin.  As you would expect, csp::AltChanin allows use with csp::Alternative,
*	whereas csp::Chanin does not.  As explained in @ref guide3, not all channels support alting.
*	- The csp::AltChanin::inputGuard() method is used to get a pointer to a csp::Guard.  This pointer is put into a list
*	which is then given to the csp::Alternative.  Just like the process pointers given to csp::Run, the csp::Guard pointers
*	will be deleted by the csp::Alternative so there is no need to store the pointer yourself and you should not try
*	to delete the guards yourself.  Nor should you ever allocate a csp::Guard on the stack.
*	- The csp::Alternative object can be re-used repeatedly.  This is the most efficient thing to do, to avoid continual
*	re-initialisation of a csp::Alternative object.  As we'll see shortly, it is in fact needed in some regards.
*	- The csp::Alternative::priSelect() method waits for <i>one</i> of the guards to be "ready" (in the case of inpu
*	guards, for a writer to be ready to write to the channel).  It returns the index (zero-based) of the guard
*	that was ready.
*	- The csp::Alternative::priSelect() method checks the guards in order (earliest in the guard list has the highest
*	priority).  If multiple guards are ready at the same time, the highest priority guard will be chosen.
*	- After the csp::Alternative::priSelect() method returns having chosen an input guard, the process is obliged to
*	perform a read on that channel.
*
*	The above points cover most of the semantics of csp::Alternative.  It will only ever "choose" one of the available
*	guards, so the typical use is to repeatedly use it in a loop, forever waiting for an input on one of many channels.
*	You must remember to actually perform the input yourself after an input guard has been chosen.  Not doing so
*	could result in strange behaviour in your application.
*
*	In the example above, we used csp::Alternative::priSelect(), with the quitIn channel as highest priority.  Consider
*	what would happen if the dataIn guard had the highest priority instead.  If there was a continual fast flow of 
*	data on the dataIn channel input, it would always be chosen.  Even if the quitIn guard was always ready, if the dataIn
*	guard is also always ready, the quitIn guard will never be chosen.  This is referred to as "starvation".
*	In our example, we avoided this by making the quitIn guard (which will be used much less frequently, and only once)
*	have the higher priority.  In other circumstances we would probably use the csp::Alternative::fairSelect() method
*	to ensure fair selection between the guards -- priority is rotated between all the guards, so starvation cannot
*	occur.  Be careful when using this class.  This is a correct use:
	@code
		void run()
		{
			list<Guard*> guards;
			guards.push_back(in0.inputGuard());
			guards.push_back(in1.inputGuard());
			Alternative alt(guards);
			while (true)
			{
				switch (alt.fairSelect())
				{
					// ...
				}				
			}
	@endcode
*	
*	This, however, is an <b>incorrect</b> use:
*
	@code
		void run()
		{			
			while (true)
			{
				list<Guard*> guards;
				guards.push_back(in0.inputGuard());
				guards.push_back(in1.inputGuard());
				
				Alternative alt(guards); // BAD!
			
				switch (alt.fairSelect())
				{
					// ...
				}				
			}
	@endcode
*
*	In the latter example, the csp::Alternative is reconstructed each iteration of the while loop.  This means the fairSelect
*	will not work as intended -- the priority will not be rotated because each csp::Alternative is actually different
*	from the last.  The first example is correct, because the same csp::Alternative object is used on each loop iteration,
*	and thus it can remember where the rotated priority lies.
*
*	When one of the channels being used as a guard becomes poisoned, its guard will forever be ready while it is poisoned.
*	A csp::PoisonException will only be thrown when the channel input occurs (usually in the body of a switch statement).
*	The select method will never throw a csp::PoisonException, whether the guard is selected or not selected.
*
*	@subsection tut-alt-timeout Timeout Guards
*
*	Besides channel input guards, the other main type of guards are timeout guards; csp::RelTimeoutGuard and 
*	csp::TimeoutGuard.  The former waits <i>for</i> a specified <i>amount</i> of time, whereas the latter waits
*	<i>until</i> a specified <i>instant</i> in time.
*
*	Imagine you want a process that sends a message at least every second -- either a new value, or repeating thel
*	last receieved value:
	@code
	class OneSecondSender : public CSProcess
	{
	private:
		AltChanin<int> in;
		Chanout<int> out;
	protected:
		void run()
		{			
			try
			{
				Alternative alt( boost::assign::list_of<Guard*> ( in.inputGuard() ) ( new RelTimeoutGuard(Seconds(1)) ) );
				int data = 0;
			
				while (true)
				{
					switch (alt.priSelect())
					{
						case 0:
							in >> data;
							out << data;
							break;
						case 1:
							//Send most recent value:
							out << data;
							break;
					}	
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out.poison();
			}
		}
	public:
		// ... Constructor ...
	};
	@endcode
*	- This time we are using the boost::assign::list_of helper to form an "on the spot" list of csp::Guard.
*	- We allocate the timeout guard using "new", and do not bother to keep a pointer ourselves; the csp::Alternative
*	will delete the guard.
*	
*	The main thing to note with timeout guards is that while they behave sensibly (a timeout guard will never be ready
*	before its time is up), they are not hard real-time.  If you used the above process and never sent it a value,
*	it would send out values every 1.001 seconds or similar (largely dependent on your OS, and what load it was under
*	at the time).  Due to the time taken processing the alt, and in sending the value, a small delay would naturally occur.
*
*	Similarly, do not assume that small values will or will not expire -- although a RelTimeoutGuard with exactly zero
*	seconds will always be ready.  If you have as your first guard in a priSelect() a RelTimeoutGuard with a small timeout,
*	(say, a few micro- or even, in some cases, milliseconds), it will sometimes be ready "immediately", and sometimes 
*	it will not.  This depends on the processing time during the alt, and whether the operating system has scheduled
*	the process out at an inopportune moment.  You will find that most of the time it is not ready, but do not build
*	your program around false assumptions about the timeout system.  It is not exact, but it will work well enough for
*	most purposes.
*
*	@section tut-extinput Extended Input (aka Extended Rendezvous)
*
*	C++CSP2 (unbuffered) channels are synchronised.  This means that a communication takes place only when both parties
*	are ready to communicate.  Extended input is about extended this synchronisation, allowing the reader
*	to perform additional actions while keeping the writer waiting.  A common use of this function is to
*	add "invisible" middle-man processes that perform an action without breaking the existing synchronisation
*	between writer and reader.  For example, this process writes out each communicated string without
*	interfering with the program (other than making it run a little slower):
*	@code
	class StringLoggerProcess
	{
		Chanin<std::string> in;
		Chanout<std::string> out;
	protected:
		void run()
		{
			try
			{
				std::string s;
				while (true)
				{
					ScopedExtInput<std::string> extInput(in,&s);
					std::cout << "String sent: " << s << std::endl;
					out << s;
				}
			}
			catch (PoisonException&)
			{
				in.poison();
				out.poison();
			}
		}
		// ... constructor ...
	};
	@endcode
*	- Extended input is usually done using the csp::ScopedExtInput class.  The extended action lasts as long
*	as the csp::ScopedExtInput object is in scope.  In the above example, this is the rest of the body of the while
*	loop.
*	- The two parameters to its constructor are the channel-end on which to perform the input, and the location
*	to store the data.  The constructor performs the input, so you do not need to do anything else.  In
*	particular do not then perform an input on the channel yourself!  This would lead to undefined results.
*
*	This guide is continued in @ref guide3.	
*/

/**
*	@page guide3 Guide part 3: Shared Channels and Buffered Channels
*
*	In @ref guide1 and @ref guide2, we looked only at One2OneChannel.  There are more types of channels, 
*	which can be viewed in terms of two aspects; shared channels, and buffered channels.
*
*	@section tut-shared Shared Channels
*
*	Imagine that you had a farmer process sending out work to a worker, the results of which are collected by another
*	process:
	@code
	One2OneChannel<Work> workChannel;
	One2OneChannel<Results> resultsChannel;
	Run(InParallel
		( new Farmer( workChannel.writer() ) )
		( new Worker( workChannel.reader() , resultsChannel.writer() ) )
		( new Collector( resultsChannel.reader() ) )
	);
	@endcode
*
*	This system is functioning as it should -- but you wish to add another Worker or two so that multiple instances of work 
*	can be done in parallel.  One way to do this would be to re-engineer the existing processes, to have multiple work/results
*	channels, and have a manager process that routes the work appropriately.
*
*	However, shared channels provide a simple way of accomplishing your aim:
	@code
	One2AnyChannel<Work> workChannel;
	Any2OneChannel<Results> resultsChannel;
	Run(InParallel
		( new Farmer( workChannel.writer() ) )
		( new Worker( workChannel.reader() , resultsChannel.writer() ) )
		( new Worker( workChannel.reader() , resultsChannel.writer() ) )
		( new Worker( workChannel.reader() , resultsChannel.writer() ) )
		( new Collector( resultsChannel.reader() ) )
	);
	@endcode
*	- A csp::One2AnyChannel is for use with one writer (One) communicating (2) any reader (Any).  Correspondingly, a
*	csp::Any2OneChannel is for use with any writer (Any) communicating (2) one reader (One).  There is also a csp::Any2AnyChannel
*	for any writer to communicate to any reader.
*	- The use of the shared channels is no different to csp::One2OneChannel -- the difference is that having multiple writers
*	communicate on an csp::Any2OneChannel in parallel is safe, whereas having multiple parallel writers on a csp::One2OneChannel
*	is unsafe and will probably crash your application.
*
*	Note that the behaviour of shared channels is still one writer communicating to one reader -- it is just that multiple
*	writers and/or multiple readers can contend on the channel.  Specifically, these channels are not broadcast channels.
*
*	Imagine that on a csp::Any2AnyChannel, one writer attempts to write the value 1.  A second writer then attempts to write
*	the value 2.  A third writer tries to write the value 3.  All these writers will wait for a reader.  When a reader
*	then tries to read from the channel, it will read the value 1 (and the first writer will finish its communication).
*	When a second reader tries to read from the channel, it will read the value 2 (and the second writer will finish
*	its communication).  The third writer remains waiting for a reader.  The first reader never sees the values 2 or 3,
*	and the second reader never sees the values 1 or 3.  Each writer and reader are paired up, and data is never
*	shared between multiple readers.
*
*	So with reference to the farmer example, the csp::One2AnyChannel<Work> channel allows a packet of work to be sent
*	out by the Farmer, which will be picked up by <i>one</i> of the Worker processes.  The Farmer doesn't know which
*	Worker will take it, and it does not care.  Once a Worker has finished, it will send its results to the Collector.
*	The results could come from any Worker to the Collecter, but again the Collector does not care.  So none of the processes
*	have had to be changed -- only the channels.
*
*	There is only one complication with using shared channels.  Channels with a shared reading-end (csp::One2AnyChannel
*	and csp::Any2AnyChannel) cannot be used with an Alternative.  This stems from implementation complexities/inefficiencies.
*	Consider if two processes wanted to alt over a shared channel, while another channel wants to read from the channel.
*	When a writer does write to the channel, which reader should be used?  For this reason, alting over shared reading-end channels
*	is disbarred, and hence they return a csp::Chanin from their reader() method, rather than csp::AltChanin.  If you
*	really do want to alt over shared channels, consider attaching an csp::common::ExtId (or csp::common::Id) process to the shared channel,
*	and alting over a One2OneChannel that the ExtId/Id process is writing to.
*
*	@section tut-buffered Buffered Channels
*
*	Buffered channels are useful for efficiency purposes, and for altering the semantics (compared to non-buffered channels).
*	Consider our simple example from @ref guide1, with a process sending out numbers, to be printed by another process:
	@code
	One2OneChannel<int> c;
	Run(InParallel
		( new IncreasingNumbers( c.writer() ) )
		( new NumberPrinter( c.reader() ) )
	);
	@endcode
*	
*	Synchronisation between these two processes is not really important -- the first process is trying to send out
*	numbers as fast as it can, and the second process will print them when it receieves them.  At the moment what is 
*	happening between these two processes is that the IncreasingNumbers process will produce one value to write to the channel,
*	and then it will have to wait until the NumberPrinter process has read it.  This means that the processor(s) in
*	your system will be continually switching back and forth between the two processes with each channel communication.
*	We can improve this performance using buffers:
	@code
	FIFOBuffer<int>::Factory bufferFactory(16);
	BufferedOne2OneChannel<int> c(bufferFactory);
	Run(InParallel
		( new IncreasingNumbers( c.writer() ) )
		( new NumberPrinter( c.reader() ) )
	);
	@endcode
*	- csp::FIFOBuffer is a limited-size FIFO buffer.
*	- csp::FIFOBuffer::Factory is a factory for csp::FIFOBuffer.
*	- The csp::FIFOBuffer::Factory class takes the size of the FIFO buffer as a parameter.
*	- Buffered channels (such as csp::BufferedOne2OneChannel) take a buffer factory as a parameter to their constructor.
*	- The channel-ends are used in exactly the same way as non-buffered channel-ends.
*
*	The effect of the above code is to introduce a size-16 FIFO buffer between the two processes.  This means that instead
*	of the continual switching between processes with each communication, it is likely that the IncreasingNumbers will write multiple
*	values to the buffer before being switched out, which will allow the NumberPrinter process to read some (or all)
*	of these values being itself being switched out.
*
*	If the buffer fills up (i.e. it is holding 16 items) and then the IncreasingNumbers process tries to write to the buffer,
*	it will be forced to wait until the NumberPrinter process has read some values and hence freed space in the buffer.
*	Similarly, if the buffer is empty when the NumberPrinter process attempts to read from it, it will be made to wait
*	until an item has been written to the buffer.
*
*	Thus, while a csp::FIFOBuffer allows buffering to take place on a channel, it does not entirely remove the synchronisation
*	between the two processes -- when the buffer is full or empty, the writer or reader (respectively) would have to wait.
*	One of these restrictions -- the buffer being full -- can be lifted by using an csp::InfiniteFIFOBuffer.  As you might
*	expect, this is a buffer without a maximum size.  The writer will never have to wait when writing to such a channel, although of course
*	if it was empty the reader would still have to wait.
*
*	The use of infinite buffers is potentially dangerous.  In our example, the IncreasingNumbers process is likely to be able
*	to produce values faster than the NumberPrinter process can print them.  If these two processes were connected by
*	an infinitely-buffered channel, the buffer would likely grow forever (because more was being put in than taken out)
*	which would eventually lead to all your RAM being used up by this buffer.  Be very careful when using infinite-buffers
*	to ensure that they cannot grow indefinitely.
*
*	C++CSP2 offers a third type of buffer; csp::OverwritingBuffer.  As the name suggests, this is a FIFO buffer
*	that overwrites the oldest data when it becomes full.  Imagine a process writing increasing integers to a size
*	four overwriting buffer (without the values being read just yet).  The buffer would fill up with 1,2,3,4 (1 being the oldest value).  When
*	the process wrote another value, the contents of the buffer would be 2,3,4,5.  Then 3,4,5,6.  And so on.  If a process was now
*	to read from the buffer, it would read the value 3.
*
*	All shared channels can be buffered, and all buffered channels can be shared.
*
*	This guide is continued in @ref guide4.	
*/

/**
*	@page guide4 Guide part 4: Advanced Ways of Running Processes
*
*	@section tut-forking Forking Processes
*
*	There will often be times where you want to start a process running, but also continue code in the current function.
*	This may be just for ease, or you may want to interact with the processes.  C++CSP2 provides a mechanism for doing
*	this; forking.  Forking is done using csp::ScopedForking, a @ref scoped "Scoped" object:
	@code
	One2OneChannel<int> c,d;
	{
		ScopedForking forking;
		
		forking.fork(new Id<int>(c.reader(),d.writer()));
		
		c.writer() << 6;
		int n;
		d.reader() >> n;
		
		c.writer().poison();
		d.reader().poison();
	}
	@endcode
*	- When csp::ScopedForking goes out of scope -- either by reaching the end of the block that it was declared in, or
*	because an exception was thrown -- it will wait for all the forked processes to finish.
*	
*	It is very important that the channels used by the forked process were declared <b>before</b> the csp::ScopedForking.
*	In C++, objects are destroyed in reverse order of their construction.  So if the channels were constructed after
*	the csp::ScopedForking, they would be destroyed before csp::ScopedForking.  This means that the processes you have
*	forked could end up using ends of channels that have been destroyed.  So it is very important that you declare
*	all channels, barriers, buckets, etc that the forked processes use <b>before</b> the csp::ScopedForking.
*
*	You should also be careful about poisoning channels and such.  Consider this illustrative code:
	@code
	One2OneChannel<int> c,d,e;
	try
	{
		ScopedForking forking;
		
		forking.fork(new Successor<int>(c.reader(),d.writer()));
		
		forking.fork(new Widget(e.writer()));
		
		int n;
		e.reader() >> n;
		n *= -6;
		c.writer() << n;
		d.reader() >> n;
	}
	catch (PoisonException& e)
	{
		e.reader().poison();
		c.writer().poison();
		d.reader().poison();
	}	
	@endcode
*
*	Consider what would happen if the Widget process poisoned its channel "e" before we tried to read from it.  A 
*	csp::PoisonException would be thrown.  As part of the stack unwinding, the csp::ScopedForking object would be
*	destroyed -- which would cause it to wait for all its forked processes.  However, the Successor process would
*	not have finished, because it runs until it is poisoned -- and we will not poison its channels until the catch
*	block.  So the above code will deadlock if the Widget channel turns out to be poisoned.  You can avoid such problems
*	by declaring the csp::ScopedForking object outside the try block (but after the channel declarations).
*
*	csp::ScopedForking can be used to fork compounds of processes -- anything that can be passed to the Run command
*	can be passed to csp::ScopedForking::fork:
	@code
	forking.fork( InParallel(P)(Q) );
	forking.fork( InSequence(S)(T) );
	@endcode
*
*	@section tut-advrunning Running Processes In The Same Thread
*
*	So far in this guide, I have tried to avoid going in to detail on how the library is implemented, to keep the
*	introduction simple.  However, this section requires such detail.  When you use the csp::Run method on a single
*	process, or csp::ScopedForking::fork on a single process, that process is run in a new (kernel-)thread of its own.
*	Kernel-threads (usually referred to simply as threads) are OS-level concepts that are created using
*	CreateThread on Windows, and pthread_create on Linux.  Kernel-threads can be run on different processors to each other,
*	and thus can take advantage of multi-core and multi-CPU machines.
*	When you use code such as this:
	@code
	forking.fork( InParallel(P)(Q) );
	forking.fork( InSequence(S)(T) );
	@endcode
*	P, Q, S and T will all be run in separate threads.  The advantage of using kernel-threads is that taking advantage
*	of parallel processors is automatic.  The main disadvantage of kernel-threads is their speed.  If you want to 
*	perform a great deal of communication between two processes, kernel-threads can be slow.  For programs where
*	the number of processes is small (perhaps tens) this may not be a problem.  However, if you are
*	writing large-scale simulations with thousands of processes, this speed may well be an issue.
*
*	The alternative to kernel-threads is user-threads.  User-threads live inside kernel-threads -- two user-threads in
*	the same kernel-thread cannot run on separate processors in parallel.  However, communication between user-threads
*	is usually five to ten times as fast as kernel-threads.  Processes can be grouped together as follows:
	@code
	forking.fork( InParallelOneThread(P)(Q) );
	@endcode
*	- This code puts P and Q in the same new thread
	@code
	forking.forkInThisThread( InParallelOneThread(P)(Q) );
	@endcode
*	- This code puts P and Q in the current thread
	@code
	forking.forkInThisThread( InParallel(P)(Q) );	
	@endcode
*	- This code will not compile.
*
*	More detail on the threading arrangements that different combinations give are available on the @ref run "Running Processes" page.
*
*	This guide is continued in @ref guide5.	
*/

/**
*	@page guide5 Guide part 5: Mobiles, Barriers and Buckets
*
*	@section tut-mobile Mobiles
*
*	csp::Mobile is a pointer-like class for implementing transfer of ownership.  They function in a similar manner to
*	std::auto_ptr, boost::shared_ptr and the like; they can be easily used in place of pointers, but with different 
*	behaviour.
*
*	When a csp::Mobile is assigned to (or communicated over a channel), the pointer transfers -- the destination
*	receieves the new pointer, and the source is blanked.  This allows pointers to be communicated around without
*	introducing aliasing.  Mobiles are used for efficiency (sending a pointer to an object is almost always faster
*	than sending the obejct) while preventing aliasing (so that two processes do not try to write to an object at the 
*	same time) and automating deletion of the object (much as other smart pointer classes do).  If you want only
*	efficiency, send a normal pointer.  If you don't want to prevent aliasing, use a boost::shared_ptr.
*
*	Examples of the use of Mobiles is provided in the next section on barriers.
*
*	@section tut-barriers Barriers	
*
*	Whereas channels combine synchronisation with data-passing, barriers are used purely for synchronisation.
*	A csp::Barrier keeps track of processes that are enrolled on it.  Processes may dynamically enroll (csp::BarrierEnd::enroll() and resign
*	(csp::BarrierEnd::resign()) at any time.  A synchronisation completes when all currently-enrolled processes synchronise 
*	on the barrier (by calling csp::BarrierEnd::sync()).
*
*	Barriers (csp::Barrier) are accessed by using barrier ends (csp::BarrierEnd).  The direct function of a csp::Barrier
*	is simply to get new barrier-ends -- much as the direct function of channels is to get channel-ends.  In C++CSP2,
*	you are allowed to copy channel-ends as you wish; the only restriction is that you ensure that non-shared channel-ends
*	are not used in parallel.  Barrier-ends are more difficult, because they effectively have state -- that is, a barrier-end
*	may or may not be enrolled.  If they could be copied, what should the effect of copying an enrolled end be?  To avoid
*	such complications, BarrierEnds are prevent from being copied and are wrapped inside mobiles.
*
*	This is perhaps best explained with some code:
	@code
	Barrier barrier;
	Mobile<BarrierEnd> endA,endB;
	//Now: endA is blank, endB is blank
	endA = barrier.end();
	endB = barrier.enrolledEnd();
	//Now: endA has a non-enrolled end, endB has an enrolled end
	endA->enroll();
	endB->resign();		
	//Now: endA has an enrolled end, endB has a non-enrolled end
	CSProcess* process = new csp::common::BarrierSyncer(endB);	
	//Now: endA has an enrolled end, endB is blank
	endB = endA;
	//Now: endA is blank, endB has an enrolled end
	@endcode
*	- csp::Barrier::end() returns a non-enrolled barrier-end.
*	- csp::Barrier::enrolledEnd() returns an enrolled barrier-end.
*	- csp::BarrierEnd::enroll() enrolls on the barrier.  Calling it on an already-enrolled end has no effect.
*	- csp::BarrierEnd::resign() resigns from the barrier.  Calling it on a non-enrolled end has no effect.
*	- As explained in the previous section, assigning mobiles to each other transfers the pointer.
*	
*	There is one main rule to be observed:
*	- Blanking, destroying, or assigning to a mobile that holds a <b>non-enrolled</b> end is a <b>valid</b> operation.  
*	- Blanking, destroying, or assigning to a mobile that holds an <b>enrolled</b> end is an <b>invalid</b> operation.  
*		- An exception will be thrown by csp::BarrierEnd's destructor in this case, unless the barrier-end is being
*		destroyed due to stack unwinding (i.e. because of an exception).
*
*	Dynamic enrollment and resignation is useful, but it can complicate reasoning about barriers:
	@code
	Barrier barrier;
	ScopedForking forking;
	Mobile<BarrierEnd> endA,endB;
	endA = barrier.enrolledEnd();
	endB = barrier.enrolledEnd();
	forking.fork(new csp::common::BarrierSyncer(barrier.enrolledEnd());		
	//BarrierSyncer could not complete sync yet - waiting for two other processes
	endA->resign();
	//BarrierSyncer could not complete sync yet - waiting for one other process
	endA->enroll();
	endB->resign();
	//BarrierSyncer could not complete sync yet - waiting for one other process	
	endA->resign();
	endB->enroll();
	//BarrierSyncer could complete its sync between the above two statements -- it was the only process enrolled
	@endcode
*
*	Usually, you will find that you want all processes to enroll on the barrier to begin with, and to avoid
*	over-use of dynamic enrollment and resignation.  There is another issue to beware of.  Consider the following code:
	@code
	Barrier barrier;
	Run(InParallel
		( InSequence
			( new csp::common::BarrierSyncer(barrier.end()) ) //A
			( new MyProcess() )
		)
		( InSequence
			( new csp::common::WaitProcess(Seconds(1)) )
			( new csp::common::BarrierSyncer(barrier.end()) ) //B	
		)
	);
	@endcode
*
*	Consider how long it will be before MyProcess runs.  At first glance, your guess may be one second.  However,
*	the barrier-ends passed to the two csp::common::BarrierSyncer processes are non-enrolled.  This means that what
*	will likely happen is that the BarrierSyncer labelled "A" above will run, enroll (the only process then enrolled),
*	synchronise immediately, resign and then MyProcess will be run.  Around a second later, BarrierSyncer "B" will
*	similarly enroll, synchronise immediately, and then resign.  To get the behaviour that was probably desired,
*	make sure to pass pre-enrolled ends:
	@code
	Barrier barrier;
	Run(InParallel
		( InSequence
			( new csp::common::BarrierSyncer(barrier.enrolledEnd()) )
			( new MyProcess() )
		)
		( InSequence
			( new csp::common::WaitProcess(Seconds(1)) )
			( new csp::common::BarrierSyncer(barrier.enrolledEnd()) )
		)
	);
	@endcode
*	
*	Using the above, MyProcess will not be run for at least one second.  It is not a problem that the BarrierSyncer
*	process will then also call enroll on the barrier -- this second enroll call will have no effect.  A single resign
*	call will then resign (no matter how many times enroll may have been called already).
*
*	There is one further feature of C++CSP2 that can be of use when using barriers.  It can be annoying -- especially
*	in a long run() method, that may have exceptions being thrown -- to remember to enroll and, in particular, resign from 
*	barriers.  Consider an illustrative example:
	@code
class SomeProcess : public CSProcess
{
private:
	Mobile<BarrierEnd> end;
	csp::Chanin<bool> in;
	csp::Chanout<int> out;
protected:
	void run()
	{
		end->enroll();
		
		try
		{		
			for (int i = 0;i < 100;i++)
			{
				bool b;
				in >> b;
			
				if (b)
				{
					end->resign();
					return;
				}
							
				out << 7;
				
				end->sync();
			}
		}
		catch (PoisonException&)
		{
			in.poison();
			out.poison();
		}
		end->resign();
	}
public:
	// ... constructor ...
};
	@endcode
*
*	It would have been easy to forget the resign call before the return, or to place the resign call only in the
*	catch handler, or only at the end of the try block.  This process can be simplified using the csp::ScopedBarrierEnd class:
	@code
class SomeProcess : public CSProcess
{
private:
	Mobile<BarrierEnd> end;
	csp::Chanin<bool> in;
	csp::Chanout<int> out;
protected:
	void run()
	{
		ScopedBarrierEnd scopedEnd(end);
		//end will now be blank
		
		try
		{		
			for (int i = 0;i < 100;i++)
			{
				bool b;
				in >> b;
			
				if (b)
				{					
					return;
				}
							
				out << 7;
				
				scopedEnd.sync(); //Using scopedEnd!
			}
		}
		catch (PoisonException&)
		{
			in.poison();
			out.poison();
		}		
	}
public:
	// ... constructor ...
};
	@endcode
*	
*	csp::ScopedBarrierEnd is an <a href="http://en.wikipedia.org/wiki/RAII">RAII</a> scoped class.  The use of scoped
*	classes is explained more in the  @ref scoped "Scoped" page.  A ScopedBarrierEnd enrolls on the given barrier
*	in its constructor, and resigns from the barrier in its destructor.  This makes it automatically resign
*	when the function returns, or when an exception is thrown.  The main subtlety involved in their use is
*	that the csp::Mobile<csp::BarrierEnd> that you pass to the csp::ScopedBarrierEnd constructor will be blanked
*	once the object has been constructed.  Hence all synchronise calls must be made on the csp::ScopedBarrierEnd (scopedEnd in the above example),
*	<b>not</b> on the original barrier-end ("end" in the above example) which will by then be blank.
*
*	@section tut-buckets Buckets
*
*	Like barriers, buckets are another sychronisation primitive.  There are two operations that can be performed on 
*	@link csp::Bucket buckets @endlink:
*	- "Falling into" a bucket, which waits until the bucket is flushed (csp::Bucket::fallInto()).
*	- "Flushing" a bucket, which frees all the waiting processes, i.e. all the processes that have already fallen into the bucket (csp::Bucket::flush()).
*
*	Unlike barriers, which wait for all enrolled processes to synchronise, buckets have no notion of enrollment, and
*	some flushes may free hundreds of processes, while other flushes may free none.  This can sometimes lead to non-determinism:
	@code
	Bucket bucket;
	Run(InParallel
		(new BucketJoiner(&bucket))
		(new BucketJoiner(&bucket))
		(new BucketFlusher(&bucket))
	);
	@endcode
*	- For our example, a BucketJoiner is a process that falls into a bucket once then exits, and a BucketFlusher is 
*	a process that flushes a bucket once then exits.
*
*	The above code may, or may not, deadlock -- depending on how the processes happen to get scheduled.  If both the joiners
*	are scheduled before the flusher, the code will finish.  If one (or both) joiner runs after the flusher, they will
*	wait forever on the bucket.  You should bear this consideration in mind when using buckets in your application.
*
*	This concludes the guide to C++CSP2.
*/

/** @defgroup scoped Scoped Classes
*
*	Scope can be used to provide an easy way to program things that need to have a begin/end pairing.  This can be seen as a use of the RAII (Resource Acquisition Is Initialisation)
*	pattern.
*
*	This idea has been used for a few C++CSP2 classes, such as ScopedForking, ScopedBarrierEnd and ScopedExtInput.
*	When the class is constructed, the "action" begins.  In the case of ScopedForking, nothing actually happens during
*	construction.  However, ScopedBarrierEnd enrolls on the barrier on construction, and ScopedExtInput begins an extended input
*	on construction.
*
*	During the objects' lifetime, they can be used.  ScopedForking can fork processes and ScopedBarrierEnd can synchronize
*	on the barrier (ScopedExtInput has nothing like that).
*
*	On destruction the action is finished.  ScopedForking's destructor waits for all the forked processes to finish.
*	ScopedBarrierEnd resigns from the barrier.  ScopedExtInput completes the extended input.
*
*	The usefulness of these objects (compared to using a normal BarrierEnd, or a beginExtInput/endExtInput function pair)
*	is that the action is always completed and cleaned-up properly, even in the case of an exception being thrown.  This
*	means you don't have to spend effort remembering to always end an extended input (or always resign from a barrier) 
*	when faced with complex code paths; C++'s scope naturally sorts out the problem for you.
*
*	@code
	try
	{
		ScopedBarrierEnd end(barrier.end());
		
		if (x)
		{
			if (y)
			{
				...
			}
			else
			{
				if (z)
				{
					//No need to remember to resign from the barrier here - it will happen automatically
					return;
				}
				else
				{
					//No need to remember to resign here either - it happens automatically when destroyed
					//by the stack-unwinding caused by the exception
					throw Exception();
				}
			}
		}
	} //If you reach here normally, the barrier is resigned from too
	catch (Exception)
	{
	}
	@endcode
*
*	These objects are intended for use on the stack, as above.  You can use them with new/delete but you lose their benefits.
*	Also, the objects are expected to always be used from the same process, so passing a pointer to them around could
*	cause problems.
*	
*	In particular, <b>these objects must never be used as member of your process class.</b>  This is because the objects
*	will be deleted by the destructor of the process, which can happen in a different process.  The Scoped prefix on
*	these classes is intended to remind the user that they are only suitable for use as local variables in functions, not 
*	as class members.
*
*	C++ local objects are destroyed in reverse order of their construction.
*	@code
	{
		MyClass a;
		MyClass b;
		MyClass c;
	
	} //First c is destroyed, then b is destroyed, and then a.
	@endcode
*
*	Programmers should bear this in mind when writing code.  Consider the following code:
*
*	@code
	{
		ScopedForking forking;
		One2OneChannel<int> c,d;
		int n;
	
		forking.forkInThisThread(new common::Id<int>(c.reader(),d.writer());
		
		c.writer() << 6;
		d.reader() >> n;
		
		c.writer().poison();
	}
*	@endcode
*
*	At first glance this code looks fine.  In actuality, it contains a dangerous problem.  The Id process will be forked,
*	the two communications will happen, and then this code will poison the channel @a c.  The very next action - which
*	will happen before Id has run again - is to reach the end of the block and begin destroying objects.  @a d will be destroyed,
*	then @a c will be destroyed, then @a forking will be destroyed - which will block until the Id process has finished.
*	At this point, Id will run again.  It is now using the ends of two channels <b>that have now been destroyed</b>!  Obviously
*	this is a problem, and could cause a crash.
*
*	The solution is to re-order the first two lines:
*	@code
	{
		One2OneChannel<int> c,d;
		ScopedForking forking;
	@endcode
*
*	Because of this potential subtle error, I prefer to put the channel, barrier, etc declarations outside the block
*	containing the scoped forking, to avoid having to think about construction order:
*	@code
	One2OneChannel<int> c,d;
	{
		ScopedForking forking;
		...
	} //Channels will definitely still exist here
	@endcode
*
*/

/**
*	@page migration C++CSP v1.x Migration Guide
*
*	C++CSP2 is not fully compatible with C++CSP v1.x.  Code developed for the old 1.x versions of C++CSP
*	will not compile or function the same with C++CSP2.  I realise that this is very annoying for
*	those who have already developed programs for C++CSP v1.x, and now have to migrate them.  The
*	reasons for this lack of compatibility include:
*
*	- Better design.  I have cleaned up some of uglier features of C++CSP v1.x, such as the nasty FunctionNotSupportedException,
*	and the horrible way of doing an extended input on alt guard, not to mention removed the NULL-terminated
*	varargs way of calling some functions.  These changes all inherently break what was there before --
*	but only in order to improve the design.  In the long-term, I think this is worth it.
*	- Renaming classes/functions.  Since I was already changing much of the library (as described above),
*	I took the opportunity to rename some of classes.  Most notably, buffered channels now match their
*	JCSP counterparts, using BufferedOne2OneChannel rather than the nonsensical One2OneChannelX.
*	- Added functionality.  C++CSP2 supports both (kernel-)threads and user-threads (C++CSP v1.x only supported
*	the latter), so this necessitated some changes to the library.
*
*	To help all previous users of C++CSP v1.x to convert their programs for use with C++CSP2, I have
*	prepared this guide.  I will go through the various areas of C++CSP2 that have changed.  Thankfully,
*	almost all of these changes will result in a compile error.  So if you go through each compile
*	error, then with the help of this guide you can convert your program to using C++CSP2 properly.
*	Code blocks are provided that show the OLD code, and the corresponding NEW code.
*
*	@section mig-general The Basics (a.k.a. Nuts and Bolts)
*
*	The first change is that the C++CSP headers have "moved":
	@code
	#include <csp/csp.h> //OLD
	#include <cppcsp/cppcsp.h> //NEW
	@endcode
*	- This code is partly so that if you really wanted, you could keep both the C++CSP v1.x and C++CSP2 libraries
*	on your system.  However, never try to use both versions in a single application!  The other
*	reason is that the new header location/name is a little more descriptive.
*
*	The functions used to start and stop the C++CSP2 library have had their names changed (for similar
*	reasons to the above):
	@code
	Start_CSP(); //OLD
	Start_CSP_NET(); //OLD
	End_CSP(); //OLD
	
	Start_CPPCSP(); //NEW
	End_CPPCSP(); //NEW
	@endcode
*	- The old Start_CSP/Start_CSP_NET functions had parameters to allow you to configure C++CSP's behaviour.
*	The new functions have no such parameters.
*	- Currently, C++CSP2 has no in-built network functionality, so there is only a single start function.
*
*	@section mig-time Time/Sleep/Yield Functions
*	
*	The Time data-type and its associated functions such as Seconds or GetSeconds remain the same;
*	in fact, a couple more functions have been added, for your convenience that use references instead of pointers.
*
*	The sleep/wait functions used to be members of csp::CSProcess.  This caused annoyance if you wanted
*	to call these functions in a class that was not itself a process.  So these functions have
*	now been moved to being stand-alone:
	@code
	class SomeProcess : public CSProcess
	{
	protected:
		void run()
		{
			sleepFor(Seconds(1)); //OLD, sleepFor was a member of CSProcess
			SleepFor(Seconds(1)); //NEW, SleepFor is a function in namespace csp
		
			Time t;
			CurrentTime(&t);
			t += Seconds(1);
			sleepTo(t); //OLD, sleepTo is a member of CSProcess
			SleepUntil(t); //NEW, SleepUntil is a function in namespace csp
		}
	};
	@endcode
*
*	Similarly, yield used to be a member function of csp::CSProcess.  There are now two stand-alone yield functions
*	in C++CSP2; csp::CPPCSP_Yield() yields to other user-threads in the same thread, and csp::Thread_Yield()
*	yields to other kernel-threads.  CPPCSP_Yield is useful if you are not doing any channel communications
*	or synchronisation and want to allow other user-threads to run.  Thread_Yield is perhaps less useful;
*	your thread will be switched out at the end of its time-slice anyway.
*
*	@section mig-proc Processes, and Running Processes
*
*	One change to processes is described in the previous section -- its helper functions have been
*	moved out of the class.  There are two other major changes to processes themselves.
*
*	There are now two process classes; csp::CSProcess and csp::ThreadCSProcess.  Usually you will
*	want to use CSProcess as before; if you do not change any of your existing classes from
*	inheriting from CSProcess, everything will work fine as before.  For an explanation of the 
*	purpose of csp::ThreadCSProcess, click on its link to read the documentation.
*	
*	The constructor of csp::CSProcess used to have two parameters, one which was a mandatory stack
*	size and one which was a mysterious reserve size.  This constructor now takes one optional argument
*	(the stack size).  The reason for it becoming optional is not that I have added a magical way of
*	determining the right stack size (alas!), but because there is an arbitrary high default value of 1 megabyte.
*	All your existing processes that specify a stack size should be fine as before -- although if 
*	you are moving to 64-bit, remember that some of your data types will double in size.  You can no longer
*	specify a reserve size -- if you have any code that did specify the second parameter (which is unlikely), simply remove
*	the second parameter.
*
*	One of the major changes to the library has been in the area of running processes.  The new mechanism
*	is explained in detail on the @ref run "Running Processes" page.  Below are examples of how to convert your
*	existing code to the new system.  It can either be converted as-is, to continue using user-threads,
*	or you can change to using kernel-threads:
	@code
	CSProcess* processA = new Widget;
	CSProcess* processB = new OtherWidget;
	
	Parallel(processA,processB,NULL); //OLD
	
	RunInThisThread(InParallelOneThread (processA) (processB) ); //NEW, using user-threads
	//or:
	Run(InParallelOneThread (processA) (processB) ); //NEW, using kernel-threads (processA and processB together in a single new thread)
	//or:
	Run(InParallel (processA) (processB) ); //NEW, using kernel-threads (processA and processB together in separate new threads)
	
	
	
	Sequential(processA,processB,NULL); //OLD
	
	RunInThisThread(InSequenceOneThread (processA) (processB) ); //NEW, using user-threads
	//or:
	Run(InSequenceOneThread (processA) (processB) ); //NEW, using kernel-threads (processA and processB together in a single new thread)
	//or:
	Run(InSequence (processA) (processB) ); //NEW, using kernel-threads (processA and processB together in separate new threads)
	
	@endcode
*
*	You will notice that the syntax has changed.  Gone is the annoying C-style NULL-terminated vararg list, replaced
*	by a new C++ mechanism.  Each parameter is bracketed separately -- no commas!  There is a new
*	csp::Run method that can take sequential or parallel lists.  You can also easily compose parallel sequences
*	or sequenced parallels.  All the detail is on the @ref run "Running Processes" page.
*
*	The mechanism for forking has also changed.  The old mechanism involved spawning a process with
*	a given barrier to synchronise on when it was done.  The new mechanism uses the @ref scoped "Scoped Classes"
*	idea.
	@code
	CSProcess* processA = new Widget;
	
	//begin OLD
	
	Barrier barrier;
	
	spawnProcess(processA,&barrier); //spawnProcess was a static method of CSProcess
	
	//... Do something ...
	
	barrier.sync(); //wait for processA to finish
	
	//end OLD
	
	//begin NEW
	
	{
		ScopedForking forking;
		
		forking.forkInThisThread(processA); //using user-threads, processA remains in this thread
		//or:
		forking.fork(processA); //using kernel-threads, processA is in a new thread
		
		//... Do something ...
	
	} //When forking goes out of scope, it waits for all the forked processes to finish
	
	//end NEW
	@endcode
*
*	The new csp::ScopedForking class is a little different, but ultimately it is easier and nicer
*	than the old mechanism.  This way you can't forget to synchronise on the barrier and wait for the processes, even if an
*	exception is thrown.
*
*	@section mig-channel-ends Channel Ends
*
*	As in C++CSP v1.x, channels are still used via their ends.  Chanout is much the same as it used to be,
*	except that the ParallelComm functionality is gone (see the next section), and instead of the
*	old noPoisonClone() member function, there is a stand-alone csp::NoPoison method that does the same thing.
*	These changes also apply to the channel input ends.
*
*	C++CSP v1.x had a single channel input-end type; Chanin.  Unbuffered unshared channels supported all
*	its functions, but buffered channels did not support extended input and channels with shared reading ends
*	did not support alting.  If you tried to use unsupported functionality, a FunctionNotSupportedException was thrown.
*	This has now been mixed through two major changes:
*	- Extended input is now possible on all channels, including buffered channels.  For details on the semantics,
*	refer to the documentation for specific buffers.
*	- There are now two channel-end types; Chanin and AltChanin.  As you would expect, the former does
*	not support alting, but the latter does.  
*
*	If you use a channel-end in an alt, you will need to change the type from Chanin to AltChanin.
*	You will get compile errors otherwise -- complaining about the lack of an inputGuard() method
*	(although this method itself has now changed -- see the section on @ref mig-alting "Alting" below).
*
*	@section mig-parallel ParallelComm
*
*	The ParallelComm system in C++CSP v1.x was an efficient way of performing parallel communications
*	without starting new processes, that took advantage of an easy way of implementing it in the user-threaded 
*	C++CSP v1.x run-time.
*
*	The changes made to C++CSP to support kernel-threads in C++CSP2 meant that this system could no
*	longer be retained.  Although this does help make the library similar, it will make simple parallel
*	communications more cumbersome.  The best way to mimic the functionality is to use the 
*	csp::common::WriteOnceProcess and csp::common::ReadOnceProcess processes:
	@code
	//in and out are Chanin<int> and Chanout<int> respectively
	int inInt,outInt;
	outInt = 9;
	
	//begin OLD:
	ParallelComm pc(in.parIn(&inInt),out.parOut(&outInt));
	pc.communicate();
	//end OLD
	
	//begin NEW
	Run(InParallelThisThread
		(new ReadOnceProcess<int>(in,&inInt))
		(new WriteOnceProcess<int>(out,&outInt))
	);
	in.checkPoison();
	out.checkPoison();
	//end NEW
	@endcode
*	
*	The checkPoison() calls are important.  If the old ParallelComm encountered poison, it would throw
*	a PoisonException.  Running the sub-processes as above will not detect poison; if the processes
*	do attempt to use poisoned channels, they will fail and stop.  To find out if the channels are now
*	poisoned, you must check the poison manually.  Even then, you will not know using the above
*	code whether or not the channel communications succeeded before the poison or not.  
*
*	There are other ways of implementing the same functionality as ParallelComm.  One option would
*	be to perform the communications sequentially.  This would make things much easier, but it is
*	not always possible (this could lead to deadlock).  Another option, if you do not mind some buffering,
*	is to use buffered channels sequentially, or to use a csp::common::Id process to achieve a similar
*	effect.
*
*	@section mig-channels Channels
*
*	The channels themselves are broadly similar to C++CSP v1.x.  The return types of the reader() methods
*	have changed to AltChanin in some places (as a consequence of the changes to channel-ends mentioned previously).
*	The noPoisonReader()/noPoisonWriter() methods have also gone:
	@code
	One2OneChannel<int> c;
	
	Chanin<int> in = c.noPoisonReader(); //OLD
	
	Chanin<int> in = NoPoison(c.reader()); //NEW
	@endcode
*
*	The names of the buffered channels have changed, from One2OneChannelX to BufferedOne2OneChannel.
*	The original names were actually the result of a mix-up but were never changed, until now.
*	Other changes to the buffered channels are detailed in the @ref mig-buffers "Buffered Channels" section below.
*	
*	@section mig-buffers Buffered Channels
*
*	As mentioned above, the names of the buffered channels have changed.  The channels are now:
*	- csp::BufferedOne2OneChannel
*	- csp::BufferedOne2AnyChannel
*	- csp::BufferedAny2OneChannel
*	- csp::BufferedAny2AnyChannel
*
*	The names of the channel buffers have always changed.  Previously - in line with JCSP - the channel buffers
*	were derived from the ChannelDataStore class.  Copies of buffers were obtained by calling the
*	clone() method of the buffer.  
*
*	In C++CSP2, channel buffers now derive from csp::ChannelBuffer.  The interface has changed to
*	be more flexible and support new features - this is only of concern if you previously implemented
*	your own buffers.  Instead of a clone() method, copies of buffers are obtained from a channel-buffer factory.
*
*	@code
	//begin OLD:
	Buffer<int> buffer;
	One2OneChannelX c(buffer),d(buffer);
	//end OLD
	
	//begin NEW:
	FIFOBuffer<int>::Factory bufferFactory;
	BufferedOne2OneChannel c(bufferFactory), d(bufferFactory);
	//end OLD
	@endcode
*
*	You will note that the buffer names have also changed.  Buffer has become csp::FIFOBuffer and InfiniteBuffer
*	is now csp::InfiniteFIFOBuffer.  ZeroBuffer has gone (use an unbuffered channel instead) and
*	csp::OverwritingBuffer has been added.
*
*	The other major change to buffered channels is that extended input on buffered channels is now supported.
*	The behaviour is roughly intuitive - in the non-overwriting FIFO buffers, the item of data is read
*	from the buffer at the beginning of the extended input but not removed until the end of the extended input.
*	However for the overwriting buffer (to prevent the writer ever blocking), the data is effectively
*	removed at the beginning of the extended input.
*
*	@section mig-ext Extended Input
*
*	As noted above, extended input is now available on all channels - both buffered and unbuffered.
*	The mechanism for using it has also changed.  Previously, an awkward extInput() function was offered
*	for performing extended input.  This has been changed to use a new scoped class:
	@code
	//begin OLD:
	class MyProcess
	{
		Chanin<int> in;
		Chanout<int> out;
		int n;
	public:
		void extendedAction()
		{
			out << n;
		}
	protected:
		void run()
		{
			while(true)
			{
				in.extInput(&n,this,&MyProcess::extendedAction);
			}
		}
	};
	//end OLD
	
	//begin NEW:
	class MyProcess
	{
		Chanin<int> in;
		Chanout<int> out;		
	protected:
		void run()
		{
			while(true)
			{
				int n;	
				{
					ScopedExtInput<int> extInput(in,&n);
					out << n;
				} //extended input finishes at end of scope
			}
		}
	};
	//end NEW
	@endcode
*
*	Anyone who had to use the old method will appreciate the comparative ease of this new version.
*	No more awkward member functions or static functions; the code is inline where you would want
*	it to be.  This method is also exception-safe; in the case of an exception being thrown during
*	an extended input (for example, poison), the extended input is still ended safely.
*	More details are available on the csp::ScopedExtInput page.
*
*	@section mig-barrierbucket Barriers and Buckets
*	
*	Buckets are effectively identical to their C++CSP v1.x version.  Barriers, however, have
*	changed significantly.  Previously barriers were used like buckets; processes wanting to use
*	the barrier held a pointer to it, and used it accordingly.  C++CSP2 introduces the idea of
*	barrier-ends, which aids safe usage of barriers.
*
*	Barrier-ends (csp::BarrierEnd) are always "encased" in a csp::Mobile wrapper:
*	@code
	//begin OLD:
	Barrier barrier;
	barrier.enroll();
	barrier.sync();
	barrier.resign();
	//end OLD
	
	//begin NEW:
	Barrier barrier;
	Mobile<BarrierEnd> barrierEnd(barrier.end());
	barrierEnd.enroll();
	barrierEnd.sync();
	barrierEnd.resign();
	//end NEW
	@endcode
*	You may also want to read about the csp::ScopedBarrierEnd, which offers functionality in a similar way
*	to csp::ScopedForking and csp::ScopedExtInput.  Barrier-ends can be constructed as non-enrolled
*	(csp::Barrier::end()) or enrolled (csp::Barrier::enrolledEnd()).  If you destroy an enrolled barrier-end,
*	try to sync on a non-enrolled barrier end or a barrier is destroyed while processes are still enrolled
*	a csp::BarrierError will be thrown.  This is not just to annoy the programmer - these uses of barriers
*	mean that your program has a bug in it!
*
*	Processes that used to take a Barrier* parameter to their constructor should now take a Mobile<BarrierEnd>
*	parameter.
*
*	@section mig-alting Alting
*
*	Alting has also changed in a small but very significant way since C++CSP v1.x.  In JCSP, you
*	use channel guards in an alternative, and when the index of the chosen guard is returned, it is
*	the programmer's responsibility to perform the channel input.  This decision was reversed in C++CSP v1.x
*	to make the channel guards perform the input automatically on the chosen guard.  This only served
*	to confuse users of the library and also made using an extended guard particularly annoying.
*
*	I have now reversed the situation in C++CSP2, returning to the JCSP method.  I realise that this 
*	will be very confusing to existing C++CSP v1.x users, but I think this tough transition is for the best.
*	It will be less confusing for new users and allows extended inputs to be done in a nice way.  Existing
*	alting code will break because the inputGuard method no longer takes a parameter, so you will
*	be able to see where the code needs changing.  You must removed the parameter, and instead manually
*	perform the input inside the switch statement.  
*
*	For example:
*	@code
	//begin OLD:	
	class MyProcess
	{
		Chanin<int> in0,in1;
		Chanout<int> out;
		int n0,n1;
	public:
		static void extInput(CSProcess* proc)
		{
			MyProcess* myproc = (MyProcess*)proc;
			myproc->out << myproc->n1; 
		}
	protected:
		void run()
		{			
			Alternative alt(
				in0.inputGuard(&n0),
				in1.extInputGuard(&extInput,&n1),
				new RelTimeoutGuard(csp::Seconds(1)),
			NULL);
			
			while (true)
			{
				switch (alt.priSelect())
				{
					case 0: //no need to perform input
						out << n0;
						break;
					case 1: //no need to perform input
						//The real code was in the extended action
						break;
					case 2: //timeout
						out << -1;
						break;
				}
			}
		}
	};
	//end OLD
	
	//begin NEW:
	class MyProcess
	{
		AltChanin<int> in0,in1; //Now AltChanin instead of Chanin
		Chanout<int> out;		
	protected:
		void run()
		{
			int n;
			Alternative alt(  boost::assign::list_of<Guard*>
				(in0.inputGuard()) 
				(in1.inputGuard()) 
				(new RelTimeoutGuard(csp::Seconds(1)))  
			);
			
			while (true)
			{
				switch (alt.priSelect())
				{
					case 0: //must perform input:
						in0 >> n;
						out << n;
						break;
					case 1: //must perform extended input:
						{
							ScopedExtInput<int> extInput(in1,&n);
							out << n;
						}
						break;
					case 2: //timeout
						out << -1;
						break;
				}
			}
		}
	};
	//end NEW
	@endcode
*
*	You will notice a few of the other changes; AltChanin must be used for alting channel-ends,
*	instead of Chanin.  The guard list that is passed to the Alternative is no longer the clumsy
*	NULL-terminated vararg list, but instead uses the cleaner boost::assign library.
*	Guards for extended inputs are now identical to normal input guards - you simply choose
*	in the switch statement whether to perform an input or an extended input.  You can even vary it
*	each time you perform an alt, if you wanted.
*	As before, the Alternative takes care of deleting the guards itself.
*
*	@section mig-networking Networking
*
*	C++CSP v1.3.x had built-in networking support.  This is currently being reconsidered/redeveloped for
*	C++CSP2, but is not yet available.
*
*	@section mig-misc Miscellaneous Other Features
*
*	C++CSP v1.x had a few other miscellaneous features, in particular logging and channel bundles.
*	Logging, which was always a rather strange and useless feature, has now been removed from the library.
*	I am not aware that anyone had much use for it.  Channel bundles used some flashy templating 
*	but I do not know if anyone used them much.  If you did use them, and would like them re-added to
*	C++CSP2, please let me know.  However, for the time being, I have omitted them in the interests of
*	keeping the library simple.
*/
