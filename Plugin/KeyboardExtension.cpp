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
    returnArgumentFunctions.Push(WSLit("getKeyEvents"));

    InitializeCriticalSection(&keyEventLock);
}

KeyboardExtension::~KeyboardExtension()
{
    DeleteCriticalSection(&keyEventLock);
}

bool 
KeyboardExtension::Handle(
    const WebString &functionName,
    const JSArray &args, 
    JSArray *returnArgs)
{
    // [[type, vkCode]..] getKeyEvents()
    if (functionName == WSLit("getKeyEvents")) {
        assert(args.size() == 0);
        assert(returnArgs);

        EnterCriticalSection(&keyEventLock);
        JSArray jsArray;
        while(keyEvents.Num())
        {
            Keyboard::Key &key = keyEvents[0];
            JSArray args;
            args.Push(JSValue(key.type));
            args.Push(JSValue((int)key.vkCode));
            returnArgs->Push(args);
            keyEvents.Remove(0);
        }
        
        LeaveCriticalSection(&keyEventLock);
        return true;
    }

    return false;
}

void 
KeyboardExtension::KeyboardEvent(Keyboard::Key &key)
{
    EnterCriticalSection(&keyEventLock);
    keyEvents.Add(key);
    LeaveCriticalSection(&keyEventLock);
}
