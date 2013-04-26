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

        Key::Key key;

        // Is Control being held down?
        key.control = ((GetKeyState(VK_LCONTROL) & 0x80) != 0) ||
            ((GetKeyState(VK_RCONTROL) & 0x80) != 0);

        // Is Shift being held down?
        key.shift = ((GetKeyState(VK_LSHIFT) & 0x80) != 0) ||
            ((GetKeyState(VK_RSHIFT) & 0x80) != 0);

        // Is Alt being held down?
        key.alt = ((GetKeyState(VK_LMENU) & 0x80) != 0) ||
            ((GetKeyState(VK_RMENU) & 0x80) != 0);

        // Is CapsLock on?
        key.capslock = (GetKeyState(VK_CAPITAL) != 0);

        key.type = Key::UP;

        // Handle KeyDown and KeyUp events
        switch (wParam)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            {
                key.type = Key::DOWN;
                break;
            }
        case WM_KEYUP:
        case WM_SYSKEYUP:
            {
                key.type = Key::UP;
                break;
            }
        }

        key.vkCode = kb->vkCode;

        //KeyboardManager *keyboardManager = BrowserSourcePlugin::instance->GetBrowserManager()->GetKeyboardManager();
        //keyboardManager->PushKeyEvent(key);

    }

    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// scratch pad

//void BrowserSource::OnMethodCall(
//	WebView* caller, 
//	unsigned int remoteObjectId, 
//	const WebString& methodName,                         
//	const JSArray& args) 
//{
//	if(remoteObjectId == hGlobalJSObject &&
//		methodName == WSLit("subscribeToKeyEvents"))
//	{
//		if (args.size() != 1 || !args[0].IsBoolean()) {
//			Log(TEXT("bad call to subscribeToKeyEvents, must have 1 argument"));
//		} else {
//			hasKeyEventSubscriber = args[0].ToBoolean();
//		}
//	}
//}
//
//JSValue BrowserSource::OnMethodCallWithReturnValue(
//	WebView* caller, 
//	unsigned int remoteObjectId, 
//	const WebString& methodName,                         
//	const JSArray& args) 
//{
//	return JSValue::Undefined();
//}
//
//
//void BrowserSource::KeyboardEvent(Key::Key &key)
//{
//	EnterCriticalSection(&keyboardLock);
//	keyboardEvents.Add(key);
//	LeaveCriticalSection(&keyboardLock);
//}

// Future stuff, maybe
/*EnterCriticalSection(&keyboardLock);

if (keyboardEvents.Num() > 0) {


if (hasKeyEventSubscriber) {
JSValue windowValue = webView->ExecuteJavascriptWithResult(WSLit("window"), WSLit(""));

if (windowValue.IsObject()) {
JSObject windowObject = windowValue.ToObject();
if (windowObject.HasMethod(WSLit("KeyEventCallback"))) {

for(UINT i = 0; i < keyboardEvents.Num(); i++) {
Key::Key &key = keyboardEvents[i];
JSArray args;
args.Push(JSValue(key.alt));
args.Push(JSValue(key.shift));
args.Push(JSValue(key.control));
args.Push(JSValue(key.capslock));
args.Push(JSValue(key.type));
args.Push(JSValue((int)key.vkCode));

windowObject.Invoke(WSLit("KeyEventCallback"), args);
if (windowObject.last_error() != kError_None) {
Log(TEXT("failed to invoke KeyEventCallback anonymous method (%d)"), windowObject.last_error());
break;
}
}
} else {
Log(TEXT("web view doesn't have KeyEventCallback function declared in the global scope (or is defined as a primitive)"));
}
} else {
Log(TEXT("web view doesn't have KeyEventCallback function declared in the global scope (or is defined as a primitive)"));
}
}
keyboardEvents.Clear();
}

LeaveCriticalSection(&keyboardLock);*/