#ifndef __IPAddress_include__
#define __IPAddress_include__

/*
 *      IPAddress:	IP-Addresses
 *      date:   	01/28/99
 *      author: 	jon
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <assert.h>

/*
* This class represents an IP-Address in format needed by send, sendto ..., this
* is realized while IPAddress inherits from sockaddr_in. The clou is the automatic
* initialization from a given servername and port. So the user don't need to think
* about network byte order and such things.
*/

class IPAddress: public sockaddr_in {
public:

	/* Constructor for usage with hostname and port.
	*
	* @param host is the name of the host for example "www.google.de"
	* @param port is the portnumber
	*/
	IPAddress(const char* host, short port)
	{
		resolve(host, port);
	}

	/* Constructor for usage with hostname and port.
	*
	* @param hostid is 32bit IPv4-Address
	* @param port is the portnumber
	*/
	IPAddress(long hostid, short port)
	{
		sin_addr.s_addr = htonl(static_cast<unsigned>(hostid));
		sin_family = AF_INET;
		setPort(port);
	}

	/* Constructor for serverside, there communication is initiated from other side.
	*
	* @param port there the service should receive data
	*/
	explicit IPAddress(short port)
	{
		sin_family = AF_INET;
		sin_addr.s_addr = INADDR_ANY;
		setPort(port);
	}

	/*
	* Constructor for serverside, where Addressdata is filled in while receiving data
	* from other host.
	*/
	IPAddress() { bzero(static_cast<sockaddr_in*>(this), sizeof(sockaddr_in)); }

	void  setPort(short port) { sin_port = htons(port); }
	short getPort() const { return ntohs(sin_port); }

	void  setAddr(unsigned addr) { sin_addr.s_addr = htonl(addr); }
	unsigned getAddr() const { return ntohl(sin_addr.s_addr); }


	/*
	* Resolves a hostname and port to an IPAddress and port.
	*/
	void resolve(const char* host, short port)
	{
		struct hostent *hp;
		assert(host != 0);

		hp = gethostbyname(host);
		assert(hp != 0);

		bcopy(hp->h_addr, &sin_addr, hp->h_length);
		sin_family = AF_INET;
		setPort(port);
	}

	/*
	* Equal operator for IPAddresses for comparing them
	*/
	bool operator==(const IPAddress& other) const
	{
		return ((other.sin_port == sin_port)
			&& (other.sin_addr.s_addr == sin_addr.s_addr));
	}
};

#endif
