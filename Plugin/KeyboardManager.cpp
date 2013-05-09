/**
* John Bradley (jrb@turrettech.com)
*/
#include "KeyboardManager.h"
#include "BrowserSourcePlugin.h"

// global
HHOOK KeyboardManager::hHook;

LRESULT CALLBACK 
KeyboardManager::LowLevelKeyboardProc(
    int nCode, 
    WPARAM wParam, 
    LPARAM lParam)
{
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;

        Keyboard::Key key;

        key.type = Keyboard::UP;

        // Handle KeyDown and KeyUp events
        switch (wParam)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            {
                key.type = Keyboard::DOWN;
                break;
            }
        case WM_KEYUP:
        case WM_SYSKEYUP:
            {
                key.type = Keyboard::UP;
                break;
            }
        }

        key.vkCode = kb->vkCode;

        KeyboardManager *keyboardManager = KeyboardExtensionFactory::GetKeyboardManager();
        keyboardManager->PushKeyEvent(key);

    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);
}