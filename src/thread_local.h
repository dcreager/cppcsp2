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
		*	A class for thread-local storage of a pointer to type T.  Acts just like T*
		*/
		template <typename T>
		class ThreadLocalPointer
		{
		private:
			#ifdef WIN32
				//Windows threads:
				DWORD tlsSlot;
				inline void allocate() { tlsSlot = TlsAlloc(); }
				inline void deallocate() { TlsFree(tlsSlot); }
				inline T* _get() const { return static_cast<T*>(TlsGetValue(tlsSlot)); }
				inline void _set(T* p) { TlsSetValue(tlsSlot,p); };
			#else
				//Use pthreads:
				pthread_key_t tlsKey;
				inline void allocate() { pthread_key_create(&tlsKey,NULL); }
				inline void deallocate() { pthread_key_delete(tlsKey); }
				inline T* _get() const { return static_cast<T*>(pthread_getspecific(tlsKey)); }
				inline void _set(T* p) { pthread_setspecific(tlsKey,p); }
			#endif
		public:
			inline ThreadLocalPointer() { allocate(); }
			inline ~ThreadLocalPointer() { deallocate(); }
			inline T* operator -> () { return _get(); }
			inline const T* operator -> () const { return _get(); }
			
			inline T& operator * () { return *_get(); }
			inline const T& operator * () const { return *_get(); }
			
			inline void operator = (T* p) { _set(p); }
			
			inline operator T* () const { return _get(); }
		};

		//Use compiler intrinsics if possible:
		#ifdef CPPCSP_GCC
			#ifndef CPPCSP_MINGW
				#ifdef CPPCSP_LINUX
					#define DeclareThreadLocalPointer(T) __thread T *
				#else
					#define DeclareThreadLocalPointer(T) ThreadLocalPointer< T >
				#endif
			#else
				#define DeclareThreadLocalPointer(T) ThreadLocalPointer< T >
			#endif
		#else
			#ifdef CPPCSP_MSVC
				#define DeclareThreadLocalPointer(T) __declspec(thread) T *
			#else
				#define DeclareThreadLocalPointer(T) ThreadLocalPointer< T >
			#endif			
		#endif		

	
	} //namespace internal
} //namespace csp
