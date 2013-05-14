/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include <windows.h>
#include <process.h>

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
        INTERACTION,
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

            if (info) {
                Free(info);
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
        void *info;
    };
}

class BrowserManager
{

public:
    static unsigned __stdcall 
    BrowserManagerEntry(void* threadArgs) 
    {
        (static_cast<BrowserManager *>(threadArgs))->BrowserManagerEntry();
        _endthreadex(0);
        return 0;
    }

private:
    WebCore *webCore;

    CRITICAL_SECTION cs;

    std::vector<WebView *> webViews;
    std::vector<JavascriptExtensionFactory *> javascriptExtensionFactories;
    std::vector<Browser::Event *> pendingEvents;

    Browser::Event *generalUpdate;

    bool isStarted;
    HANDLE hThread;
    HANDLE updateEvent;

public:
    BrowserManager() 
    {
		InitializeCriticalSection(&cs);

        generalUpdate = new Browser::Event(Browser::UPDATE, NULL);
        updateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        hThread = NULL;
        webCore = NULL;
        isStarted = false;  

        // setup the default extensions
        //javascriptExtensionFactories.Add(new AudioPlayerExtensionFactory());
        //javascriptExtensionFactories.Add(new KeyboardExtensionFactory());
        javascriptExtensionFactories.push_back(new IrcExtensionFactory());
    }

    ~BrowserManager() 
    {
        RunAndWait(Browser::CLEANUP, NULL, 0);

        // this is overkill
        // but make sure the thread has completed
        if (hThread) {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
            hThread = NULL;
        }

        assert(!isStarted);

        for (UINT i = 0; i < javascriptExtensionFactories.size(); i++) {
            delete javascriptExtensionFactories[i];
        }

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
            while(pendingEvents.size()) {
                EnterCriticalSection(&cs);
                Browser::Event *browserEvent = pendingEvents[0];
                LeaveCriticalSection(&cs);

                switch(browserEvent->eventType) {
                case Browser::CREATE_VIEW: 
                    {
                        int insertIndex = -1;

                        if (browserEvent->webView >= 0) {
                            WebView *webView = webViews[browserEvent->webView];
                            webView->Destroy();
                            webViews.erase(webViews.begin() + browserEvent->webView);

                            // reuse the index we just deleted
                            insertIndex = browserEvent->webView;
                        } else {
                            for(UINT i = 0; i < webViews.size(); i++) {
                                if (webViews[i] == NULL) {
                                    insertIndex = i;
                                    webViews.erase(webViews.begin() + i);
                                    break;
                                }
                            }
                        }

                        if (insertIndex >= 0) {
                            webViews.insert(webViews.begin() + insertIndex, browserEvent->source->CreateWebViewCallback(webCore, insertIndex));
                        } else {
                            webViews.push_back(browserEvent->source->CreateWebViewCallback(webCore, webViews.size()));
                        }

                        browserEvent->Complete();
                        delete browserEvent;
                        break;
                    }
                case Browser::SCENE_CHANGE:
                    {
                        webCore->Update();
                        browserEvent->source->SceneChangeCallback(webViews[browserEvent->webView]);
                        browserEvent->Complete();
                        delete browserEvent;
                        break;
                    }
                case Browser::INTERACTION:
                    {
#ifdef INTERACTION_SUPPORT // remove when implemented
                        WebView *webView = webViews[browserEvent->webView];
                        Interaction *interaction = static_cast<Interaction *>(browserEvent->info);
                        browserEvent->source->InteractionCallback(webView, *interaction);
                        delete browserEvent;
#endif INTERACTION_SUPPORT // remove when implemented
                        break;

                    }
                case Browser::UPDATE:
                    {
                        webCore->Update();

                        if (browserEvent->source) {
                            browserEvent->source->UpdateCallback(webViews[browserEvent->webView]);
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
                            WebView *webView = (hWebView >= 0) ? webViews[hWebView] : nullptr;
                            if (webView) {
                                webView->LoadURL(WebURL(WSLit("about:blank")));
                                // fixes a bug in awesomium where if you destroy it while a flash application
                                // is running it may fail when doing in the future.
                                while(webView->IsLoading()) {
                                    webCore->Update();
                                    Sleep(20);
                                }
                                webView->Destroy();
                                webViews.erase(webViews.begin() + hWebView);
                                webViews.insert(webViews.begin() + hWebView, nullptr);
                            }

                            EnterCriticalSection(&cs);
                            for(UINT i = 1; i < pendingEvents.size(); i++) {
                                // remove all events belonging to this web view
                                // this would include only Update, Shutdown and Create View requests
                                Browser::Event *pendingEvent = pendingEvents[i];
                                if (pendingEvent->source == browserEvent->source) {
                                    pendingEvents.erase(pendingEvents.begin() + i);
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
                        while(webViews.size()) {
                            // by the time we get here,  everything should already be shut down
                            WebView *webView = webViews[0];
                            webViews.erase(webViews.begin());
                            if (webView) {
                                webView->LoadURL(WebURL(WSLit("about:blank")));
                                // fixes a bug in awesomium where if you destroy it while a flash application
                                // is running it may fail when doing in the future.
                                while(webView->IsLoading()) {
                                    webCore->Update();
                                    Sleep(20);
                                }
                                webView->Destroy();
                            }
                        }

                        webCore->Shutdown();

                        isStarted = false;

                        browserEvent->Complete();
                        delete browserEvent;

                        return;
                    }
                }

                EnterCriticalSection(&cs);
                pendingEvents.erase(pendingEvents.begin());
                LeaveCriticalSection(&cs);
            }

        }
    }

public:
    void Startup() 
    {
        if (!isStarted) {
            isStarted = true;
            hThread = (HANDLE)_beginthreadex(NULL, 0, BrowserManagerEntry, this, 0, NULL);
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
        pendingEvents.push_back(browserEvent);
        LeaveCriticalSection(&cs);
        SetEvent(updateEvent);
    }

    void RunAndWait(Browser::EventType eventType, BrowserSource *browserSource, int hWebView) 
    {
        if (isStarted) {
            Browser::Event *blockingEvent = new Browser::Event(eventType, browserSource, hWebView, true);
            AddEvent(blockingEvent);
            WaitForSingleObject(blockingEvent->completionEvent, INFINITE);
        }
    }

    void ShutdownAndWait(BrowserSource *browserSource)
    {
        RunAndWait(Browser::SHUTDOWN, browserSource, -1);
    }

public:
    WebCore *GetWebCore() 
    {
        return webCore;
    }

    std::vector<JavascriptExtensionFactory *> &GetJavascriptExtensionFactories()
    {
        return javascriptExtensionFactories;
    }

};