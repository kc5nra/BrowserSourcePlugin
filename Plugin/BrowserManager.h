/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include <windows.h>

#include "OBSApi.h"
#include "BrowserSource.h"

#include "JavascriptExtension.h"
#include "Extensions\AudioPlayerExtension.h"
#include "Extensions\KeyboardExtension.h"
#include "Extensions\IrcExtension.h"

#include <Awesomium\WebCore.h>
#include <Awesomium\WebView.h>
#include <Awesomium\WebURL.h>
#include <Awesomium\STLHelpers.h>
#include <Awesomium\BitmapSurface.h>

using namespace Awesomium;

namespace Browser 
{
    enum EventType 
    {
        UPDATE,
        CREATE_VIEW,
        SCENE_CHANGE,
        SHUTDOWN,
        CLEANUP
    };

    struct Event 
    {
        Event(EventType eventType, BrowserSource *source, int webView = -1, bool isNotifyingOnCompletion = false) 
        {
            this->eventType = eventType;
            this->source = source;
            this->webView = webView;
            if (isNotifyingOnCompletion) {
                this->completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            } else {
                this->completionEvent = NULL;
            }
        }

        ~Event() 
        {
            if (completionEvent) {
                CloseHandle(this->completionEvent);
            }
        }

        void Complete() 
        {
            if (completionEvent) {
                SetEvent(completionEvent);
            }
        }

        // I don't like the spelling Canceled
        void Cancelled() 
        {
            if (completionEvent) {
                SetEvent(completionEvent);
            }
        }

        enum EventType eventType;
        BrowserSource *source;
        int webView;
        HANDLE completionEvent;
    };
}

class BrowserManager
{

public:

    static DWORD WINAPI BrowserManagerEntry(LPVOID self) 
    {
        (static_cast<BrowserManager *>(self))->BrowserManagerEntry();
        return 0;
    }




private:
    WebCore *webCore;

    CRITICAL_SECTION cs;

    List<WebView *> webViews;
    List<JavascriptExtensionFactory *> javascriptExtensionFactories;
    List<Browser::Event *> pendingEvents;

    Browser::Event *generalUpdate;

    bool isStarted;
    HANDLE threadHandle;
    HANDLE updateEvent;

public:
    BrowserManager() 
    {
        InitializeCriticalSection(&cs);

        generalUpdate = new Browser::Event(Browser::UPDATE, NULL);
        updateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        threadHandle = NULL;
        webCore = NULL;
        isStarted = false;  

        // setup the default extensions
        //javascriptExtensionFactories.Add(new AudioPlayerExtensionFactory());
        //javascriptExtensionFactories.Add(new KeyboardExtensionFactory());
        javascriptExtensionFactories.Add(new IrcExtensionFactory());
    }

