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

#include "test.h"
#include <boost/assign/list_of.hpp>

#include "../src/cppcsp.h"
#include "../src/common/basic.h"

using namespace csp;
using namespace csp::internal;
using namespace csp::common;
using namespace boost::assign;
using namespace boost;
using namespace std;

vector< unsigned char> reverseVector(const vector<unsigned char>& v)
{
	vector<unsigned char> r(v.size());
	reverse_copy(v.begin(),v.end(),r.begin());
	return r;
}

class NetChannelTest : public Test, public virtual internal::TestInfo, public SchedulerRecorder
{
public:
	static std::vector<unsigned char> getPattern(size_t size)
	{
		std::vector<unsigned char> v(size);
		for (unsigned i = 0;i < size;i++)
		{
			v[i] = static_cast<unsigned char>(i);
		}
		return v;
	}
	
	template <typename T>
	static bool vectorsEqual(const vector<T>& a,const vector<T>& b)
	{
		if (a.size() != b.size())
			return false;
		
		for (unsigned i = 0;i < a.size();i++)
		{
			if (a[i] != b[i])
				return false;
		}
		
		return true;
	}
	/*
	static TestResult test0()
	{
		BEGIN_TEST()
	
		vector<unsigned char> dataOut = getPattern(100);
		vector<unsigned char> dataIn;
		
	
		usign16 port = 56000;
		
		Mobile<TCPSocketAccepterChannel> chan;
		
		chan = OpenTCPSocketAccepter(port,IPAddress_Any,8);
		
		while (!chan)
		{
			port++;
			chan = OpenTCPSocketAccepter(port,IPAddress_Any,8);
		}
		
		{
			ScopedForking forking;
			
			forking.fork(new ConnectionHandler< 				
				TCPSocketChannel ,
				FunctionProcess< vector<unsigned char> >,
				vector<unsigned char> (*)(const vector<unsigned char>&)
				> (chan->reader(),&reverseVector)
			);
			
			Mobile<TCPSocketChannel> clientChan = ConnectTCPSocket(TCPUDPAddress(IPAddress_Localhost,port),5);
			
			ASSERTL(clientChan,"Could not connect to TCP accepter channel",__LINE__);
			
			clientChan->writer() << dataOut;
			clientChan->reader() >> dataIn;
			
			ASSERTL(vectorsEqual(dataOut,reverseVector(dataIn)),"Data did not arrive back as transmitted",__LINE__);
		}
		
		END_TEST("TCP network channel test");
	}
	*/
	
	std::list<TestResult (*)()> tests()
	{		
		return std::list<TestResult (*)()>();
		/*
		return list_of<TestResult (*) ()>
			(test0)
		;
		*/
	}
	
	std::list<TestResult (*)()> perfTests()
	{		
		return std::list<TestResult (*)()>();
	}	
};

Test* GetNetChannelTest()
{
	return new NetChannelTest;
}
