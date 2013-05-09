/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#include "OBSApi.h"

#include "KeyboardManager.h"
#include "Awesomium\JSObject.h"

namespace Awesomium {
	class WebCore;
	class WebSession;
	class WebView;
    class JSArray;
    class JSValue;
    class JSMethodHandler;
}

class DataSourceWithMimeType;
class JavascriptExtension;

struct BrowserSourceConfig;

class BrowserSource : public ImageSource
{
    class BrowserSourceListener;

public:
    BrowserSource(XElement *data);
    ~BrowserSource();

private:
	Vect2 browserSize;
	Texture *texture;
	int id;
	int hWebView;
    bool hasRegisteredJavascriptExtensions;

	unsigned int hJSGlobal;
    
    BrowserSourceListener *browserSourceListener;

    List<JavascriptExtension *> javascriptExtensions;
    List<DataSourceWithMimeType *> dataSources;
	BrowserSourceConfig *config;


	CRITICAL_SECTION textureLock;
protected:
    CRITICAL_SECTION jsGlobalLock;

public:
    // ImageSource
	void Tick(float fSeconds);
    void Render(const Vect2 &pos, const Vect2 &size);
    void ChangeScene();
    void UpdateSettings();
    Vect2 GetSize() const;
    int GetWebView() { return hWebView; }

// callbacks
public:
	Awesomium::WebView *CreateWebViewCallback(Awesomium::WebCore *webCore, const int hWebView);
	void UpdateCallback(Awesomium::WebView *webView);
	void SceneChangeCallback(Awesomium::WebView *webView);
	
};