#include <dlfcn.h>
#include <genf/thread.h>
#include "module.h"

/* Not sure if this belongs in genf or netp, but here for now. */

void (*_loadProxyHostNames)( LookupSet *lookupSet ) = 0;
void (*_sniffConfigureContext)( NetpConfigure *npc, Context *ctx ) = 0;
void (*_proxyConfigureContext)( NetpConfigure *npc, Context *ctx ) = 0;
void (*_fetchConfigureContext)( NetpConfigure *npc, Context *ctx ) = 0;
void (*_brokerDispatchPacket)( SelectFd *fd, Recv &recv ) = 0;
void (*_fetchAllocFetchObjs)( FetchList *fetchList, NetpConfigure *npc, Thread *thread, SSL_CTX *sslCtx, PacketConnection *brokerConn ) = 0;

void ModuleList::loadModule( const char *fn )
{
	void *dl = dlopen( fn, RTLD_NOW );
	if ( dl == NULL )
		log_FATAL( dlerror() );

	Module *module = new Module;

	module->loadProxyHostNames = (ModuleLoadProxyHostNames) dlsym( dl, "loadProxyHostNames" );
	if ( module->loadProxyHostNames == NULL )
		log_FATAL( dlerror() );

	module->sniffConfigureContext = (ModuleSniffConfigureContext) dlsym( dl, "sniffConfigureContext" );
	if ( module->sniffConfigureContext == NULL )
		log_FATAL( dlerror() );

	module->proxyConfigureContext = (ModuleProxyConfigureContext) dlsym( dl, "proxyConfigureContext" );
	if ( module->proxyConfigureContext == NULL )
		log_FATAL( dlerror() );

	module->fetchConfigureContext = (ModuleFetchConfigureContext) dlsym( dl, "fetchConfigureContext" );
	if ( module->fetchConfigureContext == NULL )
		log_FATAL( dlerror() );

	module->brokerDispatchPacket = (ModuleBrokerDispatchPacket) dlsym( dl, "brokerDispatchPacket" );
	if ( module->brokerDispatchPacket == NULL )
		log_FATAL( dlerror() );

	module->fetchAllocFetchObjs = (ModuleFetchAllocFetchObjs) dlsym( dl, "fetchAllocFetchObjs" );
	if ( module->fetchAllocFetchObjs == NULL )
		log_FATAL( dlerror() );

	moduleList.append( module );
}

void ModuleList::loadProxyHostNames( LookupSet *lookupSet )
{
	for ( Module *module = moduleList.head; module != 0; module = module->next )
		module->loadProxyHostNames( lookupSet );
}

void ModuleList::brokerDispatchPacket( SelectFd *fd, Recv &recv )
{
	for ( Module *module = moduleList.head; module != 0; module = module->next )
		module->brokerDispatchPacket( fd, recv );
}

void ModuleList::sniffConfigureContext( NetpConfigure *npc, Context *ctx )
{
	for ( Module *module = moduleList.head; module != 0; module = module->next )
		module->sniffConfigureContext( npc, ctx );
}

void ModuleList::proxyConfigureContext( NetpConfigure *npc, Context *ctx )
{
	for ( Module *module = moduleList.head; module != 0; module = module->next )
		module->proxyConfigureContext( npc, ctx );
}

void ModuleList::fetchConfigureContext( NetpConfigure *npc, Context *ctx )
{
	for ( Module *module = moduleList.head; module != 0; module = module->next )
		module->fetchConfigureContext( npc, ctx );
}

void ModuleList::fetchAllocFetchObjs( FetchList *fetchList, NetpConfigure *npc,
		Thread *thread, SSL_CTX *sslCtx, PacketConnection *brokerConn )
{
	for ( Module *module = moduleList.head; module != 0; module = module->next )
		module->fetchAllocFetchObjs( fetchList, npc, thread, sslCtx, brokerConn );
}

ModuleList moduleList;