    ~BrowserManager() 
    {
        AddEvent(new Browser::Event(Browser::CLEANUP, NULL));
        
        while(isStarted) {
            Sleep(10);
        }

        for (UINT i = 0; i < javascriptExtensionFactories.Num(); i++) {
            delete javascriptExtensionFactories[i];
        }

        threadHandle = NULL;
        CloseHandle(updateEvent);
        delete generalUpdate;
    }

protected:
    // this method should never be called directly
    void BrowserManagerEntry() 
    {
        WebConfig webConfig = WebConfig();
        webConfig.remote_debugging_port = 1337;
        webCore = WebCore::Initialize(webConfig);

        for(;;) {

            WaitForSingleObject(updateEvent, INFINITE);
            while(pendingEvents.Num()) {
                EnterCriticalSection(&cs);
                Browser::Event *browserEvent = pendingEvents.GetElement(0);
                LeaveCriticalSection(&cs);

                switch(browserEvent->eventType) {
                case Browser::CREATE_VIEW: 
                    {
                        if (browserEvent->webView >= 0) {
                            WebView *webView = webViews.GetElement(browserEvent->webView);
                            webView->Destroy();
                            webViews.Remove(browserEvent->webView);
                            webViews.Insert(browserEvent->webView, NULL);
                        }

                        int insertIndex = -1;
                        for(UINT i = 0; i < webViews.Num(); i++) {
                            if (webViews.GetElement(i) == NULL) {
                                insertIndex = i;
                                webViews.Remove(i);
                                break;
                            }
                        }
                        if (insertIndex >= 0) {
                            webViews.Insert(insertIndex, browserEvent->source->CreateWebViewCallback(webCore, insertIndex));
                        } else {
                            webViews.Add(browserEvent->source->CreateWebViewCallback(webCore, webViews.Num()));
                        }

                        browserEvent->Complete();
                        delete browserEvent;
                        break;
                    }
                case Browser::SCENE_CHANGE:
                    {
                        webCore->Update();
                        browserEvent->source->SceneChangeCallback(webViews.GetElement(browserEvent->webView));
                        browserEvent->Complete();
                        delete browserEvent;
                        break;
                    }
                case Browser::UPDATE:
                    {
                        webCore->Update();

                        if (browserEvent->source) {
                            browserEvent->source->UpdateCallback(webViews.GetElement(browserEvent->webView));
                            browserEvent->Complete();
                            delete browserEvent;
                            break;
                        }

                        browserEvent->Complete();
                        break;
                    }
                case Browser::SHUTDOWN: 
                    {
                        if (browserEvent->source->GetWebView() >= 0) {

                            int hWebView = browserEvent->source->GetWebView();
                            WebView *webView = (hWebView >= 0) ? webViews[hWebView] : NULL;
                            if (webView) {
                                webView->LoadURL(WebURL(WSLit("about:blank")));
                                // fixes a bug in awesomium where if you destroy it while a flash application
                                // is running it may fail when doing in the future.
                                while(webView->IsLoading()) {
                                    webCore->Update();
                                    Sleep(20);
                                }
                                webView->Destroy();
                                webViews.Remove(hWebView);
                                webViews.Insert(hWebView, NULL);
                            }

                            EnterCriticalSection(&cs);
                            for(UINT i = 1; i < pendingEvents.Num(); i++) {
                                // remove all events belonging to this web view
                                // this would include only Update, Shutdown and Create View requests
                                Browser::Event *pendingEvent = pendingEvents.GetElement(i);
                                if (pendingEvent->source == browserEvent->source) {
                                    pendingEvents.Remove(i);
                                    pendingEvent->Cancelled();
                                    delete pendingEvent;
                                    i--;
                                }
                            }
                            LeaveCriticalSection(&cs);
                        }

                        browserEvent->Complete();
                        delete browserEvent;
                        break;
                    }

                case Browser::CLEANUP:
                    {
                        while(webViews.Num()) {
                            WebView *webView = webViews.GetElement(0);
                            webViews.Remove(0);
                            if (webView) {
                                webView->Destroy();
                            }
                        }

                        webCore->Shutdown();

                        browserEvent->Complete();
                        delete browserEvent;
                        isStarted = false;
                        return;
                    }
                }
                EnterCriticalSection(&cs);
                pendingEvents.Remove(0);
                LeaveCriticalSection(&cs);
            }

        }
    }




public:
    void Startup() 
    {
        if (!isStarted) {
            isStarted = true;
            threadHandle = ::CreateThread(0, 0, BrowserManagerEntry, this, 0, 0);
        }	
    }

    void Update() 
    {
        if (isStarted) {
            AddEvent(generalUpdate);
        }
    }

    void AddEvent(Browser::Event *browserEvent)
    {
        EnterCriticalSection(&cs);
        pendingEvents.Add(browserEvent);
        LeaveCriticalSection(&cs);
        SetEvent(updateEvent);
    }

    void RunAndWait(Browser::EventType eventType, BrowserSource *browserSource, int hWebView) 
    {
        Browser::Event *blockingEvent = new Browser::Event(eventType, browserSource, hWebView, true);
        AddEvent(blockingEvent);
        WaitForSingleObject(blockingEvent->completionEvent, INFINITE);
    }

    void ShutdownAndWait(BrowserSource *browserSource)
    {
        RunAndWait(Browser::SHUTDOWN, browserSource, -1);
        Browser::Event *shutdownEvent = new Browser::Event(Browser::SHUTDOWN, browserSource, -1, true);
        AddEvent(shutdownEvent);
        WaitForSingleObject(shutdownEvent->completionEvent, INFINITE);
    }

public:
    WebCore *GetWebCore() 
    {
        return webCore;
    }

    List<JavascriptExtensionFactory *> &GetJavascriptExtensionFactories()
    {
        return javascriptExtensionFactories;
    }

};