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

namespace csp
{		
	class SequentialHelper;
	class ParallelHelper;
	class SequentialHelperOneThread;
	class ParallelHelperOneThread;
	class DeadlockError;
	
	class ThreadCSProcess : private internal::Process, private internal::Primitive
	#ifdef CPPCSP_DOXYGEN
		//For the public docs, so the users know it can't be copied
		, public boost::noncopyable
	#endif
	{
	private:
		Mobile<BarrierEnd> finalBarrier;
		void startInNewThread();
		
		virtual void runProcess();
		virtual void endProcess();	
		
		static void StartAll(std::list<ThreadCSProcess*>::const_iterator begin, std::list<ThreadCSProcess*>::const_iterator end);
	protected:
		
		/**
		*	You must implement this function to provide the code for your process.
		*
		*	When the run method finishes, the process will terminate.
		*
		*	You should not let an uncaught exception cause the end of this function.  If it derives from std::exception,
		*	it will be caught (although this behaviour should not be relied upon) but otherwise undefined behaviour will result.
		*/
		virtual void run() = 0;
		
	
		/**
		*	Allows the stack size to be specified.  For information on stack sizes, please read the stacks section
		*	of the @ref processes "Processes" page.
		*/
		explicit ThreadCSProcess(unsigned stackSize = 1048576);
	
		inline virtual ~ThreadCSProcess()
		{
			if (finalBarrier)
			{
				finalBarrier->resign();
			}
		}
		
		#ifndef CPPCSP_DOXYGEN
		friend class CSProcess;
		friend class ScopedForking;
		friend class DeadlockError;
		friend class internal::TestInfo;
		
		friend void Run(const ParallelHelper&);
		friend void Run(const ParallelHelperOneThread&);		
		friend void Run(const SequentialHelper&);
		friend void Run(const SequentialHelperOneThread&);
		friend void Run(ThreadCSProcess*);
		friend void RunInThisThread(CSProcess*);
		friend void RunInThisThread(const ParallelHelperOneThread&);		
		friend void RunInThisThread(const SequentialHelperOneThread&);
		#endif //CPPCSP_DOXYGEN
	};
	
	class CSProcess : public ThreadCSProcess	
	{
	private:		
		//bool useKernelThread;
		
		void startInThisThread();
		
		//Starts this process, and all the ones chained on from it
		//void startAll();
		
		static void StartAllInThisThread(std::list<CSProcessPtr>::const_iterator begin, std::list<CSProcessPtr>::const_iterator end);				
		
	protected:
		
		/**
		*	Allows the stack size to be specified.  For information on stack sizes, please read the stacks section
		*	of the @ref processes "Processes" page.
		*/
		inline explicit CSProcess(unsigned _stackSize = 1048576) : ThreadCSProcess(_stackSize) {}
	
	public:		
		
		#ifndef CPPCSP_DOXYGEN
		friend class internal::TestInfo;
		friend void Run(const ParallelHelperOneThread&);
		friend void Run(const SequentialHelperOneThread&);
		friend void RunInThisThread(CSProcess* process);
		friend void RunInThisThread(const ParallelHelperOneThread&);		
		friend void RunInThisThread(const SequentialHelperOneThread&);		
		friend class ScopedForking;
		#endif //CPPCSP_DOXYGEN
	};
	
	typedef ThreadCSProcess* ThreadCSProcessPtr;
	typedef CSProcess* CSProcessPtr;		
	
