/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "..\JavascriptExtension.h"

class AudioPlayerExtension : public JavascriptExtension
{

public:
    AudioPlayerExtension();
public:
    JSValue Handle(const Awesomium::WebString &functionName, const Awesomium::JSArray &args);
};

class AudioPlayerExtensionFactory : public JavascriptExtensionFactory
{
public:
    JavascriptExtension *Create() {
        return new AudioPlayerExtension();
    }
};