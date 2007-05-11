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

/**
*	A channel "factory" class
*
*	This is the templated base class for channel factories.  It contains methods to get one-to-one, one-to-any, any-to-one and any-to-any
*	channel ends from the factory.  The channel is guaranteed to last until the factory is destroyed.  
*
*	This guarantee means that a ChannelFactory can end up using up a lot of memory if channels are continually re-allocated,
*	because none of these channels will be destroyed until the factory is destroyed.  Be mindful of this if you allocate
*	channels inside a loop, or otherwise repeatedly reallocate them.
*
*	@see StandardChannelFactory
*	@see BufferedChannelFactory
*
*/
template <typename DATA_TYPE>
class ChannelFactory : public boost::noncopyable
{
protected:
	//Helper function:
	template <typename T>
	static void deleteAll(std::list<T>& list)
	{
		for (typename std::list<T>::iterator it = list.begin();it != list.end();it++)
		{
			delete *it;
		}
	}

public:
	/**
	*	Gets a one-to-one channel from the factory
	*
	*	@param in A pointer to an AltChanin object that will become the input end of the requested channel.
	*	@param out A pointer to a Chanout object that will become the output end of the requested channel.
	*	@param canPoisonIn Flag to designate whether the input end should be poisonable
	*	@param canPoisonOut Flag to designate whether the output end should be poisonable
	*/
	virtual void one2One(AltChanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true) = 0;
	
	/**
	*	Gets a one-to-any channel from the factory
	*
	*	@param in A pointer to a Chanin object that will become the input end of the requested channel.
	*	@param out A pointer to a Chanout object that will become the output end of the requested channel.
	*	@param canPoisonIn Flag to designate whether the input end should be poisonable
	*	@param canPoisonOut Flag to designate whether the output end should be poisonable
	*/	
	virtual void one2Any(Chanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true) = 0;
	
	/**
	*	Gets an any-to-one channel from the factory
	*
	*	@param in A pointer to an AltChanin object that will become the input end of the requested channel.
	*	@param out A pointer to a Chanout object that will become the output end of the requested channel.
	*	@param canPoisonIn Flag to designate whether the input end should be poisonable
	*	@param canPoisonOut Flag to designate whether the output end should be poisonable
	*/	
	virtual void any2One(AltChanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true) = 0;
	
	/**
	*	Gets an any-to-any channel from the factory
	*
	*	@param in A pointer to a Chanin object that will become the input end of the requested channel.
	*	@param out A pointer to a Chanout object that will become the output end of the requested channel.
	*	@param canPoisonIn Flag to designate whether the input end should be poisonable
	*	@param canPoisonOut Flag to designate whether the output end should be poisonable
	*/	
	virtual void any2Any(Chanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true) = 0;
	
	/**
	*	Gets a one-to-one channel from the factory
	*
	*	@param canPoisonIn Flag to designate whether the input end should be poisonable
	*	@param canPoisonOut Flag to designate whether the output end should be poisonable
	*	@return A pair comprising the input and output ends (respectively) of the channel
	*/
	virtual inline std::pair< AltChanin<DATA_TYPE> , Chanout<DATA_TYPE> > one2OnePair(bool canPoisonIn = true,bool canPoisonOut = true)
	{
		std::pair< AltChanin<DATA_TYPE> , Chanout<DATA_TYPE> > pr;
		one2One(&pr.first,&pr.second,canPoisonIn,canPoisonOut);
		return pr;
	};
	
		
	/**
	*	Gets a one-to-any channel from the factory
	*
	*	@param canPoisonIn Flag to designate whether the input end should be poisonable
	*	@param canPoisonOut Flag to designate whether the output end should be poisonable
	*	@return A pair comprising the input and output ends (respectively) of the channel
	*/
	virtual inline std::pair< Chanin<DATA_TYPE> , Chanout<DATA_TYPE> > one2AnyPair(bool canPoisonIn = true,bool canPoisonOut = true)
	{
		std::pair< Chanin<DATA_TYPE> , Chanout<DATA_TYPE> > pr;
		one2Any(&pr.first,&pr.second,canPoisonIn,canPoisonOut);
		return pr;
	};
	
		
	/**
	*	Gets an any-to-one channel from the factory
	*
	*	@param canPoisonIn Flag to designate whether the input end should be poisonable
	*	@param canPoisonOut Flag to designate whether the output end should be poisonable
	*	@return A pair comprising the input and output ends (respectively) of the channel
	*/
	virtual inline std::pair< AltChanin<DATA_TYPE> , Chanout<DATA_TYPE> > any2OnePair(bool canPoisonIn = true,bool canPoisonOut = true)
	{
		std::pair< AltChanin<DATA_TYPE> , Chanout<DATA_TYPE> > pr;
		any2One(&pr.first,&pr.second,canPoisonIn,canPoisonOut);
		return pr;
	};
				
	/**
	*	Gets an any-to-any channel from the factory
	*
	*	@param canPoisonIn Flag to designate whether the input end should be poisonable
	*	@param canPoisonOut Flag to designate whether the output end should be poisonable
	*	@return A pair comprising the input and output ends (respectively) of the channel
	*/
	virtual inline std::pair< Chanin<DATA_TYPE> , Chanout<DATA_TYPE> > any2AnyPair(bool canPoisonIn = true,bool canPoisonOut = true)
	{
		std::pair< Chanin<DATA_TYPE> , Chanout<DATA_TYPE> > pr;
		any2Any(&pr.first,&pr.second,canPoisonIn,canPoisonOut);
		return pr;
	};			