	/**
	*	@defgroup processes Processes
	*
	*	A process in C++CSP2 is the basic unit of code.  A rough analogue (although not to be taken too
	*	literally) is that a process is to C++CSP what a function is to C, and what a class is to Java.
	*	A process is a subclass of CSProcess or ThreadCSProcess, that implements the pure-virtual run() method
	*	to provide the code for the process.
	*
	*	Processes can then be run in different ways.  More information on this can be found on the @ref run "Running Processes"
	*	page.	
	*
	*	<b>Stack Sizes:</b>
	*
	*	If you wish, you can provide a stack size to the constructor of CSProcess or ThreadCSProcess.  The exact use
	*	of this value is dependent on the underlying Operating System; however it is guaranteed to be a minimum available
	*	stack size.  
	*
	*	The default minimum stack size is one megabyte.  This is the default maximum stack size on Windows, and should
	*	be a safe maximum stack size.  However, most users will not be interested in increasing this size, but rather in
	*	decreasing it.  If this memory was actually all allocated (which on some systems it could be), this would require
	*	one megabyte of stack size per process.  On a machine with one gigabye of memory and two gigabyte of swap space,
	*	this would give a maximum of less than 3,000 processes.
	*
	*	A temptation might be to reduce it as low as possible.  On systems where the minimum could drop so low, allocating
	*	a four kilobyte stack could allow 256 times as many processes - over 700,000 processes on the machine described above.
	*	However, this stack size would be very dangerous on most systems.  Not only do you have to allow for your own stack usage,
	*	but any system calls or calls to library functions might use up a large amount of stack space, especially if the call
	*	might be recursive.
	*
	*	Unfortunately there is no magic formula or easy estimation to work out what a safe small stack size is.  If you do
	*	use many processes, do not specify a stack size and stick with one megabyte.  If you do need extra performance - 64 kilobytes
	*	usually seems to work fine for me for processes making system and library calls, and 8 kilobytes seems to be workable for very simple processes (such as Id).
	*	To help keep your stack usage low in applications where you need to shrink the stack as much as possible, 
	*	do not allocate large data structures on the stack (something that is not good practice anyway).
	*	You'll notice that all the common processes such as Id put their data in class variables rather than on the stack. 
	*	Allocating classes on the heap with new/delete is the other obvious method of avoiding over-use of the stack.  
	*
	*	Do not become over-zealous in avoiding the stack - this practice is only necessary when the memory allocation of the object
	*	is many kilobytes.  Usually, the underlying storage of collection classes (such as vector, list, map) is 
	*	on the heap, so such objects can safely be placed on the stack.  The only likely use of a lot of memory would be
	*	large arrays, or objects containing large arrays.
	*
	*	<b>C++CSP v1.x Compatibility:</b>
	*
	*	The main change to processes since C++CSP v1.x is the new availablity of kernel threading.  This is also dealt with
	*	in detail in the page on @ref run "Running Processes".  ThreadCSProcess did not exist in v1.x and instead all
	*	processes inherited from CSProcess.  These processes should almost all remain that way, unless you now want to 
	*	change them to <i>always</i> running in their own thread, in which case inherit from ThreadCSProcess instead.  If
	*	you simply want to run the process in a new thread in a particular instance, leave it untouched and read up on
	*	running it as a new thread on the @ref run "Running Processes" page.
	*
	*	The other change is that in C++CSP v1.x, CSProcess contained methods for sleeping, yielding and forking.
	*	Forking is now taken care of in ScopedForking, whereas sleeping and yielding use the global methods CPPCSP_Yield(),
	*	CPPCSP_SleepUntil() and CPPCSP_SleepFor() (named to avoid conflict or confusion with similar OS functions).
	*	Previously, using the CSProcess methods from a function called by a process was very difficult, so these global
	*	functions make the user's life easier, and de-clutter the CSProcess (and now ThreadCSProcess) class.
	*
	*	@{
	*/	
	
	/** @class CSProcess
	*	The base class for processes, that allows processes to be run in a new thead or the same thread.
	*
	*	You should sub-class this class in order to implement the run() method.  In most cases you should sub-class
	*	this class rather than ThreadCSProcess - see the notes in the documentation for ThreadCSProcess for when you should
	*	use that class.
	*
	*	This inheritance structure may seem strange at first (CSProcess inheriting from ThreadCSProcess).  The practical 
	*	result is that ThreadCSProcess, or any child/grandchild(via CSProcess) of it, can be run in a new thread, whereas only
	*	CSProcess and its sub-classes can be added to the current thread.  For more information, see the page
	*	on @ref run "Running Processes".
	*
	*	More information on processes is available on the @ref processes "Processes" page
	*	and the @ref tut-processes "Processes" section of the guide.
	*
	*	@see ThreadCSProcess
	*	@see @ref run "Running Processes"
	*	@see @ref processes "Processes"
	*	@see @ref tut-processes "Processes section of the guide"
	*/
		
	/** @class ThreadCSProcess
	*	A direct sub-class of ThreadCSProcess (except of course CSProcess) is a process that will always be started in a new
	*	thread.  Processes then spawned by a ThreadCSProcess can either themselves be started in new threads, or
	*	started in the same thread.  See the @ref run "Running Processes" page for more information.
	*
	*	This class is intended for use to wrap around blocking system calls.  For example, a process that reads in a file and then
	*	sends the result over a channel and exits is a good candidate for being a sub-class of ThreadCSProcess; it definitely
	*	makes blocking system calls and so should go in its own thread to avoid interfering with processes in other threads.
	*
	*	For the most part you should inherit from CSProcess, not this class.  Only inherit directly from ThreadCSProcess 
	*	if you are making blocking calls.
	*
	*	More information on processes is available on the @ref processes "Processes" page
	*	and the @ref tut-processes "Processes" section of the guide.
	*
	*	@see CSProcess
	*	@see @ref run "Running Processes"
	*	@see @ref processes "Processes"
	*	@see @ref tut-processes "Processes section of the guide"
	*/
	
	/** @} */ //end of group
	
}//namespace csp
