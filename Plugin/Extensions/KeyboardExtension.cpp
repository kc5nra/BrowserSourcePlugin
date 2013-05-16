/**
* John Bradley (jrb@turrettech.com)
*/
#include "KeyboardExtension.h"

KeyboardManager *
KeyboardExtensionFactory::GetKeyboardManager()
{
    return keyboardManager;
}
KeyboardManager *KeyboardExtensionFactory::keyboardManager = NULL;

KeyboardExtension::KeyboardExtension(const KeyboardManager *keyboardManager)
    : JavascriptExtension(WSLit("OBSKeyboardExtension"))
{
    this->keyboardManager = keyboardManager;
    returnArgumentFunctions.insert(WSLit("getKeyEvents"));

    InitializeCriticalSection(&keyEventLock);
}

KeyboardExtension::~KeyboardExtension()
{
    DeleteCriticalSection(&keyEventLock);
}

JSValue 
KeyboardExtension::Handle(
    const WebString &functionName,
    const JSArray &args)
{
    // {events: [[type, vkCode]..] getKeyEvents()
    if (functionName == WSLit("getKeyEvents")) {
        assert(args.size() == 0);

        EnterCriticalSection(&keyEventLock);
        JSArray returnArgs;
        while(keyEvents.size())
        {
            Keyboard::Key &key = keyEvents[0];
            JSArray args;
            args.Push(JSValue(key.type));
            args.Push(JSValue((int)key.vkCode));
            returnArgs.Push(args);
			keyEvents.erase(keyEvents.begin());
        }

        LeaveCriticalSection(&keyEventLock);
        return returnArgs;
    }

    return JSValue::Undefined();
}

void 
KeyboardExtension::KeyboardEvent(Keyboard::Key &key)
{
    EnterCriticalSection(&keyEventLock);
    keyEvents.push_back(key);
    LeaveCriticalSection(&keyEventLock);
}
