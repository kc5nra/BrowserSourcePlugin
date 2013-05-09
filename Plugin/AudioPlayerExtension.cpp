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

    
bool 
AudioPlayerExtension::Handle(
    const WebString &functionName,
    const JSArray &args, 
    JSArray *returnArgs)
{
    return false;
}

