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
	namespace common
	{	
		/**
		*	This process syncs on a barrier a specified number of times, then finishes.
		*
		*	When the process starts up, it enrolls on the barrier, then syncs a specified number of times
		*	and then resigns from the barrier and finishes.  You will probably want to pass
		*	a pre-enrolled barrier end to the constructor, rather than letting the process be unenrolled
		*	until it starts up in future (in the mean-time, before it enrolls, syncs will succeed without this process).		
		*
		*	To use this process, you will need to #include <cppcsp/common/barrier_bucket.h>
		*/
		class BarrierSyncer : public CSProcess
		{
		private:
			Mobile<BarrierEnd> end;
			int times;
		protected:
			void run()
			{
				ScopedBarrierEnd scopedEnd(end);				
				for (int i = 0;i < times;i++)
				{
					scopedEnd.sync();
				}				
			}
		public:
			/**
			*	Constructs the BarrierSyncer to use the specified barrier-end.
			*
			*	@param _end The barrier-end to use for sync()ing on.
			*	@param _times The number of times to sync() on the barrier.  Defaults to one.
			*/
			inline BarrierSyncer(const Mobile<BarrierEnd>& _end, const int _times = 1)
				:	CSProcess(65536),end(_end),times(_times)
			{
			}
			
			inline virtual ~BarrierSyncer()
			{
				//In case we don't start up (because there aren't enough resources for the thread):
				//This can happen, because BarrierSyncer is used in our stress-testing for starting up threads
				if (end)
					end->resign();
			}
		};
		
		/**
		*	This process flushes a given bucket every time it is sent "true" on its channel, and quits
		*	when it is sent "false".
		*
		*	Whether this process terminates in an orderly fashion (by being sent "false"), or by being poisoned,
		*	it will always flush the bucket one last time.
		*
		*	To use this process, you will need to #include <cppcsp/common/barrier_bucket.h>
		*/
		class BucketFlusher : public CSProcess
		{
		private:
			Bucket* bucket;
			Chanin<bool> in;
		protected:
			void run()
			{
				try
				{
					bool cont = true;
					while (cont)
					{
						in >> cont;
						//Empty even if we're about to stop:
						bucket->flush();
					}
					
				}
				catch (PoisonException&)
				{
					in.poison();
					bucket->flush();
				}
			}
		public:
			/**
			*	Constructs the process
			*
			*	@param _bucket A pointer to the bucket.  This pointer must remain valid for the lifetime of the process
			*	@param _in The input channel
			*/
			inline BucketFlusher(Bucket* _bucket,const Chanin<bool>& _in)
				:	bucket(_bucket),in(_in)
			{
			}
		};
		
		
	} //namespace common
} //namespace csp

