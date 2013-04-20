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
		Event(EventType eventType, BrowserSource *source, int webView = -1, bool isNotifyingOnCompletion = false) {
			this->eventType = eventType;
			this->source = source;
			this->webView = webView;
			if (isNotifyingOnCompletion) {
				this->completionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			} else {
				this->completionEvent = NULL;
			}
		}

		~Event() {
		}
		
		void Complete() {
			if (completionEvent) {
				SetEvent(completionEvent);
			}
		}

		// I don't like the spelling Canceled
		void Cancelled() {
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
        ((BrowserManager *)(self))->BrowserManagerEntry();
        return 0;
    }

public:
	BrowserManager() 
	{
		InitializeCriticalSection(&cs);

		generalUpdate = new Browser::Event(Browser::UPDATE, NULL);
		webCore = 0;

		updateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
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
		WebConfig webConfig = WebConfig();
		webCore = WebCore::Initialize(webConfig);
		UINT maxFps = API->GetMaxFPS();

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
					case Browser::UPDATE:
					{
						webCore->Update();

						if (browserEvent->source) {
							browserEvent->source->UpdateCallback(webViews.GetElement(browserEvent->webView));
							browserEvent->Complete();
							delete browserEvent;
						}

						browserEvent->Complete();
						break;
					}
					case Browser::SHUTDOWN: 
					{
						if (browserEvent->webView >= 0) {
							
							WebView *webView = webViews.GetElement(browserEvent->webView);
							if (webView) {
								webView->LoadURL(WebURL(WSLit("about:blank")));
								// fixes a bug in awesomium where if you destroy it while a flash application
								// is running it may fail when doing in the future.
								while(webView->IsLoading()) {
									webCore->Update();
									Sleep(20);
								}
								webView->Destroy();
								webViews.Remove(browserEvent->webView);
								webViews.Insert(browserEvent->webView, NULL);
							}

							EnterCriticalSection(&cs);
							for(UINT i = 1; i < pendingEvents.Num(); i++) {
								// remove all events belonging to this web view
								// this would include only Update, Shutdown and Create View requests
								Browser::Event *pendingEvent = pendingEvents.GetElement(0);
								if (pendingEvent->webView == browserEvent->webView) {
									pendingEvents.Remove(i);
									pendingEvent->Cancelled();
									delete pendingEvent;
								}
							}
							LeaveCriticalSection(&cs);
						}

						browserEvent->Complete();
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


private:
	WebCore *webCore;
	CRITICAL_SECTION cs;

	List<WebView *> webViews;
	List<Browser::Event *> pendingEvents;
	Browser::Event *generalUpdate;
	
	bool isStarted;
	HANDLE threadHandle;
	HANDLE updateEvent;

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

	void ShutdownAndWait(int hWebView)
	{
		Browser::Event *shutdownEvent = new Browser::Event(Browser::SHUTDOWN, NULL, hWebView, true);
		AddEvent(shutdownEvent);
		WaitForSingleObject(shutdownEvent->completionEvent, INFINITE);
		delete shutdownEvent;
	}

public:
	WebCore *GetWebCore() 
	{
		return webCore;
	}


};