	virtual ~ChannelFactory() {};
};

/**
*	An implementation of ChannelFactory for standard (unbuffered, non-networked) channels
*
*	@see ChannelFactory
*/

template <typename DATA_TYPE>
class StandardChannelFactory : public ChannelFactory<DATA_TYPE>
{
	internal::SpinMutex mutex;
	std::list< One2OneChannel<DATA_TYPE>* > o2o;
	std::list< One2AnyChannel<DATA_TYPE>* > o2a;
	std::list< Any2OneChannel<DATA_TYPE>* > a2o;
	std::list< Any2AnyChannel<DATA_TYPE>* > a2a;
public:
	void one2One(AltChanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true)
	{
		mutex.claim();
			o2o.push_back(new One2OneChannel<DATA_TYPE>());		
			One2OneChannel<DATA_TYPE>& c = *(o2o.back());
			*in = canPoisonIn ? c.reader() : NoPoison(c.reader());
			*out = canPoisonOut ? c.writer() : NoPoison(c.writer());
		mutex.release();
	}
	void one2Any(Chanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true)
	{
		mutex.claim();
			o2a.push_back(new One2AnyChannel<DATA_TYPE>());		
			One2AnyChannel<DATA_TYPE>& c = *(o2a.back());
			*in = canPoisonIn ? c.reader() : NoPoison(c.reader());
			*out = canPoisonOut ? c.writer() : NoPoison(c.writer());
		mutex.release();	
	}
	void any2One(AltChanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true)
	{
		mutex.claim();
			a2o.push_back(new Any2OneChannel<DATA_TYPE>());		
			Any2OneChannel<DATA_TYPE>& c = *(a2o.back());
			*in = canPoisonIn ? c.reader() : NoPoison(c.reader());
			*out = canPoisonOut ? c.writer() : NoPoison(c.writer());
		mutex.release();
	}
	void any2Any(Chanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true)
	{
		mutex.claim();
			a2a.push_back(new Any2AnyChannel<DATA_TYPE>());		
			Any2AnyChannel<DATA_TYPE>& c = *(a2a.back());
			*in = canPoisonIn ? c.reader() : NoPoison(c.reader());
			*out = canPoisonOut ? c.writer() : NoPoison(c.writer());
		mutex.release();	
	}
	
	inline ~StandardChannelFactory()
	{
		mutex.claim();
			deleteAll(o2o);
			deleteAll(o2a);
			deleteAll(a2o);
			deleteAll(a2a);
		mutex.release();
	}
};

/**
*	An implementation of ChannelFactory for buffered channels
*
*	@see ChannelFactory
*/
template <typename DATA_TYPE>
class BufferedChannelFactory : public ChannelFactory<DATA_TYPE>
{
	internal::SpinMutex mutex;
	std::list< BufferedOne2OneChannel<DATA_TYPE>* > o2o;
	std::list< BufferedOne2AnyChannel<DATA_TYPE>* > o2a;
	std::list< BufferedAny2OneChannel<DATA_TYPE>* > a2o;
	std::list< BufferedAny2AnyChannel<DATA_TYPE>* > a2a;
	
	csp::Mobile< csp::ChannelBufferFactory<DATA_TYPE> > bufferFactory;
public:
	inline explicit BufferedChannelFactory(const csp::Mobile< csp::ChannelBufferFactory<DATA_TYPE> >& _bufferFactory)
		:	bufferFactory(_bufferFactory)
	{
	}

	void one2One(AltChanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true)
	{
		mutex.claim();
			o2o.push_back(new BufferedOne2OneChannel<DATA_TYPE>(*bufferFactory));
			BufferedOne2OneChannel<DATA_TYPE>& c = *(o2o.back());
			*in = canPoisonIn ? c.reader() : NoPoison(c.reader());
			*out = canPoisonOut ? c.writer() : NoPoison(c.writer());
		mutex.release();
	}
	void one2Any(Chanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true)
	{
		mutex.claim();
			o2a.push_back(new BufferedOne2AnyChannel<DATA_TYPE>(*bufferFactory));
			BufferedOne2AnyChannel<DATA_TYPE>& c = *(o2a.back());
			*in = canPoisonIn ? c.reader() : NoPoison(c.reader());
			*out = canPoisonOut ? c.writer() : NoPoison(c.writer());
		mutex.release();	
	}
	void any2One(AltChanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true)
	{
		mutex.claim();
			a2o.push_back(new BufferedAny2OneChannel<DATA_TYPE>(*bufferFactory));
			BufferedAny2OneChannel<DATA_TYPE>& c = *(a2o.back());
			*in = canPoisonIn ? c.reader() : NoPoison(c.reader());
			*out = canPoisonOut ? c.writer() : NoPoison(c.writer());
		mutex.release();
	}
	void any2Any(Chanin<DATA_TYPE>* in,Chanout<DATA_TYPE>* out,bool canPoisonIn = true,bool canPoisonOut = true)
	{
		mutex.claim();
			a2a.push_back(new BufferedAny2AnyChannel<DATA_TYPE>(*bufferFactory));
			BufferedAny2AnyChannel<DATA_TYPE>& c = *(a2a.back());
			*in = canPoisonIn ? c.reader() : NoPoison(c.reader());
			*out = canPoisonOut ? c.writer() : NoPoison(c.writer());
		mutex.release();	
	}

	inline ~BufferedChannelFactory()
	{
		mutex.claim();
			deleteAll(o2o);
			deleteAll(o2a);
			deleteAll(a2o);
			deleteAll(a2a);
		mutex.release();
	}
};

} //namespace csp

