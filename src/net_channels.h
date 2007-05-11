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

#include <vector>
#include <map>

namespace csp
{
	class TCPSocketChannel : public boost::noncopyable
	{
	private:
		BufferedOne2OneChannel< std::vector<unsigned char> > fromNetwork;
		BufferedOne2OneChannel< std::vector<unsigned char> > toNetwork;
		
		inline explicit TCPSocketChannel(const usign32 bufSize)
			:	fromNetwork(PrimitiveAggregatingFIFOBuffer<unsigned char>::Factory(bufSize)),
				toNetwork(PrimitiveAggregatingFIFOBuffer<unsigned char>::Factory(bufSize))
		{
		}
		
	public:
	
		AltChanin< std::vector<unsigned char> > reader()
		{
			return fromNetwork.reader();
		}
		
		Chanout< std::vector<unsigned char> > writer()
		{
			return toNetwork.writer();
		}
	};	
	
	class TCPSocketAccepterChannel : public boost::noncopyable
	{
		One2OneChannel< Mobile<TCPSocketChannel> > channel;
		
		
	public:
		AltChanin< Mobile<TCPSocketChannel> > reader()
		{
			return channel.reader();
		}
	};

	Mobile<TCPSocketChannel> ConnectTCPSocket(const TCPUDPAddress& address, const usign32 bufSize);
	
	Mobile<TCPSocketAccepterChannel> OpenTCPSocketAccepter(const usign16 port, const NetworkInterface& netInterface, const usign32 bufSize /*of the new channels*/);

	namespace internal
	{
		typedef std::pair< Chanin< std::vector<unsigned char> > , Chanout< std::vector<unsigned char> > > NetChanPair;

		class SocketConnectRequest
		{
		public:
			TCPUDPAddress address;
			usign32 bufSize;
			NetChanPair channels;
		};
		
		class SocketOpenAcceptRequest
		{
		public:
			usign16 port;
			NetworkInterface netInterface;			
		};
		
		typedef std::pair< boost::variant<SocketConnectRequest,SocketOpenAcceptRequest> , Chanout<bool> > SocketRequest;

		class NetworkHandler
		{
		private:	
			std::map< Socket, NetChanPair > tcpSockets;
			std::map< Socket, NetChanPair > udpSockets;
			
			BufferedAny2OneChannel< SocketRequest > requestChan;
			Chanin< SocketRequest > requestIn;
			static Chanout< SocketRequest > requestOut;
			
			Time selectTimeout;
			
			class Writer  : public ThreadCSProcess {};
			class Reader  : public ThreadCSProcess {};
		protected:
			void run();
		public:
		
			friend Mobile<TCPSocketChannel> ConnectTCPSocket(const TCPUDPAddress& address, const usign32 bufSize);
			friend Mobile<TCPSocketAccepterChannel> OpenTCPSocketAccepter(const usign16 port, const NetworkInterface& netInterface, const usign32 bufSize /*of the new channels*/);
		};
	
	} //namespace internal	

} //namespace csp


