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

class KeyboardSubscriber 
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
    List<KeyboardSubscriber *> subscribers;
    List<DWORD> keyEvents;
    HINSTANCE hinstUser32;
    CRITICAL_SECTION cs;
    HANDLE keyEvent;

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
    void Subscribe(KeyboardSubscriber *subscriber)
    {
        EnterCriticalSection(&cs);
        if (!subscribers.HasValue(subscriber)) {
            subscribers.Add(subscriber);
        }
        LeaveCriticalSection(&cs);
    }
    void Unsubscribe(KeyboardSubscriber *subscriber)
    {
        EnterCriticalSection(&cs);
        subscribers.RemoveItem(subscriber);
        LeaveCriticalSection(&cs);
    }

    void PushKeyEvent(Key::Key key)
    {
        EnterCriticalSection(&cs);
        for(UINT i = 0; i < subscribers.Num(); i++) {
            subscribers[i]->KeyboardEvent(key);
        }
        LeaveCriticalSection(&cs);
    }
};
