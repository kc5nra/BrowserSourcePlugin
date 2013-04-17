/**
 * John Bradley (jrb@turrettech.com)
 */

#include "BrowserSource.h"
#include "BrowserSourcePlugin.h"

#include <Awesomium\WebCore.h>
#include <Awesomium\WebView.h>
#include <Awesomium\WebURL.h>
#include <Awesomium\STLHelpers.h>
#include <Awesomium\BitmapSurface.h>
#include <Awesomium\DataPak.h>

using namespace Awesomium;

#define NO_VIEW -2
#define PENDING_VIEW -1

class BrowserDataSource : public DataSource 
{
public:
	BrowserDataSource() { }
	virtual ~BrowserDataSource() { }
	
public:
	virtual void OnRequest(int request_id, const WebString& path) {
		String pathString;
		char buffer[1025];
		path.ToUTF8(buffer, 1024);
		String filePath(buffer);
		
		XFile file;
		String document;

		if(file.Open(filePath, XFILE_READ | XFILE_SHARED, XFILE_OPENEXISTING)) {
			file.ReadFileToString(document);
		} else {
			document = TEXT("");
			AppWarning(TEXT("BrowserDataSource::OnRequest: could not open specified file %s (invalid file name or access violation)"), buffer);
		}
		unsigned char *documentBuffer = (unsigned char *)document.CreateUTF8String();

		SendResponse(request_id,
				strlen((const char *)documentBuffer),
				documentBuffer,
				WSLit("text/html"));
		
	}
};

BrowserSource::BrowserSource(XElement *data)
{
	config = new BrowserSourceConfig(data);

	UpdateSettings();

    Log(TEXT("Using Browser Source"));

	hWebView = -2;

	browserDataSource = new BrowserDataSource();
}

BrowserSource::~BrowserSource()
{
	delete config;
	delete browserDataSource;
	delete texture;
}

void BrowserSource::Tick(float fSeconds)
{
}

WebView *BrowserSource::CreateWebViewCallback(WebCore *webCore, const int hWebView) {
	WebPreferences webPreferences;
	WebString webString = WebString((const wchar16 *)config->url.Array());
	webPreferences.user_stylesheet = webString;
	
	WebSession *webSession;
	webSession = webCore->CreateWebSession(WSLit("plugins\\BrowserSourcePlugin\\cache"), webPreferences);
	webSession->AddDataSource(WSLit("local"), browserDataSource);

	WebView *webView;
	webView = webCore->CreateWebView(config->width, config->height, webSession);
	webView->SetTransparent(true);

	//WebURL url(WSLit("asset://local/plugins/BrowserSourcePlugin/movie.html"));
	webString = WebString((const wchar16 *)config->url.Array());
	WebURL url(webString);
	webView->LoadURL(url);
	
	this->hWebView = hWebView;

	browserSize.x = float(config->width);
	browserSize.y = float(config->height);

	texture = CreateTexture(config->width, config->height, GS_BGRA, NULL, FALSE, FALSE);

	return webView;
}

void BrowserSource::UpdateCallback(WebView *webView)
{
	BitmapSurface *surface = (BitmapSurface *)webView->surface();
	if (surface) {
		texture->SetImage((void *)surface->buffer(), GS_IMAGEFORMAT_BGRA, surface->row_span());
	}
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

			if (texture) {
				DrawSprite(texture, 0xFFFFFFFF, pos.x, pos.y, pos.x + size.x, pos.y + size.y);
			}
		}
	}
}

void BrowserSource::UpdateSettings()
{
	BrowserManager *browserManager = BrowserSourcePlugin::instance->GetBrowserManager();	

	config->Reload();
	browserManager->AddEvent(new Browser::Event(Browser::CREATE_VIEW, this, hWebView));
}

Vect2 BrowserSource::GetSize() const 
{
	return browserSize;
}
