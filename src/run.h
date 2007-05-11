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

#include <list>

namespace csp
{

class ParallelHelper;
class ParallelHelperOneThread;
class SequentialHelper;

class RunHelper
{
public:
	/**
	*	Gets a CSProcess that will run the contained processes 
	*/
	virtual CSProcessPtr process() const = 0;
	virtual ~RunHelper() {}
};

class ParallelHelper : public RunHelper
{
public:
	std::list<ThreadCSProcessPtr> processList;
	
	/** Constructor, that initialises the process list with the specified process */
	inline explicit ParallelHelper(ThreadCSProcessPtr p)
	{
		processList.push_back(p);
	}
	/** @overload */
	inline explicit ParallelHelper(const RunHelper& h)
	{
		processList.push_back(h.process());
	}	
	
	/** Constructor, that initialises the process list using the specified iterators */	
	template <typename ITERATOR>
	inline ParallelHelper(ITERATOR begin,ITERATOR end)
		:	processList(begin,end)
	{		
	}
	
	/** Adds the given process to the process list and returns *this */
	inline ParallelHelper& with(ThreadCSProcessPtr p)
	{
		processList.push_back(p);
		return *this;
	}
	
	/** @overload */
	inline ParallelHelper& with(const RunHelper& p)
	{
		processList.push_back(p.process());
		return *this;
	}
	
	/** Adds the given processes in the ParallelHelper to the process list and returns *this */
	inline ParallelHelper& with(const ParallelHelper& p)
	{
		for (std::list<ThreadCSProcessPtr>::const_iterator it = p.processList.begin();it != p.processList.end();it++)
		{
			processList.push_back(*it);
		}
		return *this;
	}
	
	/** Adds the given process/helper to the process list and returns *this */
	template <typename T>
	inline ParallelHelper& operator()(T t)
	{
		return with(t);
	}	
	
	CSProcessPtr process() const;
};

class ParallelHelperOneThread : public RunHelper
{
public:
	std::list<CSProcessPtr> processList;
		
	/** Constructor, that initialises the process list with the specified process */	
	inline explicit ParallelHelperOneThread(CSProcessPtr p)
	{
		processList.push_back(p);
	}
	
	/** @overload */
	inline explicit ParallelHelperOneThread(const RunHelper& h)
	{
		processList.push_back(h.process());
	}
	
	/** Constructor, that initialises the process list using the specified iterators */	
	template <typename ITERATOR>
	inline ParallelHelperOneThread(ITERATOR begin,ITERATOR end)
		:	processList(begin,end)
	{		
	}
	
	/** Adds the given process to the process list and returns *this */
	inline ParallelHelperOneThread& with(CSProcessPtr p)
	{
		processList.push_back(p);
		return *this;
	}
	
	/** @overload */
	inline ParallelHelperOneThread& with(const RunHelper& p)
	{
		processList.push_back(p.process());
		return *this;
	}
	
	/** Adds the given processes in the ParallelHelperOneThread to the process list and returns *this */
	inline ParallelHelperOneThread& with(const ParallelHelperOneThread& p)
	{
		for (std::list<CSProcessPtr>::const_iterator it = p.processList.begin();it != p.processList.end();it++)
		{
			processList.push_back(*it);
		}
		return *this;
	}
	
	/** Adds the given process/helper to the process list and returns *this */
	template <typename T>
	inline ParallelHelperOneThread& operator()(T t)
	{
		return with(t);
	}	
	
	CSProcessPtr process() const;
};

class SequentialHelper : public RunHelper
{
public:
	std::list<ThreadCSProcessPtr> processList;
	
	/** Constructor, that initialises the process list with the specified process */
	inline explicit SequentialHelper(ThreadCSProcessPtr p)
	{
		processList.push_back(p);
	}
	
	/** @overload */
	inline explicit SequentialHelper(const RunHelper& h)
	{
		processList.push_back(h.process());
	}
	
	/** Constructor, that initialises the process list using the specified iterators */	
	template <typename ITERATOR>
	inline SequentialHelper(ITERATOR begin,ITERATOR end)
		:	processList(begin,end)
	{		
	}	
	
