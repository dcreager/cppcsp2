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

#ifdef CPPCSP_GCC

	#define CPPCSP_ASM(gcc,msvc) __asm__ ( gcc ) ;
	
	#ifdef CPPCSP_DARWIN
		//Using Mac OS X assembler:
		//Must not have trailing space
		#define LOCK_CMPXCHG "lock/cmpxchg"
		#define LOCK_XCHG "lock/xchg"
	#else
		//Using a recent GNU assembler:
		//Must not have trailing space
		#define LOCK_CMPXCHG "lock cmpxchg"		
		#define LOCK_XCHG "lock xchg"		
	#endif

#else
	#ifdef CPPCSP_MSVC
	
		#define CPPCSP_ASM(gcc,msvc) __asm { msvc } ;
	
	#endif
#endif

#if SIZEOF_VOIDP == 4
#elif SIZEOF_VOIDP == 8
#else
	#error Code is not written to allow pointer sizes other than 4 or 8 bytes 
#endif

template <typename T>
inline T AtomicGet4Bytes(T volatile * address)
{
/*
	T retVal;
#ifdef CPPCSP_MSVC
	#if SIZEOF_VOIDP == 4
		__asm
		{
			mov ebx, address
			mov eax, [ebx] ; The atomic read
			mov retVal, eax
		}
	#elif SIZEOF_VOIDP == 8
		__asm
		{
			mov rbx, address
			mov eax, [rbx] ; The atomic read
			mov retVal, eax
		}
	#endif
#endif

#ifdef CPPCSP_GCC
	#if SIZEOF_VOIDP == 4
		__asm__ 
		(
			"movl %1, %%ebx\n\t"
			"movl (%%ebx), %%eax\n\t"
			"movl %%eax, %0"
			:	"=m"(retVal)
			:	"m"(address)
			:	"%eax","%ebx"
		);
	#elif SIZEOF_VOIDP == 8
		__asm__ 
		(
			"movq %1, %%rbx\n\t"
			"movl (%%rbx), %%eax\n\t"
			"movl %%eax, %0"
			:	"=m"(retVal)
			:	"m"(address)
			:	"%eax","%rbx"
		);	
	#endif
#endif

	return retVal;
*/
	return *address;
}

template <typename T>
inline T AtomicGet8Bytes(T volatile * address)
{
#if SIZEOF_VOIDP == 8
/*
	T retVal;

#ifdef CPPCSP_MSVC
	__asm
	{
		mov rbx, address
		mov rax, [rbx] ; The atomic read
		mov retVal, rax
	}	
#endif

#ifdef CPPCSP_GCC
	__asm__ 
	(
		"movq %1, %%rbx\n\t"
		"movq (%%rbx), %%rax\n\t"
		"movq %%rax, %0"
		:	"=m"(retVal)
		:	"m"(address)
		:	"%rax","%rbx"
	);
#endif
	return retVal;
*/
	return *address;
#else
	//We are attempting to handle 8 bytes when pointers are of size 4, but the function won't be used:
	return *address;
#endif
}

template <typename T>
inline void AtomicPut4Bytes(T volatile * address,T value)
{
/*
#ifdef CPPCSP_MSVC
	#if SIZEOF_VOIDP == 4
		__asm
		{
			mov ebx, address
			mov eax, value
			mov [ebx], eax ; The atomic write
		}
	#elif SIZEOF_VOIDP == 8
		__asm
		{
			mov rbx, address
			mov eax, value
			mov [rbx], eax ; The atomic write
		}
	#endif	
#endif

#ifdef CPPCSP_GCC
	#if SIZEOF_VOIDP == 4
		__asm__ 
		(
			"movl %1, %%ebx\n\t"
			"movl %2, %%eax\n\t"
			"movl %%eax, (%%ebx)\n\t"
			:	"=m"(*(address))
			:	"m"(address),"m"(value)
			:	"%eax","%ebx"
		);	
	#elif SIZEOF_VOIDP == 8
		__asm__ 
		(
			"movq %1, %%rbx\n\t"
			"movl %2, %%eax\n\t"
			"movl %%eax, (%%rbx)\n\t"
			:	"=m"(*(address))
			:	"m"(address),"m"(value)
			:	"%eax","%rbx"
		);	
	#endif
#endif
*/
	*address = value;
}

template <typename T>
inline void AtomicPut8Bytes(T volatile * address,T value)
{
#if SIZEOF_VOIDP == 8
/*
#ifdef CPPCSP_MSVC
	__asm
	{
		mov rbx, address
		mov rax, value
		mov [rbx], rax ; The atomic write
	}
#endif

#ifdef CPPCSP_GCC
	__asm__ 
	(
		"movq %1, %%rbx\n\t"
		"movq %2, %%rax\n\t"
		"movq %%rax, (%%rbx)\n\t"
		:	"=m"(*(address))
		:	"m"(address),"m"(value)
		:	"%eax","%ebx"
	);	
#endif
*/
	*address = value;
#else
	//We are attempting to handle 8 bytes when pointers are of size 4, but the function won't be used:
	*address = value;
#endif
}


