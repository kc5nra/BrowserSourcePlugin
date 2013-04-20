/**
 * John Bradley (jrb@turrettech.com)
 */

#include "BrowserSource.h"
#include "BrowserSourcePlugin.h"
#include "SwfReader.h"

#include <Awesomium\WebCore.h>
#include <Awesomium\WebView.h>
#include <Awesomium\WebURL.h>
#include <Awesomium\STLHelpers.h>
#include <Awesomium\BitmapSurface.h>
#include <Awesomium\DataPak.h>

using namespace Awesomium;

#define NO_VIEW -2
#define PENDING_VIEW -1

struct FileMimeType {
	FileMimeType(const String &fileType, const String &mimeType) {
		this->fileType = fileType;
		this->mimeType = mimeType;
	}

	String fileType;
	String mimeType;
};

class BrowserDataSource : public DataSource 
{

private:
	bool isWrappingAsset;
	String assetWrapTemplate;
	int swfWidth;
	int swfHeight;

	List<FileMimeType> mimeTypes;

public:
	BrowserDataSource(bool isWrappingAsset, const String &assetWrapTemplate, int swfWidth, int swfHeight) 
	{ 
		this->isWrappingAsset = isWrappingAsset;
		this->assetWrapTemplate = assetWrapTemplate;
		this->swfWidth = swfWidth;
		this->swfHeight = swfHeight;
	}

	virtual ~BrowserDataSource() { }
	
public:
	virtual void OnRequest(int request_id, const WebString& path) {
		String pathString;
		char buffer[1025];
		path.ToUTF8(buffer, 1024);
		String filePath(buffer);
		
		XFile file;

		LPSTR lpFileDataUTF8 = 0;
		DWORD dwFileSize = 0;
		if(file.Open(filePath, XFILE_READ | XFILE_SHARED, XFILE_OPENEXISTING)) {
			file.SetPos(0, XFILE_BEGIN);
			dwFileSize = (DWORD)file.GetFileSize();
			lpFileDataUTF8 = (LPSTR)Allocate(dwFileSize+1);
			lpFileDataUTF8[dwFileSize] = 0;
			file.Read(lpFileDataUTF8, dwFileSize);
		} else {
			Log(TEXT("BrowserDataSource::OnRequest: could not open specified file %s (invalid file name or access violation)"), buffer);
		}

		String mimeType = TEXT("text/html");

		for(UINT i = 0; i < mimeTypes.Num(); i++) {
			FileMimeType &fileMimeType = mimeTypes.GetElement(i);
			if (fileMimeType.fileType.Length() > filePath.Length()) {
				continue;
			}
			if (filePath.Right(filePath.Length() - fileMimeType.fileType.Length()).CompareI(fileMimeType.fileType)) {
				mimeType = fileMimeType.mimeType;
				break;
			}
		}

		WebString wsMimeType = WebString((wchar16 *)mimeType.Array());

		if (isWrappingAsset) {
			isWrappingAsset = false;

			String fileName;

			for(UINT i = filePath.Length() - 1; i >= 0; i--) {
				if (filePath[i] == '/') {
					fileName = filePath.Right(filePath.Length() - i - 1);
					break;
				}
			}

			assetWrapTemplate.FindReplace(TEXT("$(FILE)"), fileName);
			
			//TODO: Figure out what to do with this information
			// Since a lot of flash is vector art, it ends up 
			// making it super blurry if the actual size
			// is pretty small.

			//SwfReader swfReader((unsigned char *)lpFileDataUTF8);
			//if (!swfReader.hasError()) {
			//	swfWidth = swfReader.getWidth();
			//	swfHeight = swfReader.getHeight();
			//}

			assetWrapTemplate.FindReplace(TEXT("$(WIDTH)"), IntString(swfWidth));
			assetWrapTemplate.FindReplace(TEXT("$(HEIGHT)"), IntString(swfHeight));

			LPSTR lpAssetWrapTemplate = assetWrapTemplate.CreateUTF8String();
			SendResponse(request_id,
				strlen(lpAssetWrapTemplate),
				(unsigned char *)lpAssetWrapTemplate,
				WSLit("text/html"));

			Free(lpAssetWrapTemplate);
				
			
		} else {
			SendResponse(request_id,
				dwFileSize,
				(unsigned char *)lpFileDataUTF8,
				wsMimeType);
		}

		
		
		Free(lpFileDataUTF8);

		file.Close();
	}

	void AddMimeType(const String &fileType, const String &mimeType) {
		mimeTypes.Add(FileMimeType(fileType, mimeType));
	}
};

BrowserSource::BrowserSource(XElement *data)
{
	Log(TEXT("Using Browser Source"));

	hWebView = -2;
	browserDataSource = NULL;

	config = new BrowserSourceConfig(data);

	InitializeCriticalSection(&textureLock);

	UpdateSettings();	
}

BrowserSource::~BrowserSource()
{

	if (hWebView >= 0) {
		BrowserSourcePlugin::instance->GetBrowserManager()->ShutdownAndWait(hWebView);
	}

	EnterCriticalSection(&textureLock);
	delete texture;
	texture = NULL;
	LeaveCriticalSection(&textureLock);

	delete config;
	delete browserDataSource;
	browserDataSource = NULL;
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
	
	if (browserDataSource) {
		delete browserDataSource;
	}
	browserDataSource = new BrowserDataSource(config->isWrappingAsset, config->assetWrapTemplate, config->width, config->height);
	webSession->AddDataSource(WSLit("local"), browserDataSource);

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