	/** Adds the given process to the process list and returns *this */
	inline SequentialHelper& with(ThreadCSProcessPtr p)
	{
		processList.push_back(p);
		return *this;
	}
	
	/** @overload */
	inline SequentialHelper& with(const RunHelper& p)
	{
		processList.push_back(p.process());
		return *this;
	}
	
	/** Adds the given processes in the SequentialHelper to the process list and returns *this */
	inline SequentialHelper& with(const SequentialHelper& p)
	{
		for (std::list<ThreadCSProcessPtr>::const_iterator it = p.processList.begin();it != p.processList.end();it++)
		{
			processList.push_back(*it);
		}
		return *this;
	}
	
	/** Adds the given process/helper to the process list and returns *this */
	template <typename T>
	inline SequentialHelper& operator()(T t)
	{
		return with(t);
	}
	
	CSProcessPtr process() const;	
};

class SequentialHelperOneThread : public RunHelper
{
public:
	std::list<CSProcessPtr> processList;
	
	/** Constructor, that initialises the process list with the specified process */
	inline explicit SequentialHelperOneThread(CSProcessPtr p)
	{
		processList.push_back(p);
	}
	
	/** @overload */
	inline explicit SequentialHelperOneThread(const RunHelper& h)
	{
		processList.push_back(h.process());
	}
	
	/** Constructor, that initialises the process list using the specified iterators */	
	template <typename ITERATOR>
	inline SequentialHelperOneThread(ITERATOR begin,ITERATOR end)
		:	processList(begin,end)
	{		
	}	
	
	/** Adds the given process to the process list and returns *this */
	inline SequentialHelperOneThread& with(CSProcessPtr p)
	{
		processList.push_back(p);
		return *this;
	}
	
	/** @overload */
	inline SequentialHelperOneThread& with(const RunHelper& p)
	{
		processList.push_back(p.process());
		return *this;
	}
	
	/** Adds the given processes in the SequentialHelperOneThread to the process list and returns *this */
	inline SequentialHelperOneThread& with(const SequentialHelperOneThread& p)
	{
		for (std::list<CSProcessPtr>::const_iterator it = p.processList.begin();it != p.processList.end();it++)
		{
			processList.push_back(*it);
		}
		return *this;
	}
	
	/** Adds the given process/helper to the process list and returns *this */
	template <typename T>
	inline SequentialHelperOneThread& operator()(T t)
	{
		return with(t);
	}
	
	CSProcessPtr process() const;	
};

class ScopedForking : public boost::noncopyable
{
private:
	Barrier barrier;
	Mobile<BarrierEnd> end;
public:
	
	/**
	*	The default constructor.
	*/
	inline ScopedForking()
	{		
		end = barrier.end();
		end->enroll();
	}
	
	/**
	*	The destructor.  The destruction of this object will cause a synchronization with all the forked processes.
	*	That is, the destruction of this object will block the process until all its forked processes have finished.	
	*/
	inline ~ScopedForking()
	{		
		end->sync();		
		end->resign();
		end.blank();
	}
	
	/**
	*	Variants of forkInThisThread() are provided such that forkInThisThread() can be used in any manner that
	*	RunInThisThread() can be.  However, whereas RunInThisThread() does not return until the processes have all
	*	finished, forkInThisThread() returns after starting the processes.
	*/	
	void forkInThisThread(CSProcessPtr);
	
	/** @overload */
	template <typename ITERATOR>
	inline void forkInThisThread(ITERATOR _begin, ITERATOR _end)
	{
		while (_begin != _end)
		{
			try
			{
				forkInThisThread(*_begin++);
			}
			catch (OutOfResourcesException&)
			{
				while (_begin != _end)
				{
					delete *_begin++;
				}
				throw;
			}
		}
	}
	/** @overload */
	void forkInThisThread(const ParallelHelperOneThread&);
	/** @overload */
	void forkInThisThread(const SequentialHelperOneThread&);

