/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "..\JavascriptExtension.h"

#include <string>

class AudioPlayerExtension : public JavascriptExtension
{

public:
    AudioPlayerExtension();
public:
    JSValue Handle(const std::string &functionName, const Awesomium::JSArray &args);
};

class AudioPlayerExtensionFactory : public JavascriptExtensionFactory
{
public:
    JavascriptExtension *Create() {
        return new AudioPlayerExtension();
    }
};