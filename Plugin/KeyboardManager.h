/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "OBSApi.h"
#include "windows.h"

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
    List<KeyboardListener *> listeners;
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
        if (!listeners.HasValue(listener)) {
            listeners.Add(listener);
        }
        LeaveCriticalSection(&listenerLock);
    }
    void RemoveListener(KeyboardListener *listener)
    {
        EnterCriticalSection(&listenerLock);
        listeners.RemoveItem(listener);
        LeaveCriticalSection(&listenerLock);
    }

    void PushKeyEvent(Keyboard::Key key)
    {
        EnterCriticalSection(&listenerLock);
        for(UINT i = 0; i < listeners.Num(); i++) {
            listeners[i]->KeyboardEvent(key);
        }
        LeaveCriticalSection(&listenerLock);
    }
};


/*


    void KeyboardEvent(Key::Key &key)
    {
        EnterCriticalSection(&keyLock);
        keyboardEvents.Add(key);
        LeaveCriticalSection(&keyLock);
    }

    JSArray GetKeyEvents() 
    {
        EnterCriticalSection(&keyLock);
        JSArray jsArray;
        while(keyboardEvents.Num())
        {
            Key::Key &key = keyboardEvents[0];
            JSArray args;
            args.Push(JSValue(key.alt));
            args.Push(JSValue(key.shift));
            args.Push(JSValue(key.control));
            args.Push(JSValue(key.capslock));
            args.Push(JSValue(key.type));
            args.Push(JSValue((int)key.vkCode));
            jsArray.Push(args);
            keyboardEvents.Remove(0);
        }
        LeaveCriticalSection(&keyLock);
        return jsArray;
    }

    void ClearKeyEvents()
    {
        EnterCriticalSection(&keyLock);
        keyboardEvents.Clear();
        LeaveCriticalSection(&keyLock);
    }

    */