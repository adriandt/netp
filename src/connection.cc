#include "thread.h"

#include <openssl/ssl.h>
#include <errno.h>

Listener::Listener( Thread *thread )
:
	thread(thread),
	selectFd(0)
{
}

void Listener::startListen( unsigned short port )
{
	int fd = thread->inetListen( port );

	selectFd = new SelectFd( thread, fd, this );

	selectFd->type = SelectFd::ConnListen;
	selectFd->fd = fd;
	selectFd->wantRead = true;

	thread->selectFdList.append( selectFd );
}

Connection *PacketListener::accept( int fd )
{
	bool nb = thread->makeNonBlocking( fd );
	if ( !nb )
		log_ERROR( "pkt-listen, post-accept: non-blocking IO not available" );

	PacketConnection *pc = new PacketConnection( thread, 0 );
	SelectFd *selectFd = new SelectFd( thread, fd, 0 );
	selectFd->local = static_cast<Connection*>(pc);
	pc->selectFd = selectFd;
	pc->tlsConnect = false;
	selectFd->type = SelectFd::Connection;
	selectFd->state = SelectFd::Established;
	selectFd->wantRead = true;
	thread->selectFdList.append( selectFd );
	thread->notifyAccept( pc );

	return pc;
}

int Connection::read( char *data, int len )
{
	if ( tlsConnect ) {
		return thread->tlsRead( selectFd, data, len );
	}
	else {
		int res = ::read( selectFd->fd, data, len );
		if ( res < 0 ) {
			if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				/* Nothing now. */
				return 0;
			}
			else {
				/* closed. */
				return -1;
			}
		}
		else if ( res == 0 ) {
			/* Normal closure. */
			return -1;
		}

		return res;
	}
}

int Connection::write( char *data, int len )
{
	if ( tlsConnect ) {
		log_debug( DBG_CONNECTION, "TLS write of " << len << " bytes" );
		return thread->tlsWrite( selectFd, data, len );
	}
	else {
		log_debug( DBG_CONNECTION, "system write of " <<
				len << " bytest to fd " << selectFd->fd );
		int res = ::write( selectFd->fd, data, len );
		if ( res < 0 ) {
			if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				/* Cannot write anything now. */
				return 0;
			}
			else {
				/* error-based closure. */
				return -1;
			}
		}
		return res;
	}
}

void Connection::initiateTls( const char *host, uint16_t port )
{
	selectFd = new SelectFd( thread, 0, 0 );
	selectFd->local = this;

	tlsConnect = true;
	selectFd->type = SelectFd::Connection;
	selectFd->state = SelectFd::Lookup;
	selectFd->port = port;

	thread->asyncLookupHost( selectFd, host );
}

void Connection::initiatePkt( const char *host, uint16_t port )
{
	selectFd = new SelectFd( thread, -1, 0 );
	selectFd->local = this;

	tlsConnect = false;
	selectFd->type = SelectFd::Connection;
	selectFd->state = SelectFd::Lookup;
	selectFd->port = port;

	thread->asyncLookupHost( selectFd, host );
}

void Connection::close( )
{
	if ( selectFd != 0 ) {
		if ( selectFd->ssl != 0 ) {
			SSL_shutdown( selectFd->ssl );
		    SSL_free( selectFd->ssl );
		}

		if ( selectFd->fd >= 0 )
		    ::close( selectFd->fd );

		selectFd->closed = true;
	}

	selectFd = 0;
	closed = true;
}


