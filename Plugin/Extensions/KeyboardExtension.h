/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "OBSApi.h"
#include "..\JavascriptExtension.h"

#include "KeyboardManager.h"

#include <vector>

class KeyboardExtension :
    public JavascriptExtension, public KeyboardListener
{

public:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    const KeyboardManager *keyboardManager;
    std::vector<Keyboard::Key> keyEvents;
    CRITICAL_SECTION keyEventLock;

public:
    KeyboardExtension(const KeyboardManager *keyboardManager);
    ~KeyboardExtension();

public:
    JSValue Handle(const std::string &functionName, const Awesomium::JSArray &args);
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