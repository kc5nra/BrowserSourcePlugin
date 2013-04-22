/**
 * John Bradley (jrb@turrettech.com)
 */

#include "BrowserSource.h"
#include "BrowserSourcePlugin.h"
#include "SwfReader.h"
#include "DataSources.h"

#include <Awesomium\WebCore.h>
#include <Awesomium\WebView.h>
#include <Awesomium\WebURL.h>
#include <Awesomium\STLHelpers.h>
#include <Awesomium\BitmapSurface.h>
#include <Awesomium\DataPak.h>
#include <Awesomium\JSValue.h>

using namespace Awesomium;

#define NO_VIEW -2
#define PENDING_VIEW -1

BrowserSource::BrowserSource(XElement *data)
{
	Log(TEXT("Using Browser Source"));

	hWebView = -2;

	config = new BrowserSourceConfig(data);

	InitializeCriticalSection(&textureLock);

	UpdateSettings();	
}

BrowserSource::~BrowserSource()
{
	BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();
	
	if (hWebView >= 0) {
		browserManager->ShutdownAndWait(hWebView);
	}

	EnterCriticalSection(&textureLock);
	delete texture;
	texture = NULL;
	LeaveCriticalSection(&textureLock);

	delete config;
	for (UINT i = 0; i < dataSources.Num(); i++) {
        delete dataSources[i];
    }
    dataSources.Clear();

	config = NULL;
}

void BrowserSource::Tick(float fSeconds)
{
}

WebView *BrowserSource::CreateWebViewCallback(WebCore *webCore, const int hWebView) {
	WebPreferences webPreferences;
	WebString webString = WebString((const wchar16 *)config->customCss.Array());
	webPreferences.user_stylesheet = webString;
	webPreferences.enable_web_gl = true;
	WebSession *webSession;
	webSession = webCore->CreateWebSession(WSLit("plugins\\BrowserSourcePlugin\\cache"), webPreferences);
	
	for (UINT i = 0; i < dataSources.Num(); i++) {
        delete dataSources[i];
    }
    dataSources.Clear();

    dataSources.Add(new BrowserDataSource(config->isWrappingAsset, config->assetWrapTemplate, config->width, config->height));
    dataSources.Add(new BlankDataSource(config->isWrappingAsset, config->assetWrapTemplate, config->width, config->height));
	
    for(UINT i = 0; i < dataSources.Num(); i++) {
        dataSources[i]->AddMimeType(TEXT(".jpg"), TEXT("image/jpg"));
	    dataSources[i]->AddMimeType(TEXT(".jpeg"), TEXT("image/jpeg"));
        webSession->AddDataSource(dataSources[i]->GetHost(), dataSources[i]);
    }

	WebView *webView;
	webView = webCore->CreateWebView(config->width, config->height, webSession);
	webView->SetTransparent(true);

	webString = WebString((const wchar16 *)config->url.Array());
	WebURL url(webString);
	webView->LoadURL(url);
	
	this->hWebView = hWebView;

	browserSize.x = float(config->width);
	browserSize.y = float(config->height);

	EnterCriticalSection(&textureLock);
	if (texture) {
		delete texture;
		texture = 0;
	}
	texture = CreateTexture(config->width, config->height, GS_BGRA, NULL, FALSE, FALSE);
	LeaveCriticalSection(&textureLock);

	return webView;
}




void BrowserSource::UpdateCallback(WebView *webView)
{
	BitmapSurface *surface = (BitmapSurface *)webView->surface();

	EnterCriticalSection(&textureLock);
	if (surface && texture) {
		texture->SetImage((void *)surface->buffer(), GS_IMAGEFORMAT_BGRA, surface->row_span());
	}
	LeaveCriticalSection(&textureLock);

}

void BrowserSource::Render(const Vect2 &pos, const Vect2 &size)
{
	BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();	
	
	// mini state machine
	switch(hWebView) {
		case NO_VIEW: 
		{
			hWebView = PENDING_VIEW;
			browserManager->AddEvent(new Browser::Event(Browser::CREATE_VIEW, this));

			break;
		}
		case PENDING_VIEW:
		{
			browserManager->Update();
			return;
		}
		default:
		{
			browserManager->AddEvent(new Browser::Event(Browser::UPDATE, this, hWebView));
			browserManager->Update();

			EnterCriticalSection(&textureLock);
			if (texture) {
				DrawSprite(texture, 0xFFFFFFFF, pos.x, pos.y, pos.x + size.x, pos.y + size.y);
			}
			LeaveCriticalSection(&textureLock);
		}
	}
}

void BrowserSource::UpdateSettings()
{
	BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();	

	if (hWebView == NO_VIEW) {
		hWebView = PENDING_VIEW;
	}

	config->Reload();
	browserManager->AddEvent(new Browser::Event(Browser::CREATE_VIEW, this, hWebView));
}

Vect2 BrowserSource::GetSize() const 
{
	return browserSize;
}
