/**
* John Bradley (jrb@turrettech.com)
*/

#include "IrcExtension.h"

#include <time.h>
#include <process.h>

#include <Awesomium\STLHelpers.h>
#include <Awesomium\JSValue.h>

#include <sstream>

using namespace Awesomium;

unsigned __stdcall IrcThread(void* threadArgs)
{
	{ // without this extra scope destructors will not be called

		IrcExtension *context = (IrcExtension *)threadArgs;

		irc_session_t *session = context->GetSession();
		std::string &channelName = context->GetChannelName();

		irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);


		std::string serverName;
		serverName += channelName;
		serverName += ".jtvirc.com";

		srand((unsigned int)time(NULL));

		std::stringstream s;
		s << "justinfan" << rand() % 999999;

		std::string nickName = s.str();

		context->SetNickName(nickName);

		if (irc_connect(session, serverName.c_str(), 6667, 0, nickName.c_str(), 0, 0))
		{
			//Log(TEXT("IrcThread() Could not connect: %s"), irc_strerror(irc_errno(session)));
			goto end_thread;
		}

		// and run into forever loop, generating events
		if (irc_run (session))
		{
			//Log(TEXT("IrcThread() Could not connect or I/O error: %s"), irc_strerror(irc_errno(session)));
		}

end_thread:;
	}

	_endthreadex(0);
	return 0; 
}

void event_connect(
	irc_session_t *session, 
	const char *eventType,
	const char *origin, 
	const char **params, 
	unsigned int count)
{
	IrcExtension *context = (IrcExtension *)(irc_get_ctx(session));

	std::string joinChannel = "#" + context->GetChannelName();
	irc_cmd_join(session, joinChannel.c_str(), NULL);
}

void event_join(
	irc_session_t *session, 
	const char *eventType, 
	const char *origin, 
	const char **params, 
	unsigned int count)
{
	if ( !origin || count != 1)
		return;

	IrcExtension *context = (IrcExtension *)(irc_get_ctx(session));

	std::string originNickName = origin;

	if (context->GetNickName().compare(originNickName) == 1) {
		context->SetJoinedChannel(true);
	}   
}

void event_channel(
	irc_session_t *session, 
	const char *eventType, 
	const char *origin, 
	const char **params, 
	unsigned int count)
{
	if ( !origin || count != 2 )
		return;

	IrcExtension *context = (IrcExtension *)(irc_get_ctx(session));

	std::string message;;
	message += origin;
	message += ": ";
	message += params[1];

	context->AddMessage(message);
}

IrcExtension::IrcExtension()
	: JavascriptExtension("OBSIrcExtension")
{
	noReturnArgumentFunctions.insert("connect");
	noReturnArgumentFunctions.insert("disconnect");
	returnArgumentFunctions.insert("getMessages");
	returnArgumentFunctions.insert("isConnected()");

    irc_callbacks_t callbacks = { 0 };
	
    //memset(&callbacks, 0, sizeof(callbacks));

	callbacks.event_connect = event_connect;
	callbacks.event_join = event_join;
	callbacks.event_channel = event_channel;

	session = irc_create_session(&callbacks);

	irc_set_ctx(session, this);

	hThread = NULL;

	InitializeCriticalSection(&messageLock);
}

IrcExtension::~IrcExtension()
{
	if (hThread) {
		irc_disconnect(session);
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		hThread = NULL;
	}

	irc_destroy_session(session);
}
JSValue IrcExtension::Handle(
	const std::string &functionName,
	const JSArray &args)
{
	// connect(channelName)
	if (functionName == "connect") {
		// were in the middle of something
		if (irc_is_connected(session)) {
			return JSValue::Undefined();
		}

		if (hThread) {
			// thread finished?
			if (WaitForSingleObject(hThread, 0) == WAIT_OBJECT_0) {
				CloseHandle(hThread);	
				hThread = NULL;
			} else {
				// thread is still running
				return JSValue::Undefined();
			}
		}


		if (args.size() != 1 || !args[0].IsString()) {
			// TODO: log this and exit
			return JSValue::Undefined();
		}

		isJoinedChannel = false;

		channelName = ToString(args[0].ToString());

		hThread = (HANDLE)_beginthreadex(NULL, 0, &IrcThread, this, 0, NULL);

		return JSValue::Undefined();    


	} else if (functionName == "disconnect") { // disconnect()

		if (args.size() != 0) {
			// TODO: log invalid method call
			return JSValue::Undefined();
		}

		irc_disconnect(session);

		if (hThread) {
			WaitForSingleObject( hThread, INFINITE );
			CloseHandle(hThread);
			hThread = NULL;
		}

		isJoinedChannel = false;

		return JSValue::Undefined();

	} else if (functionName == "getMessages") {  // [message, message, ..] getMessages()
		EnterCriticalSection(&messageLock);

		if (args.size() != 0) {
			// TODO: log invalid method call
			return JSValue::Undefined();
		}

		JSArray returnArgs;

		while(messages.size()) {
			returnArgs.Push(ToWebString(messages[0]));
			messages.erase(messages.begin());
		}

		LeaveCriticalSection(&messageLock);

		return returnArgs;

	} else if (functionName == "isReady") {
		return JSValue(irc_is_connected(session) && isJoinedChannel);
	}

	return JSValue::Undefined();
}

