/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include <windows.h>
#include <process.h>

#include "OBSApi.h"
#include "BrowserSource.h"
#include "LocalFileHandler.h"

//#include "JavascriptExtension.h"
//#include "Extensions\AudioPlayerExtension.h"
//#include "Extensions\KeyboardExtension.h"
//#include "Extensions\IrcExtension.h"

//#include <Awesomium\WebCore.h>
//#include <Awesomium\WebView.h>
//#include <Awesomium\WebURL.h>
//#include <Awesomium\STLHelpers.h>
//#include <Awesomium\BitmapSurface.h>

#include <Coherent/UI/UISystem.h>
#include <Coherent/UI/View.h>
#include <Coherent/Libraries/Logging/Declarations.h>
#include <Coherent/Libraries/Logging/ILogHandler.h>
#include <Coherent/UI/License.h>


class SystemEventListener : public Coherent::UI::EventListener
{
private:
    Coherent::UI::UISystem *system;
    bool isSystemReady;
public:
    SystemEventListener() 
        : system (nullptr)
        , isSystemReady(false) 
    { }

    virtual void SystemReady()
    {
        isSystemReady = true;
    }

public:

    bool IsSystemReady()
    {
        return isSystemReady;
    }

    void SetSystemReady(bool isSystemReady)
    {
        this->isSystemReady = isSystemReady;
    }
};

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
        enum EventType eventType;
        BrowserSource *source;
        HANDLE completionEvent;
        void *info;
        float timestamp;

        Event(EventType eventType, BrowserSource *source, float timestamp = 0.0f, bool isNotifyingOnCompletion = false) 
        {
            this->eventType = eventType;
            this->source = source;
            this->timestamp = timestamp;

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


    };
}

class BrowserLogHandler : public Coherent::Logging::ILogHandler
{
public:
	/// Called when a log message has to be written
	/// @param severity the severity of the message
	/// @param message the log message itself
	/// @param length the length of the message
    void WriteLog(Coherent::Logging::Severity severity, const char* message, size_t length)
    {
        wchar_t *severityString;
        switch (severity)
        {
        case Coherent::Logging::Trace:          severityString = TEXT("TRACE "); break;
        case Coherent::Logging::Debug:          severityString = TEXT("DEBUG "); break;
        case Coherent::Logging::Info:           severityString = TEXT("INFO  "); break;
        case Coherent::Logging::Warning:        severityString = TEXT("WARN  "); break;
        case Coherent::Logging::AssertFailure:  severityString = TEXT("ASSERT"); break;
        case Coherent::Logging::Error:          severityString = TEXT("ERROR "); break;
        }
            
        int messageBufferLength = utf8_to_wchar_len(message, length, 0);
        wchar_t *messageBuffer = static_cast<wchar_t *>(calloc(messageBufferLength + 1, sizeof(wchar_t)));
        utf8_to_wchar(message, length, messageBuffer, messageBufferLength, 0);

        Log(TEXT("BrowserPlugin::%s | %s"), severityString, messageBuffer);

        free(messageBuffer);
    }

	/// Called when an assert is triggered
	/// @param message a message that describes the reason for the assertion
	void Assert(const char* message)
    {
        // uh?
        //AppWarning(String(message));
    }
};

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

    CRITICAL_SECTION pendingEventLock;
    CRITICAL_SECTION pendingViewCreationLock;

    
    std::vector<BrowserSource *> webViews;
    //std::vector<JavascriptExtensionFactory *> javascriptExtensionFactories;
    std::vector<Browser::Event *> pendingEvents;

    Browser::Event *generalUpdate;

    bool isStarted;
    HANDLE hThread;
    HANDLE updateEvent;

    unsigned int pendingViewCreations;

public:
    BrowserManager() 
    {
		InitializeCriticalSection(&pendingEventLock);
        InitializeCriticalSection(&pendingViewCreationLock);

        generalUpdate = new Browser::Event(Browser::UPDATE, NULL, FLT_MAX);
        updateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        hThread = NULL;
        isStarted = false;  

        
        // setup the default extensions
        //javascriptExtensionFactories.Add(new AudioPlayerExtensionFactory());
        //javascriptExtensionFactories.Add(new KeyboardExtensionFactory());
        //javascriptExtensionFactories.push_back(new IrcExtensionFactory());
    }

    ~BrowserManager() 
    {
        RunAndWait(Browser::CLEANUP, NULL);

        // this is overkill
        // but make sure the thread has completed
        if (hThread) {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
            hThread = NULL;
        }

        assert(!isStarted);

       /* for (UINT i = 0; i < javascriptExtensionFactories.size(); i++) {
            delete javascriptExtensionFactories[i];
        }*/

        CloseHandle(updateEvent);
        
        DeleteCriticalSection(&pendingEventLock);
        DeleteCriticalSection(&pendingViewCreationLock);
        delete generalUpdate;
    }

