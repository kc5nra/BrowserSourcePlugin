/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#include "OBSApi.h"

#include "KeyboardManager.h"
#include "Awesomium\DataSource.h"

namespace Awesomium {
	class WebCore;
	class WebSession;
	class WebView;
}

class DataSourceWithMimeType;

struct BrowserSourceConfig;

class BrowserSource : public ImageSource
{

public:
    BrowserSource(XElement *data);
    ~BrowserSource();

private:
	Vect2 browserSize;
	Texture *texture;
	int id;
	int hWebView;
	
    List<DataSourceWithMimeType *> dataSources;
	BrowserSourceConfig *config;

	CRITICAL_SECTION textureLock;

public:
    // ImageSource
	void Tick(float fSeconds);
    void Render(const Vect2 &pos, const Vect2 &size);
    void UpdateSettings();
    Vect2 GetSize() const;

// callbacks
public:
	Awesomium::WebView *CreateWebViewCallback(Awesomium::WebCore *webCore, const int hWebView);
	void UpdateCallback(Awesomium::WebView *webView);
	
	
};