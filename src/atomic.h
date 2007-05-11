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
	namespace internal
	{
	
	/**@internal
	*	@defgroup atomic The Atomic Functions and Defines
	*
	*	These functions define the atomic operations that C++CSP2 uses for a lot of its algorithms.
	*	Currently they are only defined for x86 and x86-64 processors, and the defines only work
	*	on the GCC and Microsoft Visual C++ compilers.  If you want to port C++CSP2 to your desired
	*	architecture you will need to provide definitions for everything in this group.
	*
	*	The alignment macros are very difficult to deal with on both compilers.  I believe they are usually
	*	unnecessary (the default being to align values to their appropriate boundaries anyway),
	*	but they should give an extra measure of security - assuming they are correct.  It is not 
	*	clear that I have definitely specified an aligned pointer rather than a pointer to an aligned value.
	*
	*	There are many overloaded variants of each function, but there are only really six functions:
	*
	*	AtomicCompareAndSwap:
	*
	*	These functions perform an atomic cmp-swap.
	*	
	*	If *address is equal to compareTo, *address is set to swapOnEqual, and the original value of *address is returned.
	*	If *address is not equal to compareTo, *address is left unchanged, and the value of *address is returned.
	*
	*	This operation is carried out atomically, of course.
	*
	*	AtomicGet:
	*
	*	These functions return the value of *address, using an atomic-read
	*
	*	AtomicPut:
	*
	*	These functions write to *address using an atomic-write
	*
	*	AtomicSwap:
	*	
	*	These functions perform an atomic swap, writing value to *address and returning the original value of *address
	*
	*	AtomicDecrement:
	*
	*	These functions perform an atomic decrement of *address (subtracting 1), and return the new value (NOT the old value)
	*
	*	AtomicIncrement:
	*
	*	These functions perform an atomic increment of *address (adding 1), and return the new value (NOT the old value)
	*	
	*	Implementation Notes:
	*
	*	I considered writing assembly for the atomic reads and writes (gets and puts).  However, this
	*	seems unnecessary - no special instructions are needed for an atomic read or write.  A mov instruction
	*	on the x86 (and x86-64) architecture is atomic as long as the source/destination in memory is aligned.
	*	We know the value is aligned, so it would only be non-atomic if the compiler did not write a pointer
	*	using a single mov instruction.  I cannot imagine an instance where this would not be the case.
	*	If you ever find one, please let me know.
	*
	*	@see csp::internal::SpinMutex
	*	@see csp::internal::PureSpinMutex
	*	@see csp::internal::PureSpinMutex_TTS
	*	@see csp::internal::QueuedMutex
	*	
	*	@{
	*/
	
	
	#ifdef CPPCSP_GCC
		#define __CPPCSP_ALIGNED2_L
		#define __CPPCSP_ALIGNED4_L
		#define __CPPCSP_ALIGNED8_L
		#define __CPPCSP_ALIGNED2_R __attribute__ ((aligned (2)))
		#define __CPPCSP_ALIGNED4_R __attribute__ ((aligned (4)))
		#define __CPPCSP_ALIGNED8_R __attribute__ ((aligned (8)))
	#endif
	
	#ifdef CPPCSP_MSVC
		#define __CPPCSP_ALIGNED2_L __declspec(align(2))
		#define __CPPCSP_ALIGNED4_L __declspec(align(4))
		#define __CPPCSP_ALIGNED8_L __declspec(align(8))
		#define __CPPCSP_ALIGNED2_R
		#define __CPPCSP_ALIGNED4_R
		#define __CPPCSP_ALIGNED8_R

		#ifdef WIN64
			#define SIZEOF_VOIDP 8
		#else
			#define SIZEOF_VOIDP 4
		#endif
	#endif
	
	/**@internal
	*	@def SIZEOF_VOIDP
	*	This define is used to determine the size of a pointer, in bytes, in the preprocessor.
	*
	*	On systems using autoconf/automake, this macro is defined on the compiler command-line by
	*	autoconf.  On Microsoft Visual C++, the SIZEOF_VOIDP define is defined in this file based on whether
	*	or not WIN64 is defined.
	*
	*	SIZEOF_VOIDP is expected to be either 2, 4 or 8.  I do not consider this to be an unreasonable
	*	expectation.
	*/
	
	#ifndef __CPPCSP_ALIGNED4_L
		#error Unknown compiler -- atomic reads/writes may be at risk of not being atomic!
		#error Please see C++CSP documentation (FAQ section) for more information
	#endif
	
	#if SIZEOF_VOIDP == 2
		#define __CPPCSP_ALIGNED_PTR_L __CPPCSP_ALIGNED2_L
		#define __CPPCSP_ALIGNED_PTR_R __CPPCSP_ALIGNED2_R
	#elif SIZEOF_VOIDP == 4
		#define __CPPCSP_ALIGNED_PTR_L __CPPCSP_ALIGNED4_L
		#define __CPPCSP_ALIGNED_PTR_R __CPPCSP_ALIGNED4_R
	#elif SIZEOF_VOIDP == 8
		#define __CPPCSP_ALIGNED_PTR_L __CPPCSP_ALIGNED8_L
		#define __CPPCSP_ALIGNED_PTR_R __CPPCSP_ALIGNED8_R
	#endif
	
	#ifndef __CPPCSP_ALIGNED_PTR_L
		#error Your pointers are of a strange size (not 2, 4 or 8 bytes), or you need to define SIZEOF_VOIDP. 
		#error Please also define __CPPCSP_ALIGNED_PTR to be the appropriate aligned attribute 
	#endif
	
	/**@internal
	*	@def __CPPCSP_ALIGNED_PTR(TYPE)
	*	This macro defines the type of an aligned pointer to TYPE.
	*/
	
	/**@internal
	*	@def __CPPCSP_ALIGNED_VOID_PTR
	*	This macro defines the type of an aligned pointer to void.
	*/
	
	/**@internal
	*	@def __CPPCSP_ALIGNED_DATA_TYPE_PTR
	*	This macro defines the type of an aligned pointer to DATA_TYPE (which is assumed to be defined, probably the argument to a templated class).
	*/
	
	/**@internal
	*	@def __CPPCSP_ALIGNED_USIGN32
	*	This macro defines the type of an aligned usign32 variable (not a pointer).
	*/
	
	/**@internal
	*	@def __CPPCSP_ALIGNED_USIGN32_PTR
	*	This macro defines the type of an aligned pointer to usign32.
	*/

	#define __CPPCSP_ALIGNED_PTR(TYPE) TYPE __CPPCSP_ALIGNED_PTR_L * volatile __CPPCSP_ALIGNED_PTR_R 
	#define __CPPCSP_ALIGNED_VOID_PTR __CPPCSP_ALIGNED_PTR(void)
	#define __CPPCSP_ALIGNED_DATA_TYPE_PTR __CPPCSP_ALIGNED_PTR(DATA_TYPE)
	#define __CPPCSP_ALIGNED_USIGN32 __CPPCSP_ALIGNED4_L usign32 volatile __CPPCSP_ALIGNED4_R
	#define __CPPCSP_ALIGNED_USIGN32_PTR usign32 volatile __CPPCSP_ALIGNED_PTR_L * __CPPCSP_ALIGNED_PTR_R
	#define __CPPCSP_ALIGNED_SIGN32 __CPPCSP_ALIGNED4_L sign32 volatile __CPPCSP_ALIGNED4_R
	#define __CPPCSP_ALIGNED_SIGN32_PTR sign32 volatile __CPPCSP_ALIGNED_PTR_L * __CPPCSP_ALIGNED_PTR_R
	
	
	///@internal See the top of the page for a description of this function	
	inline usign32 AtomicCompareAndSwap(__CPPCSP_ALIGNED_USIGN32_PTR address,usign32 compareTo,usign32 swapOnEqual);

	///@internal See the top of the page for a description of this function
	inline void* _AtomicCompareAndSwap(__CPPCSP_ALIGNED_VOID_PTR * address,void* compareTo,void* swapOnEqual);
		
	///@internal See the top of the page for a description of this function
	template <typename DATA_TYPE>
	inline DATA_TYPE* AtomicCompareAndSwap(__CPPCSP_ALIGNED_DATA_TYPE_PTR * address,DATA_TYPE* compareTo,DATA_TYPE* swapOnEqual)
	{
		return static_cast<DATA_TYPE*>(_AtomicCompareAndSwap(reinterpret_cast<__CPPCSP_ALIGNED_VOID_PTR *>(address),compareTo,swapOnEqual));
	}	
				
	///@internal See the top of the page for a description of this function
	inline void* _AtomicGet(__CPPCSP_ALIGNED_VOID_PTR * address);
	
	///@internal See the top of the page for a description of this function	
	inline usign32 AtomicGet(__CPPCSP_ALIGNED_USIGN32_PTR address);
	
	///@internal See the top of the page for a description of this function	
	template <typename DATA_TYPE>
	inline DATA_TYPE* AtomicGet(__CPPCSP_ALIGNED_DATA_TYPE_PTR * address)
	{
		return static_cast<DATA_TYPE*>(_AtomicGet(reinterpret_cast<__CPPCSP_ALIGNED_VOID_PTR *>(address)));
	}

	///@internal See the top of the page for a description of this function
	inline void _AtomicPut(__CPPCSP_ALIGNED_VOID_PTR * address,void* value);
	
	///@internal See the top of the page for a description of this function	
	inline void AtomicPut(__CPPCSP_ALIGNED_USIGN32_PTR address,usign32 value);
	
	///@internal See the top of the page for a description of this function	
	template <typename DATA_TYPE>
	inline void AtomicPut(__CPPCSP_ALIGNED_DATA_TYPE_PTR * address,DATA_TYPE* value)
	{
		_AtomicPut(reinterpret_cast<__CPPCSP_ALIGNED_VOID_PTR *>(address),value);
	}		
	
	///@internal See the top of the page for a description of this function	
	inline usign32 AtomicDecrement(__CPPCSP_ALIGNED_USIGN32_PTR address);	

	///@internal See the top of the page for a description of this function	
	inline usign32 AtomicIncrement(__CPPCSP_ALIGNED_USIGN32_PTR address);	
		
	///@internal See the top of the page for a description of this function	
	inline void* _AtomicSwap(__CPPCSP_ALIGNED_VOID_PTR * address,void* value);
		
	///@internal See the top of the page for a description of this function	
	inline usign32 AtomicSwap(__CPPCSP_ALIGNED_USIGN32_PTR address,usign32 value);
	
	///@internal See the top of the page for a description of this function		
	template <typename DATA_TYPE>
	inline DATA_TYPE* AtomicSwap(__CPPCSP_ALIGNED_DATA_TYPE_PTR * address,DATA_TYPE* value)
	{
		return static_cast<DATA_TYPE*>(_AtomicSwap(reinterpret_cast<__CPPCSP_ALIGNED_VOID_PTR *>(address),value));
	}
				
				
	//End of group:
	/** @} */
		
	} //namespace internal
} //namespace csp