	/**
	*	Variants of fork() are provided such that fork() can be used in any manner that
	*	Run() can be.  However, whereas Run() does not return until the processes have all
	*	finished, fork() returns after starting the processes.
	*/	
	void fork(ThreadCSProcessPtr);	
	
	/** @overload */
	template <typename ITERATOR>
	inline void fork(ITERATOR _begin, ITERATOR _end)
	{
		while (_begin != _end)
		{
			try
			{
				fork(*_begin++);
			}
			catch (OutOfResourcesException&)
			{
				while (_begin != _end)
				{
					delete *_begin++;
				}
				throw;
			}
		}
	}
	
	#ifndef CPPCSP_DOXYGEN
	//Not important in the docs that this function has a specialised version for optimisation
	/** @overload */
	void fork(const ParallelHelper&);
	#endif
	/** @overload */
	void fork(const RunHelper&);
	
};



/**	@defgroup run Running Processes
*
*	@section runpageintro A Brief Introduction to Threading
*	
*	Underneath C++CSP2, there is a threading system that uses both kernel-threads and user-threads.  User-threads
*	live inside kernel-threads; that is, kernel-threads can contain multiple user-threads.
*	This may not mean much to programmers who are not familiar with threading, but there are four important points:
*
*	-# Two kernel-threads can execute in parallel on two different CPUs (or cores), but two user-level threads inside the same 
*		kernel-thread can never execute in parallel on different CPUs or cores. 
*	-# A user-thread will only switch to a sibling user-thread when it performs a channel communication, uses some other
*		C++CSP2 primitive (barriers, buckets, etc), or makes an explicit CPPCSP_Yield() call.
*	-# If a user-thread makes a function call to the operating system, its sibling user-threads will be unable to execute 
*		until the call has returned.  For example, if one user-thread reads from a socket, its sibling user-threads will 
*		not be able to do anything until this socket-read has returned. 
*	-# User-threads are much faster and usually more scalable than kernel-threads.  With simple communication-bound processes,
*		programs based on user-threads are usually around ten times faster than the same program using kernel-threads.  This is only
*		for communication of course - the actual processing code inside the process will run at the same speed regardless, but
*		the communications between processes are faster between user-threads.
*
*	Because kernel-threads are easier to understand C++CSP2 uses kernel-level threads by default (especially because of
*	point 3 above) and because it fits the user's intuition that running parallel processes on a multi-core machine will
*	take advantage of the multiple cores (see point 1 above).
*	
*	The reason for providing user-threads at all is the last point on the list (point 4).  Where performance or scalability is an issue, user-threads
*	provide a way to improve performance over kernel-threading.  So users who want to run simulations with tens or hundreds
*	of thousands of processes will want to use a mixture of user-threads and kernel-threads, and will also want to read
*	the section on stack sizes on the @ref processes "Processes" page.
*
*	Wherever you see thread used on this page without a kernel- or user- qualifier, it means kernel-thread.
*
*	@section runpagecppcsp Running Processes
*
*	Processes can be run in two main ways in C++CSP2; using a Run() call or using forking (with the ScopedForking class).
*	The Run() calls wait until all the sub-processes have finished before returning, whereas forking is used when you
*	wish to start a new process, but continue running the current process alongside.
*
*	Both methods delete the process when the process has finished.  Processes should always be allocated using the new operator,
*	never on the stack.  You should not delete processes yourself, and you should never attempt to run the same process
*	twice (neither sequentially nor in parallel).
*
*	If not enough resources (either memory or OS resources) are available to create a thread, an OutOfResourcesException
*	will be thrown.  If you have made a call to spawn many processes, then the processes that have already started will
*	stay running, but the processes that have not yet been run will be deleted.  After the first failure, no further attempt
*	is made to run any of the other processes.
*
*	If this error does occur, you will probably be left with half of your process network running, and half of it unstarted,
*	which will probably cause your program to deadlock.  You should probably attempt to poison all channels.  This error
*	is unlikely to occur unless you are running a very large program; common Windows and Linux machines should allow over
*	a thousand processes (kernel-thread or user-thread) to be run in parallel without a problem.
*
*	Processes can be easily composed, either in parallel or in sequence.  The following sections contain many examples of the use
*	of the Run() function, and shows the result of each.  All the processN variables are assumed to be of type CSProcess*
*	unless specifically noted.
*
*	@section runpageparsimple Simple Parallel Processes 
*
*	@code
*		Run(process0);
*	@endcode
*	The above line runs process0 in a new kernel-thread, only returns when process0 has finished:
*	- Our thread:
*		- Us; waiting
*	- New thread #0:
*		- process0
* 
*	@code
*		RunInThisThread(process0);
*	@endcode
*	The above line runs process0 in a new user-thread within the current kernel-thread, only returns when process0 has finished:
*	- Our thread:
*		- Us; waiting
*		- process0
*
*	@code
*		Run( InParallel (process0) (process1) );
*	@endcode
*	@code
*		Run( InParallel(process0).with(process1) );
*	@endcode
*	@code
*		std::list<CSProcess*> processes;
*		process0.push_back(process0);
*		process1.push_back(process1);
*		Run( InParallel( processes.begin() , processes.end() ) );
*	@endcode
*	All of the above code samples have the same effect.  Some programmers may feel uneasy about the syntax of the first
*	sample (which is inspired by the boost::assign library), so they may wish to use the second, more explicit sample instead,
*	or the third lengthier sample that builds a list of processes first.  The effect is:
*	- Our thread:
*		- Us; waiting
*	- New thread #0:
*		- process0
*	- New thread #1:
*		- process1
*	
*	If you instead wanted to put process0 and process1 in the same new thread, you could do the following:
*	@code
*		Run( InParallelOneThread(process0) (process1) );
*	@endcode
*	The above line would give this arrangement:
*	- Our thread:
*		- Us; waiting
*	- New thread #0:
*		- process0
*		- process1
*	
*	If you instead want to put both process0 and process1 into the current thread, you could do the following:
*	@code
*		RunInThisThread( InParallelOneThread(process0) (process1) );
*	@endcode
*	The above line would provide this:
*	- Our thread:
*		- Us; waiting
*		- process0
*		- process1
*
*	Inquiring minds might wonder as to the effect of:
*	@code
*		RunInThisThread( InParallel (process0) (process1) );
*	@endcode
*	The answer is that the above line will not compile.
*
*	@section runpageseqsimple Simple Sequential Processes
*
*	InSequence can be used instead of InParallel with identical syntax:
*
*	@code
*		Run( InSequence (process0) (process1) );
*	@endcode
*	@code
*		Run( InSequence(process0).with(process1) );
*	@endcode
*	@code
*		std::list<CSProcess*> processes;
*		process0.push_back(process0);
*		process1.push_back(process1);
*		Run( InSequence( processes.begin() , processes.end() ) );
*	@endcode
*	The effect of the above three samples is identical:
*	- Our thread:
*		- Us; waiting
*	- New thread #0:
*		- process0
*	- New thread #1:
*		- process1
*
*	Of course, process1 will not actually be started until process0 has finished - that is the point of composing processes
*	in sequence.  The threading arrangements that result from InSequence will be identical to InParallel
*	(and InSequenceOneThread identical to InParallelOneThread), with this added restriction that each process will only
*	be created when its immediate predecessor has finished.
*
*	@section runpagecomp Complex Composite Processes
*
*	Running processes directly in parallel or in sequence is simple, as seen above.  Sometimes however you will want
*	to nest parallel and sequentially arranged processes; you may want to run (process0 then process1) in parallel with process2 and
*	(process3 then (process4 in parallel with process5)):
*	@code
*		Run( InParallel
*			( InSequence (process0) (process1) )
*			( process2 )
*			( InSequence
*				( process3 )
*				( InParallel (process4) (process5) )
*			)
*		);
*	@endcode
*	The threading arrangement of the above line will be (the numbering of the threads is arbitrary, and does not
*	represent the order they will be started in):
*	- Our thread:
*		- Us; waiting
*	- New thread #0:
*		- process0
*	- New thread #1:
*		- process1
*	- New thread #2:
*		- process2
*	- New thread #3:		
*		- process3
*	- New thread #4:
*		- process4
*	- New thread #5:
*		- process5
*	- New thread #6:
*		- InSequence(process0)(process1) helper process
*	- New thread #7:
*		- InSequence(process3)(...) helper process
*	- New thread #8:
*		- InParallel(process4)(process5) helper process
*
*	You may wish to compress this threading arrangement.  For example, you may want to put process0 in the same thread as process1,
*	and process 4 and process5 in the same thread as each other.  This would be accomplished as follows:
*	@code
*		Run( InParallel
*			( InSequenceOneThread (process0) (process1) )
*			( process2 )
*			( InSequence
*				( process3 )
*				( InParallelOneThread (process4) (process5) )
*			)
*		);
*	@endcode
*	This would give:
*	- Our thread:
*		- Us; waiting
*	- New thread #0:
*		- process0
*		- process1
*		- InSequenceOneThread(process0)(process1) helper process
*	- New thread #1:
*		- process2
*	- New thread #2:		
*		- process3
*	- New thread #3:
*		- process4
*		- process5
*		- InParallelOneThread(process4)(process5) helper process
*	- New thread #4:
*		- InSequence(process3)(...) helper process

*
*	Now consider if you wanted to put process0, process1 and process2 into one thread, and process3, process4 and process5 into
*	another.  That could be done as follows:
*	@code
*		Run( InParallel
*			( InParallelOneThread
*				( InSequenceOneThread (process0) (process1) )
*				( process2 )
*			)
*			( InSequenceOneThread
*				( process3 )
*				( InParallelOneThread (process4) (process5) )
*			)
*		);
*	@endcode
*
*	For completeness, the effect of this:
*	@code
*		Run( InParallel
*			( InParallelOneThread
*				( InSequence (process0) (process1) )
*				( process2A )
*				( process2B )
*			)
*			( InSequenceOneThread
*				( process3A )
*				( process3B )
*				( InParallel (process4) (process5) )
*			)
*		);
*	@endcode
*	will be the following:
*	- Our thread:
*		- Us; waiting
*	- New thread #0:
*		- InSequence(process0)(process1) helper process
*		- process2A
*		- process2B
*	- New thread #1:
*		- process0
*	- New thread #2:		
*		- process1
*	- New thread #3:
*		- process3A
*		- process3B
*		- InParallel(process4)(process5) helper process
*	- New thread #4:
*		- process4
*	- New thread #5:
*		- process5
*
*	All the four arrangements (InParallel, InSequence, InParallelOneThread, InSequenceOneThread ) are associative with themselves.
*	That is, InParallel(x)(InParallel(y)(z)) produces the same threading arrangement as InParallel(x)(y)(InParallel(z)) and
*	InParallel(x)(y)(z) (even taking into account the hidden helper processes).  If you nest an InSequenceOneThread directly inside an InParallelOneThread (or vice versa) then
*	the processes in both will be in the same thread.
*
*	@section runargh ThreadCSProcess vs CSProcess (aka "Argh!  Why won't it compile?")
*
*	One problem with the run system is that it can make compilation errors slightly hard to understand.  Most of the problems
*	come down to one central issue.  The RunInThisThread(), ScopedForking.forkInThisThread(), InParallelOneThread() and
*	In SequenceOneThread() functions cannot be used on threads (for obvious reasons - something that will get run in 
*	a new thread cannot be forced to run in this thread).  This means you cannot pass them a helper from InParallel() or InSequence()
*	and nor can you pass them a ThreadCSProcess (except for children of CSProcess).
*
*	In most cases, processes should be descended from CSProcess not ThreadCSProcess.  But in case you use one that
*	is descended from ThreadCSProcess, you should bear in mind that you cannot use them with one of the calls listed above.
*	Similarly, you cannot nest an InParallel() or InSequence() directly inside an InParallelOneThread() or InSequenceOneThread().
*	You can however, achieve your desired effect by using the helper's process() call.  
* @{
*/

/**
*	Runs the parallel processes in new threads.  This function is usually called thus: Run(InParallel(x)(y)(z));
*/
void Run(const ParallelHelper&);

/**
*	Runs the parallel processes in <b>one new</b> thread.  This function is usually called thus: Run(InParallelOneThread(x)(y)(z));
*/
void Run(const ParallelHelperOneThread&);

/**
*	Runs the processes sequentially in new threads.  This function is usually called thus: Run(InSequence(x)(y)(z));
*/
void Run(const SequentialHelper&);

/**
*	Runs the processes sequentially in <b>one new</b> thread.  This function is usually called thus: Run(InSequenceOneThread(x)(y)(z));
*/
void Run(const SequentialHelperOneThread&);

/**
*	Runs the process in a new thread.
*/
void Run(ThreadCSProcessPtr);

/**
*	Runs the process in this thread.
*/
void RunInThisThread(CSProcessPtr);

/**
*	Runs the parallel processes in this thread.  This function is usually called thus: RunInThisThread(InParallelOneThread(x)(y)(z));
*/
void RunInThisThread(const ParallelHelperOneThread&);
/**
*	Runs the processes sequentially in this thread.  This function is usually called thus: RunInThisThread(InSequenceOneThread(x)(y)(z));
*/
void RunInThisThread(const SequentialHelperOneThread&);

/** @class RunHelper
*
*	The base class for the various helper classes for the Run function.  You are unlikely to need to use this directly.
*/

/** @class ParallelHelper
*	A helper class returned by the InParallel() function.
*/

/** @class ParallelHelperOneThread
*	A helper class returned by the InParallelOneThread() function.
*/

/** @class SequentialHelper
*	A helper class returned by the InSequence() function.
*/

/** @class SequentialHelperOneThread
*	A helper class returned by the InSequenceOneThread() function.
*/

/** Starts an InParallel chain with the given process.  See the top of this page for information on how to use this function. */
inline ParallelHelper InParallel(ThreadCSProcessPtr p) {return ParallelHelper(p); }
/** Starts an InParallel chain with the given helper.  See the top of this page for information on how to use this function. */
inline ParallelHelper InParallel(const ParallelHelper& p) {return p;}
/** @overload */
inline ParallelHelper InParallel(const ParallelHelperOneThread& p) { return ParallelHelper(p); }
/** @overload */
inline ParallelHelper InParallel(const SequentialHelper& p) { return ParallelHelper(p); }
/** @overload */
inline ParallelHelper InParallel(const SequentialHelperOneThread& p) { return ParallelHelper(p); }

/** Starts an InParallel chain with the given processes.  *ITERATOR is expected to resolve to a ThreadCSProcessPtr */
template <typename ITERATOR>
inline ParallelHelper InParallel(ITERATOR begin,ITERATOR end)
{
	return ParallelHelper(begin,end);
}

/** Starts an InParallelOneThread chain with the given process.  See the top of this page for information on how to use this function. */
inline ParallelHelperOneThread InParallelOneThread(CSProcessPtr p) {return ParallelHelperOneThread(p);}
/** @overload */
inline ParallelHelperOneThread InParallelOneThread(const ParallelHelperOneThread& p) {return p;}
/** @overload */
inline ParallelHelperOneThread InParallelOneThread(const SequentialHelperOneThread& p) {return ParallelHelperOneThread(p);}

/** Starts an InParallelOneThread chain with the given processes.  *ITERATOR is expected to resolve to a CSProcessPtr */
template <typename ITERATOR>
inline ParallelHelperOneThread InParallelOneThread(ITERATOR begin,ITERATOR end)
{
	return ParallelHelperOneThread(begin,end);
}


/** Starts an InSequence chain with the given process.  See the top of this page for information on how to use this function. */
inline SequentialHelper InSequence(ThreadCSProcessPtr p) {return SequentialHelper(p);}
/** @overload */
inline SequentialHelper InSequence(const ParallelHelper& p) {return SequentialHelper(p);}
/** @overload */
inline SequentialHelper InSequence(const SequentialHelper& p) {return p;}
/** @overload */
inline SequentialHelper InSequence(const SequentialHelperOneThread& p) {return SequentialHelper(p);}
/** @overload */
inline SequentialHelper InSequence(const ParallelHelperOneThread& p) {return SequentialHelper(p);}

/** Starts an InSequence chain with the given processes.  *ITERATOR is expected to resolve to a ThreadCSProcessPtr */
template <typename ITERATOR>
inline SequentialHelper InSequence(ITERATOR begin,ITERATOR end)
{
	return SequentialHelper(begin,end);
}

/** Starts an InSequenceOneThread chain with the given process.  See the top of this page for information on how to use this function. */
inline SequentialHelperOneThread InSequenceOneThread(CSProcessPtr p) {return SequentialHelperOneThread(p);}
/** @overload */
inline SequentialHelperOneThread InSequenceOneThread(const ParallelHelperOneThread& p) {return SequentialHelperOneThread(p);}
/** @overload */
inline SequentialHelperOneThread InSequenceOneThread(const SequentialHelperOneThread& p) {return p;}

/** Starts an InSequenceOneThread chain with the given processes.  *ITERATOR is expected to resolve to a CSProcessPtr */
template <typename ITERATOR>
inline SequentialHelperOneThread InSequenceOneThread(ITERATOR begin,ITERATOR end)
{
	return SequentialHelperOneThread(begin,end);
}

/** @class ScopedForking
*	A class used to fork processes.  
*
*	Whereas the Run() call runs some processes and waits for them to complete, forking starts some processes but lets the original process
*	continue.  Synchronization between the original process  and the forked processes finishing is carried out later on.  One of the main reasons
*	for this synchronization is that the forked processes usually use channels or other resources provided by the original (parent) process.
*	Therefore the original process should not finish (which would destroy its resources) until the forked processes have themselves completed.
*
*	ScopedForking ensures that this final synchronization is always done, even in the case of an exception being thrown.  It is used as follows:
*	@code
		{
			ScopedForking forking;
			
			forking.forkProcess(new ProcessA);
			
			// ... some other code ...
			
			forking.forkProcess(new ProcessB);
			
			// ... some other code ...
		} //end of block
	@endcode
*
*	At the end of the above block of code, ScopedForking goes out of scope.  At this point, the object will be destroyed, which will cause the 
*	synchronization with the sub-processes (ProcessA and ProcessB) to be completed.
*
*	This method has advantages and disadvantages.  The advantage is that it ensures that this synchronization will happen.  The disadvantage is 
*	that it hides the occurrence of this synchronization.  Looking at that sole brace at the end of the block, it is not clear that it involves 
*	the process making a blocking call.
*
*	You must make sure that all resources (particularly channels and barriers) remain in scoped until the ScopedForking object
*	has been destroyed.  Consider the following:
*	@code
	{
		ScopedForking forking;
		One2OneChannel<int> c,d; //BAD!
		// ... some other code ...
		forking.fork(new Id<int>(c.reader(),d.writer());
		// ... some other code ...
	}
	@endcode
*
*	The above code is very dangerous.  Objects in C++ are destroyed in the reverse order of their construction.
*	Therefore in the above code, the channels will be destroyed before the ScopedForking object.  So at the end
*	of the block the channels (which are being used by the sub-processes) are destroyed, and then we wait
*	for the (now endangered) sub-processes to finish.  Make sure you declare all used channels and barriers
*	before the ScopedForking object.
*
*	This process should always be used as a stack variable.  Using it as a class member variable may have hazardous consequences - if the 
*	destructor of the class is called by a process other than the original process, undefined behaviour will ensue.  In particular, never use
*	this as a member variable in a CSProcess implementation.
*	Similarly, if you ever find a piece of code that holds a pointer to this class, you are almost certainly using it wrongly or inappropriately.
*
*	More information can be found on the @ref scoped "Scoped Classes" page, the @ref run "Running Processes"
*	page and in the @ref tut-forking "Forking Processes" section of the guide.
*
*	@ingroup scoped
*/

/** @} */ //end of group

} //namespace csp
