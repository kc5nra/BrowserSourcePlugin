/**
 * John Bradley (jrb@turrettech.com)
 */

#include "BrowserSource.h"

#include <Awesomium\WebCore.h>
#include <Awesomium\WebView.h>
#include <Awesomium\WebURL.h>
#include <Awesomium\STLHelpers.h>
#include <Awesomium\BitmapSurface.h>
#include <Awesomium\DataPak.h>

using namespace Awesomium;

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
	UpdateSettings();
    Log(TEXT("Using Browser Source"));

	size = Vect2(1000, 600);
	
	texture = CreateTexture(1000, 600, GS_BGRA, NULL, FALSE, FALSE);
}

BrowserSource::~BrowserSource()
{
}

void BrowserSource::Tick(float fSeconds)
{
}


void BrowserSource::Render(const Vect2 &pos, const Vect2 &size)
{
	if (!webCore) {
		// Create the WebCore singleton with default configuration
		WebConfig webConfig = WebConfig();
		webCore = WebCore::Initialize(webConfig);
		
		
		WebPreferences webPreferences;
		webPreferences.user_stylesheet = WSLit("::-webkit-scrollbar { visibility: hidden; } body { background-color: rgba(0, 0, 0, 0); }");
		
		webSession = webCore->CreateWebSession(WSLit("plugins\\BrowserSourcePlugin\\cache"), webPreferences);
		webSession->AddDataSource(WSLit("local"), new BrowserDataSource());
	
		webView = webCore->CreateWebView(1000, 600/*, webSession*/);
		webView->SetTransparent(true);
		WebURL url(WSLit("http://localhost:8080/movie.html"));
		webView->LoadURL(url);
		
	}
	
	webCore->Update();

	BitmapSurface *surface = (BitmapSurface *)webView->surface();
	if (surface) {
		texture->SetImage((void *)surface->buffer(), GS_IMAGEFORMAT_BGRA, surface->row_span());
		DrawSprite(texture, 0xFFFFFFFF, pos.x, pos.y, pos.x + size.x, pos.y + size.y);
	}
}

void BrowserSource::UpdateSettings()
{
}

Vect2 BrowserSource::GetSize() const 
{
	return size;
}
