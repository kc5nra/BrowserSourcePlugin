 /**
* John Bradley (jrb@turrettech.com)
*/

#include "IrcExtension.h"

#include <time.h>
#include <process.h>

#include <Awesomium\STLHelpers.h>
#include <Awesomium\JSValue.h>

#include <sstream>

#include "..\STLUtilities.h"

using namespace Awesomium;


unsigned __stdcall IrcThread(void* threadArgs)
{
    { // without this extra scope destructors will not be called

        IrcExtension *context = (IrcExtension *)threadArgs;

        irc_session_t *session = context->GetSession();
        std::string &serverName = context->GetServerName();
        unsigned int port = context->GetPort();
        
        irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);

        srand((unsigned int)time(NULL));

        std::stringstream s;
        s << "justinfan" << rand() % 999999;

        std::string nickName = s.str();

        context->SetNickName(nickName);

        if (irc_connect(session, serverName.c_str(), port, 0, nickName.c_str(), 0, 0))
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
    
    // enable special messages
    irc_send_raw(session, "TWITCHCLIENT 2");

    std::string joinChannel = "#" + context->GetChannelName();
    irc_cmd_join(session, joinChannel.c_str(), NULL);

    // ?? twitch sends it
    std::string jtvChannelCommand = "JTVROOMS " + joinChannel;
    irc_send_raw(session, jtvChannelCommand.c_str());
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

    if (context->GetNickName() == originNickName) {
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

    IrcMessage &latestMessage = context->GetLatestMessage();

    latestMessage.Prepare(origin, params[1]);

    auto &moderators = context->GetModerators();

    if (moderators.find(origin) != moderators.end()) {
        latestMessage.groups.insert("moderator");
    }

    context->AddMessage(latestMessage);
}

void event_privmsg(
    irc_session_t *session, 
    const char *eventType, 
    const char *origin, 
    const char **params, 
    unsigned int count)
{
    if (!origin || count != 2 || strcmp("jtv", origin) != 0) {
        return;
    }

    IrcExtension *context = (IrcExtension *)(irc_get_ctx(session));

    std::string command(params[1]);

    auto commandArgs = BSP::Split(params[1], ' ');
    
    IrcMessage &latestMessage = context->GetLatestMessage();
    latestMessage.Prepare(commandArgs[1]);

    if (commandArgs.size() > 1) {
        if (commandArgs[0] == "USERCOLOR") {
            latestMessage.color = commandArgs[2];
        } else if (commandArgs[0] == "SPECIALUSER") {
            latestMessage.groups.insert(commandArgs[2]);
        } else if (commandArgs[0] == "EMOTESET") {
            std::string emoticonString = commandArgs[2];
            BSP::ReplaceStringInPlace(emoticonString, "[", "");
            BSP::ReplaceStringInPlace(emoticonString, "]", "");
            auto emoticons = BSP::Split(emoticonString, ',');
            for(auto i = emoticons.begin(); i != emoticons.end(); i++) {
                latestMessage.emoticons.insert(*i);
            } 
        }
    }
}

void event_mode(
    irc_session_t *session, 
    const char *eventType, 
    const char *origin, 
    const char **params, 
    unsigned int count)
{
    if (!origin || count != 3 || strcmp("jtv", origin) != 0) {
        return;
    }

    IrcExtension *context = (IrcExtension *)(irc_get_ctx(session));
    
    
    std::string joinChannel = "#" + context->GetChannelName();
    std::string mode = params[1];
    std::string nickName = params[2];

    auto &moderators = context->GetModerators();

    if (joinChannel ==  params[0]) {
        if (mode == "+o") {
            moderators.insert(nickName);
        } else if (mode == "-o") {
            moderators.erase(nickName);
        }
    }
}

IrcExtension::IrcExtension()
    : JavascriptExtension("OBSIrcExtension")
{
    noReturnArgumentFunctions.insert("connect");
    noReturnArgumentFunctions.insert("disconnect");
    returnArgumentFunctions.insert("getMessages");
    returnArgumentFunctions.insert("isConnected");

    irc_callbacks_t callbacks = { 0 };

    callbacks.event_connect = event_connect;
    callbacks.event_join = event_join;
    callbacks.event_channel = event_channel; 
    callbacks.event_privmsg = event_privmsg;
    callbacks.event_mode = event_mode;

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


        if (args.size() < 1 || !args[0].IsString()) {
            // TODO: log this and exit
            return JSValue::Undefined();
        }

        channelName = ToString(args[0].ToString());
        serverName = channelName + ".jtvirc.com";
        port = 6667;

        if (args.size() >= 2) {
            if (!(args[1].IsString())) {
                // log error
                return JSValue::Undefined();
            }
            serverName = ToString(args[1].ToString());
        }

        if (args.size() == 3) {
            if (!args[2].IsInteger()) {
                // log error
                return JSValue::Undefined();
            }
            port = (unsigned int)args[2].ToInteger();
        }

        isJoinedChannel = false;
       
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
        moderators.clear();
        latestMessage.Clear();
        
        EnterCriticalSection(&messageLock);
        messages.clear();
        LeaveCriticalSection(&messageLock);

        return JSValue::Undefined();

    } else if (functionName == "getMessages") {  // [message, message, ..] getMessages()
        EnterCriticalSection(&messageLock);

        if (args.size() != 0) {
            // TODO: log invalid method call
            return JSValue::Undefined();
        }

        JSArray returnArgs;

        while(messages.size()) {
            IrcMessage &message = messages[0];
            JSObject newMessageObject;
            
            newMessageObject.SetProperty(WSLit("nickname"), ToWebString(message.username));
            newMessageObject.SetProperty(WSLit("message"), ToWebString(message.message));
            newMessageObject.SetProperty(WSLit("color"), ToWebString(message.color));
            
            JSArray groups;
            for(auto i = message.groups.begin(); i != message.groups.end(); i++) {
                groups.Push(ToWebString(*i));
            }
            newMessageObject.SetProperty(WSLit("groups"), groups);
            
            JSArray emoticons;
            for(auto i = message.emoticons.begin(); i != message.emoticons.end(); i++) {
                groups.Push(ToWebString(*i));
            }
            newMessageObject.SetProperty(WSLit("emoticons"), emoticons);

            returnArgs.Push(newMessageObject);
            messages.erase(messages.begin());
        }

        LeaveCriticalSection(&messageLock);

        return returnArgs;

    } else if (functionName == "isConnected") {
        return JSValue(irc_is_connected(session) && isJoinedChannel);
    }

    return JSValue::Undefined();
}

