/**
 * John Bradley (jrb@turrettech.com)
 */
#pragma once

#include "OBSApi.h"

namespace Awesomium {
	class WebCore;
	class WebSession;
	class WebView;
}

class BrowserDataSource;
struct BrowserSourceConfig;

class BrowserSource : public ImageSource
{

public:
    BrowserSource(XElement *data);
    ~BrowserSource();

private:
	Vect2 browserSize;
	Texture *texture;
	int hWebView;
	BrowserDataSource *browserDataSource;
	BrowserSourceConfig *config;

public:
    void Tick(float fSeconds);
    void Render(const Vect2 &pos, const Vect2 &size);
    void UpdateSettings();
    Vect2 GetSize() const;


// callbacks
public:
	Awesomium::WebView *CreateWebViewCallback(Awesomium::WebCore *webCore, const int hWebView);
	void UpdateCallback(Awesomium::WebView *webView);
};