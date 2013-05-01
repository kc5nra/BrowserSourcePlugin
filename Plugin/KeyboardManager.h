/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "OBSApi.h"
#include "windows.h"

namespace Key 
{
    enum EventType 
    {
        UP,
        DOWN
    };

    struct Key 
    {
        bool control;
        bool shift;
        bool alt;
        bool capslock;
        EventType type;
        DWORD vkCode;
    };
}

class KeyboardListener
{
public:
    virtual void KeyboardEvent(Key::Key &key) = 0;
};

class KeyboardManager 
{

public:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static HHOOK hHook;

private:
    List<KeyboardListener *> listeners;
    List<DWORD> keyEvents;
    HINSTANCE hinstUser32;
    CRITICAL_SECTION cs;

public:
    KeyboardManager() 
    {
        InitializeCriticalSection(&cs);
        hinstUser32 = LoadLibrary(L"user32.dll");
        hHook = 0;
        hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hinstUser32, 0 );
    }

    ~KeyboardManager()
    {
        UnhookWindowsHookEx(hHook);
        FreeLibrary(hinstUser32);
    }
public:
    void AddListener(KeyboardListener *subscriber)
    {
        EnterCriticalSection(&cs);
        if (!listeners.HasValue(subscriber)) {
            listeners.Add(subscriber);
        }
        LeaveCriticalSection(&cs);
    }
    void RemoveListener(KeyboardListener *subscriber)
    {
        EnterCriticalSection(&cs);
        listeners.RemoveItem(subscriber);
        LeaveCriticalSection(&cs);
    }

    void PushKeyEvent(Key::Key key)
    {
        EnterCriticalSection(&cs);
        for(UINT i = 0; i < listeners.Num(); i++) {
            listeners[i]->KeyboardEvent(key);
        }
        LeaveCriticalSection(&cs);
    }
};
