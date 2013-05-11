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
    : JavascriptExtension(WSLit("OBSAudioPlayerExtension"))
{
}

    
JSValue 
AudioPlayerExtension::Handle(
    const WebString &functionName,
    const JSArray &args)
{
    return JSValue::Undefined();
}

