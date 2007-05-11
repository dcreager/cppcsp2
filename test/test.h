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

#include "../src/cppcsp.h"
#include <list>
#include <set>
#include <string>
#include <boost/assign/list_of.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

using namespace boost::tuples;


class TestResult
{
public:
	std::string name;
	bool result;
	std::string message;
	
	inline bool passed() const {return result;};
	inline bool failed() const {return !result;};
	
	inline std::string getName() const {return name;};
	
	inline std::string getMessage() const {return message;};
	
	inline TestResult(const std::string& _name,const bool _result,const std::string&  _message) : name(_name),result(_result),message(_message) {};
	
	inline TestResult(const std::string& _name,const TestResult _tr) : name(_name),result(_tr.result),message(_tr.message) {};
};

inline TestResult TestResultPass(const std::string&  _name)
{
	return TestResult(_name,true,"");
}

inline TestResult TestResultFail(const char* _name,const char* message)
{
	return TestResult(_name,false,message);
}

inline TestResult TestResultFail(const std::string& _name,const std::string& message)
{
	return TestResult(_name.c_str(),false,message.c_str());
}

/*
template <typename T,typename U,typename V,typename Char>
inline std::basic_stringstream<Char>& operator<< (std::basic_stringstream<Char>& str,const boost::tuple<T,U,V>& pr)
{
	str << "(" << pr.get<0>() << ", " << pr.get<1>() << ", " << pr.get<2>() << ")";
	return str;
}

template <typename T,typename Char,typename Traits>
inline std::basic_ostream<Char,Traits>& operator<< (std::basic_ostream<Char,Traits>& str,const std::list<T>& lst)
{
	str << "[";
	for (typename std::list<T>::const_iterator it = lst.begin();it != lst.end();it++)
	{
		str << *it << ", ";
	}
	str << "]";
	return str;
}

template <typename T,typename Char>
inline std::basic_stringstream<Char>& operator<< (std::basic_stringstream<Char>& str,const std::set<T>& lst)
{
	str << "{";
	for (typename std::set<T>::const_iterator it = lst.begin();it != lst.end();it++)
	{
		str << *it << ", ";
	}
	str << "}";
	return str;
}
*/

//For some reason, the inclusion of boost/variant broke the above code completely
//Therefore I have resorted to the following nasty hack:

namespace boost
{
template <>
inline std::string lexical_cast<std::string,std::list< boost::tuple< csp::internal::ProcessPtr,csp::internal::ProcessPtr,csp::internal::ProcessPtr > > >(const std::list< boost::tuple< csp::internal::ProcessPtr,csp::internal::ProcessPtr,csp::internal::ProcessPtr > >& lst)
{
	std::string str;
	str += "[";
	for (std::list< boost::tuple< csp::internal::ProcessPtr,csp::internal::ProcessPtr,csp::internal::ProcessPtr > >::const_iterator it = lst.begin();it != lst.end();it++)
	{
		str += "(" + boost::lexical_cast<std::string>(it->get<0>()) + ", " + boost::lexical_cast<std::string>(it->get<1>()) + boost::lexical_cast<std::string>(", ") + boost::lexical_cast<std::string>(it->get<2>()) + ")";
		str += ", ";
	}
	str += "]";
	return str;
}

template <>
inline std::string lexical_cast<std::string,std::set< std::list< boost::tuple< csp::internal::ProcessPtr,csp::internal::ProcessPtr,csp::internal::ProcessPtr > > > >(const std::set< std::list< boost::tuple< csp::internal::ProcessPtr,csp::internal::ProcessPtr,csp::internal::ProcessPtr > > >& st)
{
	std::string str;
	str += "{";
	for (std::set< std::list< boost::tuple< csp::internal::ProcessPtr,csp::internal::ProcessPtr,csp::internal::ProcessPtr > > >::const_iterator it = st.begin();it != st.end();it++)
	{
		str += boost::lexical_cast<std::string>(*it) + ", ";
	}
	str += "}";
	return str;
}

template <>
inline std::string
lexical_cast<std::string,std::set< csp::internal::ProcessPtr> >(const std::set< csp::internal::ProcessPtr>& st)
{
	std::string str;
	str += "{";
	for (std::set< csp::internal::ProcessPtr >::const_iterator it = st.begin();it != st.end();it++)
	{
		str += boost::lexical_cast<std::string>(*it) + ", ";
	}
	str += "}";
	return str;
}

template <>
inline std::string
lexical_cast<std::string,std::set< std::pair< csp::internal::ProcessPtr , csp::internal::ProcessPtr > > >(const std::set< std::pair< csp::internal::ProcessPtr , csp::internal::ProcessPtr > >& st)
{
	std::string str;
	str += "{";
	for (std::set< std::pair< csp::internal::ProcessPtr , csp::internal::ProcessPtr > >::const_iterator it = st.begin();it != st.end();it++)
	{
		str += "(" + boost::lexical_cast<std::string>(it->first) + ", " + boost::lexical_cast<std::string>(it->second) + "), ";
	}
	str += "}";
	return str;
}

template <>
inline std::string
lexical_cast<std::string,std::list< csp::internal::Process const *> >(const std::list< csp::internal::Process const *>& st)
{
	std::string str;
	str += "[";
	for (std::list< csp::internal::Process const * >::const_iterator it = st.begin();it != st.end();it++)
	{
		str += boost::lexical_cast<std::string>(*it) + ", ";
	}
	str += "]";
	return str;
}

template <>
inline std::string
lexical_cast<std::string,std::map< csp::ThreadId, std::pair<csp::internal::ProcessPtr,csp::internal::ProcessPtr> > >(const std::map< csp::ThreadId, std::pair<csp::internal::ProcessPtr,csp::internal::ProcessPtr> >& m)
{
	std::string str;
	str += "[";
	for (std::map< csp::ThreadId, std::pair<csp::internal::ProcessPtr,csp::internal::ProcessPtr> >::const_iterator it = m.begin();it != m.end();it++)
	{
		str += "{ " + boost::lexical_cast<std::string>(it->first) + " : "
			+ boost::lexical_cast<std::string>(it->second.first) + " , "
			+ boost::lexical_cast<std::string>(it->second.second) + " }"
		;
	}
	str += "]";
	return str;
}

}

