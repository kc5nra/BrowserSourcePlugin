/**
* John Bradley (jrb@turrettech.com)
*/
#include "AudioPlayerExtension.h"

#include <Awesomium\STLHelpers.h>

using namespace Awesomium;

enum AudioPlayerType {
    WINAMP,
    FUBAR,
    ITUNES,
    SPOTIFY
};

AudioPlayerExtension::AudioPlayerExtension()
    : JavascriptExtension("OBSAudioPlayerExtension")
{
}

    
JSValue AudioPlayerExtension::Handle(
    const std::string &functionName,
    const JSArray &args)
{
    return JSValue::Undefined();
}

