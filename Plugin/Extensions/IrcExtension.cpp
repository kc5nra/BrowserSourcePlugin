/**
* John Bradley (jrb@turrettech.com)
*/
#include "IrcExtension.h"

#include <time.h>
#include <process.h>
#include <Awesomium\STLHelpers.h>
#include <Awesomium\JSValue.h>

#include "libircclient.h"


using namespace Awesomium;

char *CreateUTF8(WebString &webString)
{
    unsigned int stringLength = webString.ToUTF8(NULL, 0);
    char *string = (char *)(Allocate(stringLength));
    webString.ToUTF8(string, stringLength);
    string[stringLength] = 0; 
    return string;
}

unsigned __stdcall 
IrcThread(void* threadArgs)
{
    IrcExtension *context = (IrcExtension *)threadArgs;

    irc_session_t *session = context->GetSesssion();
    WebString &channelName = context->GetChannelName();

    irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);
    
    WebString serverName = channelName;
    serverName.Append(WSLit(".jtvirc.com"));
    char *serverNameUtf8 = CreateUTF8(serverName);
    
    char nickName[16] = { 0 };
    srand((unsigned int)time(NULL));
    sprintf((char *)&nickName, "justinfan%d", rand() % 999999);
    context->SetNickName(nickName);
    
    if (irc_connect(session, serverNameUtf8, 6667, 0, nickName, 0, 0))
	{
		Log(TEXT("IrcThread() Could not connect: %s"), irc_strerror(irc_errno(session)));
        goto end_thread;
	}

    // and run into forever loop, generating events
	if (irc_run (session))
	{
		Log(TEXT("IrcThread() Could not connect or I/O error: %s"), irc_strerror(irc_errno(session)));
	}

end_thread:
    Free(serverNameUtf8);
    _endthreadex(0);
    return 0;   
}

void 
event_connect(
    irc_session_t *session, 
    const char *eventType,
    const char *origin, 
    const char **params, 
    unsigned int count)
{
    IrcExtension *context = (IrcExtension *)(irc_get_ctx(session));
    
    WebString channel;
    channel.Append(WSLit("#"));
    channel.Append(context->GetChannelName());

    char *channelNameUtf8 = CreateUTF8(channel);  
    irc_cmd_join(session, channelNameUtf8, NULL);
    Free(channelNameUtf8);
}

void 
event_join(
    irc_session_t *session, 
    const char *eventType, 
    const char *origin, 
    const char **params, 
    unsigned int count)
{
	if ( !origin || count != 1)
		return;

    IrcExtension *context = (IrcExtension *)(irc_get_ctx(session));

    String originNickName = origin;

    if (context->GetNickName().CompareI(originNickName) == 1) {
        context->SetJoinedChannel(true);
    }   
}

void 
event_channel(
    irc_session_t *session, 
    const char *eventType, 
    const char *origin, 
    const char **params, 
    unsigned int count)
{
	if ( !origin || count != 2 )
		return;

    IrcExtension *context = (IrcExtension *)(irc_get_ctx(session));

    String message;
    message += origin;
    message += ": ";
    message += params[1];

    context->AddMessage(message);
}

IrcExtension::IrcExtension()
    : JavascriptExtension(WSLit("OBSIrcExtension"))
{
    noReturnArgumentFunctions.Push(WSLit("connect"));
    noReturnArgumentFunctions.Push(WSLit("disconnect"));
    returnArgumentFunctions.Push(WSLit("getMessages"));
    returnArgumentFunctions.Push(WSLit("isConnected()"));
    
    memset(&callbacks, 0, sizeof(callbacks));

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
    irc_destroy_session(session);

    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = NULL;
    }
}
JSValue 
IrcExtension::Handle(
    const WebString &functionName,
    const JSArray &args)
{
    // connect(channelName)
    if (functionName == WSLit("connect")) {
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

        assert(args.size() == 1);
        assert(args[0].IsString());

        isJoinedChannel = false;

        channelName = args[0].ToString();

        hThread = (HANDLE)_beginthreadex(NULL, 0, &IrcThread, this, 0, NULL);

        return JSValue::Undefined();

    // disconnect()
    } else if (functionName == WSLit("disconnect")) {
        irc_disconnect(session);

        if (hThread) {
            WaitForSingleObject( hThread, INFINITE );
            CloseHandle(hThread);
            hThread = NULL;
        }

        isJoinedChannel = false;

        return JSValue::Undefined();

    // [message, message, ..] getMessages()
    } else if (functionName == WSLit("getMessages")) {
        EnterCriticalSection(&messageLock);
        assert(args.size() == 0);
        
        JSArray returnArgs;

        while(messages.Num()) {
            char *utf8String = messages[0].CreateUTF8String();
            returnArgs.Push(WebString::CreateFromUTF8(utf8String, strlen(utf8String)));
            messages.Remove(0);
        }
        
        LeaveCriticalSection(&messageLock);

        return returnArgs;

    } else if (functionName == WSLit("isReady")) {
        return JSValue(irc_is_connected(session) && isJoinedChannel);
    }

    return JSValue::Undefined();
}

