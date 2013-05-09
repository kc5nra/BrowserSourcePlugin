#pragma once

#include "OBSApi.h"
#include "KeyboardManager.h"
#include "JavascriptExtension.h"

class KeyboardExtension :
    public JavascriptExtension, public KeyboardListener
{

public:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    const KeyboardManager *keyboardManager;
    List<Keyboard::Key> keyEvents;
    CRITICAL_SECTION keyEventLock;

public:
    KeyboardExtension(const KeyboardManager *keyboardManager);
    ~KeyboardExtension();

public:
    bool Handle(const Awesomium::WebString &functionName, const Awesomium::JSArray &args, JSArray *returnArgs);
    void KeyboardEvent(Keyboard::Key &key);
};


// factory class singleton, owns manager singleton
class KeyboardExtensionFactory : public JavascriptExtensionFactory
{

private:
    static KeyboardManager *keyboardManager;
public:
    static KeyboardManager *GetKeyboardManager();

public:
    KeyboardExtensionFactory() {
        // there should be no manager active, multiple instances?
        assert(keyboardManager == NULL);
        keyboardManager = new KeyboardManager();
    }

    ~KeyboardExtensionFactory() {
        delete keyboardManager;
        keyboardManager = 0;
    }
public:
    JavascriptExtension *Create() {
        return new KeyboardExtension(keyboardManager);
    }
};