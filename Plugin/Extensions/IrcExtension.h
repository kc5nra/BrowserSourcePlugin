/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "OBSApi.h"
#include "..\JavascriptExtension.h"

#include "libircclient.h"

class IrcExtension :
    public JavascriptExtension
{

private:
    HANDLE hThread;
    irc_session_t *session;
    irc_callbacks_t callbacks;

    String nickName;
    WebString channelName;
    StringList messages;

    bool isJoinedChannel;
    
    CRITICAL_SECTION messageLock;

public:
    IrcExtension();
    ~IrcExtension();

public:
    void AddMessage(String &string) {
        EnterCriticalSection(&messageLock);
        messages.Add(string);
        LeaveCriticalSection(&messageLock);
    }

public:
    irc_session_t *GetSesssion() { return session; }
    WebString &GetChannelName() { return channelName; }
    
    String &GetNickName() { return nickName; }
    void SetNickName(char *nickName) { this->nickName = nickName; }

    bool IsJoinedChannel() { return isJoinedChannel; }
    void SetJoinedChannel(bool isJoinedChannel) { this->isJoinedChannel = isJoinedChannel; }

public:
    JSValue Handle(const Awesomium::WebString &functionName, const Awesomium::JSArray &args);
};

class IrcExtensionFactory : public JavascriptExtensionFactory
{

public:
    JavascriptExtension *Create() {
        return new IrcExtension();
    }
};