void* _AtomicGet(__CPPCSP_ALIGNED_VOID_PTR* address)
{	
#if SIZEOF_VOIDP == 4
	return AtomicGet4Bytes<void*>(address);
#elif SIZEOF_VOIDP == 8
	return AtomicGet8Bytes<void*>(address);
#endif
}

usign32 AtomicGet(__CPPCSP_ALIGNED_USIGN32_PTR address)
{
	return AtomicGet4Bytes<usign32>(address);
}

void _AtomicPut(__CPPCSP_ALIGNED_VOID_PTR * address,void* value)
{
	
	//Assembly, because I want to be sure:
	
#if SIZEOF_VOIDP == 4
	AtomicPut4Bytes<void*>(address,value);
#elif SIZEOF_VOIDP == 8
	AtomicPut8Bytes<void*>(address,value);
#endif
}

void AtomicPut(__CPPCSP_ALIGNED_USIGN32_PTR address,usign32 value)
{
	AtomicPut4Bytes<usign32>(address,value);
}

#ifndef CPPCSP_WINDOWS

// load eax with compare
//lock cmpxchg [pointerToPointer], exchange (in Intel syntax)

inline void* InterlockedCompareExchangePointer(void* volatile* pointerToPointer,void* exchange,void* compare)
{
	void* prevValue;
	
#if SIZEOF_VOIDP == 4
		__asm__ (
			"movl %2,%%eax\n\t"
			"movl %3,%%ecx\n\t"
			"movl %4,%%edx\n\t"
			LOCK_CMPXCHG " %%edx, (%%ecx)\n\t"
			"movl %%eax,%0\n\t"
			:	"=m"(prevValue),"=m"(*(pointerToPointer))
			:	"m"(compare),"m"(pointerToPointer),"m"(exchange)
			:	"%eax", "%ecx", "%edx"
		);

#elif SIZEOF_VOIDP == 8
		__asm__ (
			"movq %2,%%rax\n\t"
			"movq %3,%%rcx\n\t"
			"movq %4,%%rdx\n\t"
			LOCK_CMPXCHG "q %%rdx, (%%rcx)\n\t"
			"movq %%rax,%0\n\t"
			:	"=m"(prevValue),"=m"(*(pointerToPointer))
			:	"m"(compare),"m"(pointerToPointer),"m"(exchange)
			:	"%rax", "%rcx", "%rdx"
		);	
#endif
	
	return prevValue;
}

inline usign32 InterlockedCompareExchange(usign32 volatile* pointerToPointer /*misnomer!*/,usign32 exchange,usign32 compare)
{
	usign32 prevValue;
	
#if SIZEOF_VOIDP == 4
		__asm__ (
			"movl %2,%%eax\n\t"
			"movl %3,%%ecx\n\t"
			"movl %4,%%edx\n\t"
			LOCK_CMPXCHG " %%edx, (%%ecx)\n\t"
			"movl %%eax,%0\n\t"
			:	"=m"(prevValue),"=m"(*(pointerToPointer))
			:	"m"(compare),"m"(pointerToPointer),"m"(exchange)
			:	"%eax", "%ecx", "%edx"
		);

#elif SIZEOF_VOIDP == 8
		__asm__ (
			"movl %2,%%eax\n\t"
			"movq %3,%%rcx\n\t"
			"movl %4,%%edx\n\t"
			LOCK_CMPXCHG " %%edx, (%%rcx)\n\t"
			"movl %%eax,%0\n\t"
			:	"=m"(prevValue),"=m"(*(pointerToPointer))
			:	"m"(compare),"m"(pointerToPointer),"m"(exchange)
			:	"%eax", "%rcx", "%edx"
		);	
#endif
	
	return prevValue;
}

inline void* InterlockedExchangePointer(void* volatile* pointerToPointer,void* exchange)
{
	void* prevValue;
#if SIZEOF_VOIDP == 4
	__asm__
	(
		"movl %2, %%eax\n\t"
		"movl %3, %%ecx\n\t"
		LOCK_XCHG " %%eax, (%%ecx)\n\t"
		"movl %%eax, %0"
		:	"=m"(prevValue),"=m"(*(pointerToPointer))
		:	"m"(exchange),"m"(pointerToPointer)
		:	"%eax", "%ecx"
	);
#elif SIZEOF_VOIDP == 8
	__asm__
	(
		"movq %2, %%rax\n\t"
		"movq %3, %%rcx\n\t"
		LOCK_XCHG "q %%rax, (%%rcx)\n\t"
		"movq %%rax, %0"
		:	"=m"(prevValue),"=m"(*(pointerToPointer))
		:	"m"(exchange),"m"(pointerToPointer)
		:	"%rax", "%rcx"
	);
#endif
	return prevValue;
}

