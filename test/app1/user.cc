#include "user.h"
#include "itq_gen.h"

void UserThread::recvShutdown( Shutdown *msg )
{
	log_message( "received shutdown" );
	breakLoop = true;
}

int UserThread::main()
{
	recvLoop();
	log_message( "exiting" );
	return 0;
}
