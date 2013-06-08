/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include <string>
#include <vector>

extern "C" {
#include "libircclient.h"
}

#include "..\JavascriptExtension.h"

struct IrcMessage {
    std::string username;
    std::string message;
    std::string color;
    std::set<std::string> groups;
    std::set<std::string> emoticons;

    void Prepare(std::string username, std::string message = "")
    {
        if (this->username != username) {
            Clear();
            this->username = username;
        }
        if (message.length()) {
            this->message = message;
        }
    }

    void Clear()
    {
        username.clear();
        message.clear();
        color.clear();
        groups.clear();
        emoticons.clear();
    }
};

class IrcExtension :
	public JavascriptExtension
{

private:
	HANDLE hThread;
	irc_session_t *session;

	std::string nickName;
    std::string serverName;
    unsigned int port;
	std::string channelName;

    std::set<std::string> moderators;
	
    std::vector<IrcMessage> messages;
    IrcMessage latestMessage;

	bool isJoinedChannel;

	CRITICAL_SECTION messageLock;

public:
	IrcExtension();
	virtual ~IrcExtension();

public:
	void AddMessage(IrcMessage &message) {
		EnterCriticalSection(&messageLock);
		messages.push_back(message);
		LeaveCriticalSection(&messageLock);
	}

public:
	irc_session_t *GetSession() { return session; }

    std::string &GetServerName() { return serverName; }
    unsigned int GetPort() { return port; }
	std::string &GetChannelName() { return channelName; }

    IrcMessage &GetLatestMessage() { return latestMessage; }

    std::set<std::string> &GetModerators() { return moderators; }

	std::string &GetNickName() { return nickName; }
	void SetNickName(std::string &nickName) { this->nickName = nickName; }

	bool IsJoinedChannel() { return isJoinedChannel; }
	void SetJoinedChannel(bool isJoinedChannel) { this->isJoinedChannel = isJoinedChannel; }

public:
	JSValue Handle(const std::string &functionName, const Awesomium::JSArray &args);
};

class IrcExtensionFactory : public JavascriptExtensionFactory
{

public:
	JavascriptExtension *Create() {
		return new IrcExtension();
	}
};