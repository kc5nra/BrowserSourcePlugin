#pragma once

#include "JavascriptExtension.h"

class AudioPlayerExtension : public JavascriptExtension
{

public:
    AudioPlayerExtension();
public:
    bool Handle(const Awesomium::WebString &functionName, const Awesomium::JSArray &args, JSArray *returnArgs);
};

class AudioPlayerExtensionFactory : public JavascriptExtensionFactory
{
public:
    JavascriptExtension *Create() {
        return new AudioPlayerExtension();
    }
};