inline usign32 InterlockedExchange(usign32 volatile* pointerToPointer /*misnomer!*/,usign32 exchange)
{
	usign32 prevValue;
#if SIZEOF_VOIDP == 4
	__asm__
	(
		"movl %2, %%eax\n\t"
		"movl %3, %%ecx\n\t"
		LOCK_XCHG " %%eax, (%%ecx)\n\t"
		"movl %%eax, %0"
		:	"=m"(prevValue),"=m"(*(pointerToPointer))
		:	"m"(exchange),"m"(pointerToPointer)
		:	"%eax", "%ecx"
	);
#elif SIZEOF_VOIDP == 8
	__asm__
	(
		"movl %2, %%eax\n\t"
		"movq %3, %%rcx\n\t"
		LOCK_XCHG " %%eax, (%%rcx)\n\t"
		"movl %%eax, %0"
		:	"=m"(prevValue),"=m"(*(pointerToPointer))
		:	"m"(exchange),"m"(pointerToPointer)
		:	"%eax", "%rcx"
	);
#endif
	return prevValue;
}

inline usign32 InterlockedIncrement(usign32 volatile* address)
{
	usign32 newValue;
#if SIZEOF_VOIDP == 4
	__asm__
	(
		"movl %2, %%ecx\n\t"
		"movl (%%ecx), %%eax\n\t"
		"0: movl %%eax, %%edx\n\t"
		"incl %%edx\n\t"
		LOCK_CMPXCHG " %%edx, (%%ecx)\n\t"
		"jnz 0b\n\t"
		"movl %%edx, %0\n\t"
		:	"=m"(newValue),"=m"(*(address))
		:	"m"(address)
		:	"%eax", "%ecx", "%edx"
	);
#elif SIZEOF_VOIDP == 8
	__asm__
	(
		"movq %2, %%rcx\n\t"
		"movl (%%rcx), %%eax\n\t"
		"0: movl %%eax, %%edx\n\t"
		"incl %%edx\n\t"
		LOCK_CMPXCHG " %%edx, (%%rcx)\n\t"
		"jnz 0b\n\t"
		"movl %%edx, %0\n\t"
		:	"=m"(newValue),"=m"(*(address))
		:	"m"(address)
		:	"%eax", "%rcx", "%edx"
	);
#endif
	return newValue;
}


inline usign32 InterlockedDecrement(usign32 volatile* address)
{
	usign32 newValue;
#if SIZEOF_VOIDP == 4
	__asm__
	(
		"movl %2, %%ecx\n\t"
		"movl (%%ecx), %%eax\n\t"
		"0: movl %%eax, %%edx\n\t"
		"decl %%edx\n\t"
		LOCK_CMPXCHG " %%edx, (%%ecx)\n\t"
		"jnz 0b\n\t"
		"movl %%edx, %0\n\t"
		:	"=m"(newValue),"=m"(*(address))
		:	"m"(address)
		:	"%eax", "%ecx", "%edx"
	);
#elif SIZEOF_VOIDP == 8
	__asm__
	(
		"movq %2, %%rcx\n\t"
		"movl (%%rcx), %%eax\n\t"
		"0: movl %%eax, %%edx\n\t"
		"decl %%edx\n\t"
		LOCK_CMPXCHG " %%edx, (%%rcx)\n\t"
		"jnz 0b\n\t"
		"movl %%edx, %0\n\t"
		:	"=m"(newValue),"=m"(*(address))
		:	"m"(address)
		:	"%eax", "%rcx", "%edx"
	);
#endif
	return newValue;
}

#define __CPPCSP_INTERLOCKED_CAST 

#else //CPPCSP_WINDOWS

#ifdef CPPCSP_MINGW
	//MinGW doesn't have the volatile label on the parameters
	#define __CPPCSP_INTERLOCKED_CAST (LONG *)
#else
	#define __CPPCSP_INTERLOCKED_CAST (LONG volatile *)
#endif

#endif //CPPCSP_WINDOWS

usign32 AtomicCompareAndSwap(__CPPCSP_ALIGNED_USIGN32_PTR address,usign32 compareTo,usign32 swapOnEqual)
{
	return InterlockedCompareExchange(__CPPCSP_INTERLOCKED_CAST (address),swapOnEqual,compareTo);	
}

void* _AtomicCompareAndSwap(__CPPCSP_ALIGNED_VOID_PTR * address,void* compareTo,void* swapOnEqual)
{
	return InterlockedCompareExchangePointer(address,swapOnEqual,compareTo);
}

void* _AtomicSwap(__CPPCSP_ALIGNED_VOID_PTR * address,void* value)
{
	return InterlockedExchangePointer(address,value);
}

usign32 AtomicSwap(__CPPCSP_ALIGNED_USIGN32_PTR address,usign32 value)
{
	return InterlockedExchange(__CPPCSP_INTERLOCKED_CAST (address),value);
}

usign32 AtomicIncrement(__CPPCSP_ALIGNED_USIGN32_PTR address)
{
	return InterlockedIncrement(__CPPCSP_INTERLOCKED_CAST (address));
}

usign32 AtomicDecrement(__CPPCSP_ALIGNED_USIGN32_PTR address)
{
	return InterlockedDecrement(__CPPCSP_INTERLOCKED_CAST (address));
}


} //namespace internal
} //namespace csp