class TestResultFailureException 
{
public:
	std::string reason;
	
	inline TestResultFailureException(const std::string& _reason)
		:	reason(_reason)
	{
	}
};

#define BEGIN_TEST() try {

#define END_TEST_C(testname,catchcode) } catch (const TestResultFailureException& e) { catchcode ; return TestResultFail((testname),e.reason); } \
							catch (DeadlockError& e) { catchcode ; return TestResultFail((testname),std::string("DEADLOCK: ") + boost::lexical_cast<std::string>(e.recentBlocks)); } \
							catch (const std::exception& e) { catchcode ; return TestResultFail((testname),std::string("EXCEPTION THROWN: ") + e.what()); } \
							catch (...) { catchcode ; return TestResultFail((testname),std::string("UNKNOWN EXCEPTION THROWN")); } \
							return TestResultPass((testname));
							
#define END_TEST(testname) END_TEST_C(testname,;)

//#define RESULT(funcname) TestResult(#funcname,funcname())

#define ASSERTL(cond,reason,line) if (false == (cond)) { throw TestResultFailureException(std::string(__FILE__ ":" BOOST_PP_STRINGIZE(line) ": ") + reason); }

#define ASSERTEQ(exp,act,reason,line) if (false == (exp == act)) { throw TestResultFailureException(std::string(__FILE__ ":" + boost::lexical_cast<string>(line) + ": ") + reason \
	+ std::string("; expected:\n\t") + boost::lexical_cast<std::string>(exp) + "\nbut actually:\n\t" + boost::lexical_cast<std::string>(act)); }

#define ASSERTEQ1OF2(exp0,exp1,act,reason,line) if (false == ((exp0 == act) || (exp1 == act))) { throw TestResultFailureException(std::string(__FILE__ ":" BOOST_PP_STRINGIZE(line) ": ") + reason \
	+ std::string("; expected:\n\t") + boost::lexical_cast<std::string>(exp0) \
	+ std::string(" or:\n\t") + boost::lexical_cast<std::string>(exp1) + "\nbut actually:\n\t" + boost::lexical_cast<std::string>(act)); }

#define ASSERT(cond,reason) ASSERTL(cond,reason,__LINE__)

class Test
{
public:
	inline virtual ~Test() {};
	
	virtual std::list<TestResult (*)()> tests() = 0;
	virtual std::list<TestResult (*)()> perfTests() = 0;
};

class SchedulerRecorder : public virtual csp::internal::TestInfo
{
public:
	typedef std::list< boost::tuple<csp::internal::ProcessPtr,csp::internal::ProcessPtr,csp::internal::ProcessPtr> > EventList;
	typedef std::map<csp::ThreadId,EventList> ThreadedEventList;
	static ThreadedEventList events;
	static csp::internal::PureSpinMutex mutex;

