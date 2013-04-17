#pragma once

#include <windows.h>
#include "OBSApi.h"
#include "BrowserSource.h"

#include <Awesomium\WebCore.h>
#include <Awesomium\WebView.h>
#include <Awesomium\WebURL.h>
#include <Awesomium\STLHelpers.h>
#include <Awesomium\BitmapSurface.h>
#include <Awesomium\DataPak.h>

using namespace Awesomium;

namespace Browser 
{
	enum EventType 
	{
		UPDATE,
		CREATE_VIEW,
		SHUTDOWN,
		CLEANUP
	};

	struct Event 
	{
		Event(EventType eventType, BrowserSource *source, int param = -1) {
			this->eventType = eventType;
			this->source = source;
			this->param = param;
		}

		enum EventType eventType;
		BrowserSource *source;
		int param;
	};
}

class BrowserManager
{

public:

	static DWORD WINAPI BrowserManagerEntry(LPVOID self) 
	{
        ((BrowserManager *)(self))->BrowserManagerEntry();
        return 0;
    }

public:
	BrowserManager() 
	{
		InitializeCriticalSection(&cs);

		generalUpdate = new Browser::Event(Browser::UPDATE, NULL);
		webCore = 0;
	}

	~BrowserManager() 
	{
		AddEvent(new Browser::Event(Browser::CLEANUP, NULL));
		while(isStarted) {
			Sleep(10);
		}
		delete generalUpdate;
	}

protected:
	// this method should never be called directly
    void BrowserManagerEntry() 
	{
		
		webCore = WebCore::Initialize(WebConfig());
		
		for(;;) {
			while(pendingEvents.Num()) {
				EnterCriticalSection(&cs);
				Browser::Event *browserEvent = pendingEvents.GetElement(0);
				LeaveCriticalSection(&cs);
				switch(browserEvent->eventType) {
					case Browser::CREATE_VIEW: 
					{
						webViews.Add(browserEvent->source->CreateWebViewCallback(webCore, webViews.Num()));
						delete browserEvent;
						break;
					}
					case Browser::UPDATE:
					{
						webCore->Update();
						if (browserEvent->source) {
							browserEvent->source->UpdateCallback(webViews.GetElement(browserEvent->param));
							delete browserEvent;
						}
						break;
					}
					case Browser::SHUTDOWN: 
					{
						while(webViews.Num()) {
							WebView *webView = webViews.GetElement(0);
							webView->session()->Release();
							webViews.Remove(0);
							webView->Destroy();
						}

						EnterCriticalSection(&cs);
						for(UINT i = 1; i < pendingEvents.Num(); i++) {
							// the only valid pending events at this point would be cleanup
							Browser::Event *pendingEvent = pendingEvents.GetElement(0);
							if (pendingEvent->eventType != Browser::CLEANUP) {
								pendingEvents.Remove(i);
								delete pendingEvent;
								i--;
							}
						}
						LeaveCriticalSection(&cs);
						break;
					}

					case Browser::CLEANUP:
					{
						while(webViews.Num()) {
							WebView *webView = webViews.GetElement(0);
							webViews.Remove(0);
							webView->Destroy();
						}

						webCore->Shutdown();
						
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


private:
	WebCore *webCore;
	CRITICAL_SECTION cs;
	List<WebView *> webViews;
	List<Browser::Event *> pendingEvents;
	Browser::Event *generalUpdate;

	bool isStarted;
	HANDLE threadHandle;

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
		if (isStarted) {
			EnterCriticalSection(&cs);
			pendingEvents.Add(browserEvent);
			LeaveCriticalSection(&cs);
		}
	}

public:
	WebCore *GetWebCore() 
	{
		return webCore;
	}


};