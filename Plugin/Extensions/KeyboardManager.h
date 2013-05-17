/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "OBSApi.h"
#include "windows.h"
#include <set>

namespace Keyboard
{
    enum EventType 
    {
        UP,
        DOWN
    };

    struct Key 
    {
        EventType type;
        DWORD vkCode;
    };
}

class KeyboardListener
{
public:
    virtual void KeyboardEvent(Keyboard::Key &key) = 0;
};

class KeyboardManager 
{

public:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK hHook;

private:
    std::set<KeyboardListener *> listeners;
    CRITICAL_SECTION listenerLock;

public:
    KeyboardManager() 
    {
        InitializeCriticalSection(&listenerLock);
        HINSTANCE hinstUser32 = LoadLibrary(L"user32.dll");
        hHook = 0;
        hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hinstUser32, 0 );
        if (hinstUser32) {
            FreeLibrary(hinstUser32);
        }
    }

    ~KeyboardManager()
    {
        if (hHook) {
            UnhookWindowsHookEx(hHook);
        }
    }
public:
    void AddListener(KeyboardListener *listener)
    {
        EnterCriticalSection(&listenerLock);
        listeners.insert(listener);
        LeaveCriticalSection(&listenerLock);
    }
    void RemoveListener(KeyboardListener *listener)
    {
        EnterCriticalSection(&listenerLock);
        listeners.erase(listener);
        LeaveCriticalSection(&listenerLock);
    }

    void PushKeyEvent(Keyboard::Key key)
    {
        EnterCriticalSection(&listenerLock);
        for(auto i = listeners.begin(); i != listeners.end(); i++) {
            (*i)->KeyboardEvent(key);
        }
        LeaveCriticalSection(&listenerLock);
    }
};