	static bool _Schedule(KernelData*)
	{
		while (false == mutex.tryClaim()) {}
		events[csp::CurrentThreadId()].push_back(boost::make_tuple<csp::internal::ProcessPtr,csp::internal::ProcessPtr,csp::internal::ProcessPtr>(currentProcess(),NullProcessPtr,NullProcessPtr));
		mutex.release();
		return true;
	}
	
	static bool _AddProcess(KernelData*,csp::internal::ProcessPtr head,csp::internal::ProcessPtr tail)
	{
		while (false == mutex.tryClaim()) {}
		events[csp::CurrentThreadId()].push_back(boost::make_tuple(currentProcess(),head,tail));
		mutex.release();
		return true;
	}
	
	static void setUp()
	{
		addScheduleFunction(&_Schedule);
		addAddProcessFunction(&_AddProcess);
		events.clear();		
	}
	
	static void tearDown()
	{
		removeScheduleFunction(&_Schedule);
		removeAddProcessFunction(&_AddProcess);
	}

	class RecordEvents
	{
	private:
		EventList* record;
		ThreadedEventList* trecord;
		std::set<EventList>* srecord;
	public:
		inline RecordEvents(EventList* _record)
			:	record(_record),trecord(NULL),srecord(NULL)
		{
			events.clear();
		}
		
		inline RecordEvents(ThreadedEventList* _trecord)
			:	record(NULL),trecord(_trecord),srecord(NULL)
		{
			events.clear();
		}
		
		inline RecordEvents(std::set<EventList>* _srecord)
			:	record(NULL),trecord(NULL),srecord(_srecord)
		{
			events.clear();
		}		
		
		inline ~RecordEvents()
		{
			if (record != NULL)
			{
				*record = events[csp::CurrentThreadId()];
			}
			else if (trecord != NULL)
			{
				*trecord = events;
			}
			else if (srecord != NULL)
			{
				srecord->clear();
				for (ThreadedEventList::const_iterator it = events.begin();it != events.end();it++)
				{
					srecord->insert(it->second);
				}
			}
			events.clear();
		}
	};
	
	class SetUp
	{
	public:
		inline SetUp()
		{
			setUp();
		}
		
		inline ~SetUp()
		{
			tearDown();
		}
	};
};

	/**
	*	A helper class for getting the names of channels
	*/
	template <typename CHANNEL>
	class ChannelName {};
	
	template <typename T> class ChannelName<csp::One2OneChannel<T> > {public:static std::string Name() {return "One2OneChannel";}};
	template <typename T> class ChannelName<csp::Any2AnyChannel<T> > {public:static std::string Name() {return "Any2AnyChannel";}};
	template <typename T> class ChannelName<csp::One2AnyChannel<T> > {public:static std::string Name() {return "One2AnyChannel";}};
	template <typename T> class ChannelName<csp::Any2OneChannel<T> > {public:static std::string Name() {return "Any2OneChannel";}};
	template <typename T> class ChannelName<csp::BufferedOne2OneChannel<T> > {public:static std::string Name() {return "BufferedOne2OneChannel";}};
	template <typename T> class ChannelName<csp::BufferedAny2AnyChannel<T> > {public:static std::string Name() {return "BufferedAny2AnyChannel";}};
	template <typename T> class ChannelName<csp::BufferedOne2AnyChannel<T> > {public:static std::string Name() {return "BufferedOne2AnyChannel";}};
	template <typename T> class ChannelName<csp::BufferedAny2OneChannel<T> > {public:static std::string Name() {return "BufferedAny2OneChannel";}};

	#define ACCESS(BASETYPE,VAR,FIELD) (static_cast<BASETYPE*>(&(VAR)))->FIELD
	
	#define __EXP_TEST_FOR_EACH(R,TESTNAME,CHANNEL) ( TESTNAME < CHANNEL > ) 
	#define TEST_FOR_EACH(TESTNAME,CHANNELS) BOOST_PP_SEQ_FOR_EACH(__EXP_TEST_FOR_EACH,TESTNAME,CHANNELS)


Test* GetBarrierTest();
Test* GetRunTest();
Test* GetChannelTest();
Test* GetMutexTest();
Test* GetAltTest();
Test* GetTimeTest();
Test* GetBufferedChannelTest();
Test* GetAltChannelTest();
Test* GetNetChannelTest();

inline std::list<Test*> GetAllTests()
{
	return boost::assign::list_of (GetBarrierTest()) (GetRunTest()) (GetChannelTest()) (GetMutexTest()) (GetAltTest()) (GetTimeTest()) (GetBufferedChannelTest()) (GetAltChannelTest()) (GetNetChannelTest());
}