protected:
    // this method should never be called directly
    void BrowserManagerEntry() 
    {
        SystemEventListener systemEventListener;
        BrowserLogHandler logHandler;
        LocalFileHandler localFileHandler;

        Coherent::UI::SystemSettings settings;
        settings.HostDirectory = L"plugins\\BrowserSourcePlugin\\host";
        settings.HTML5LocalStoragePath = L"storage";
        settings.AllowCookies = true;
        settings.CookiesResource = L"cookies.dat";
        settings.DisableWebSecurity = true;
        settings.CachePath = L"cache";
        settings.WriteMinidumps = true;
        settings.DebuggerPort = 1337;
        
        Coherent::UI::UISystem* system = InitializeUISystem(COHERENT_KEY, settings, &systemEventListener, Coherent::Logging::Debug, &logHandler/*, &localFileHandler*/);
        
        float currentTimestamp = 0.0f;

        for(;;) {

            WaitForSingleObject(updateEvent, 10);

            if (!systemEventListener.IsSystemReady()) {
                system->Update();
                continue;
            }

            while(pendingEvents.size()) {
                EnterCriticalSection(&pendingEventLock);
                Browser::Event *browserEvent = pendingEvents[0];
                LeaveCriticalSection(&pendingEventLock);

                switch(browserEvent->eventType) {
                case Browser::CREATE_VIEW: 
                    {
                        browserEvent->source->CreateWebViewCallback(system);
                        browserEvent->Complete();
                        delete browserEvent;
                        break;
                    }
                case Browser::SCENE_CHANGE:
                    {
                        system->Update();
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
                        if (browserEvent->timestamp != currentTimestamp) {
                            if (browserEvent->timestamp < FLT_MAX) {
                                currentTimestamp = browserEvent->timestamp;
                            }
                            system->Update();
                            system->FetchSurfaces();
                        }
                        
                        if (browserEvent->source != nullptr) {
                            delete browserEvent;
                            break;
                        }

                        browserEvent->Complete();

                        

                        break;
                    }
                case Browser::SHUTDOWN: 
                    {
                        if (browserEvent->source != nullptr) {

                            if (browserEvent->source->GetWebView() != nullptr) {
                                browserEvent->source->GetWebView()->Destroy();
                            }

                            EnterCriticalSection(&pendingEventLock);
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
                            LeaveCriticalSection(&pendingEventLock);
                        }

                        browserEvent->Complete();
                        delete browserEvent;
                        break;
                    }

                case Browser::CLEANUP:
                    {
                        system->Uninitialize();
                     
                        isStarted = false;

                        browserEvent->Complete();
                        delete browserEvent;

                        return;
                    }
                }

                EnterCriticalSection(&pendingEventLock);
                pendingEvents.erase(pendingEvents.begin());
                LeaveCriticalSection(&pendingEventLock);
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
        EnterCriticalSection(&pendingEventLock);
        pendingEvents.push_back(browserEvent);
        LeaveCriticalSection(&pendingEventLock);
        SetEvent(updateEvent);
    }

    void RunAndWait(Browser::EventType eventType, BrowserSource *browserSource) 
    {
        if (isStarted) {
            Browser::Event *blockingEvent = new Browser::Event(eventType, browserSource, 0.0f, true);
            AddEvent(blockingEvent);
            WaitForSingleObject(blockingEvent->completionEvent, INFINITE);
        }
    }

    void ShutdownAndWait(BrowserSource *browserSource)
    {
        RunAndWait(Browser::SHUTDOWN, browserSource);
    }

public:
    //WebCore *GetWebCore() 
    //{
    //    return webCore;
    //}

    //std::vector<JavascriptExtensionFactory *> &GetJavascriptExtensionFactories()
    //{
    //    return javascriptExtensionFactories;
    //}

};