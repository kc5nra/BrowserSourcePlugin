/**
 * John Bradley (jrb@turrettech.com)
 */


#include "BrowserSourcePlugin.h"
#include "Localization.h"
#include "BrowserSource.h"
#include "resource.h"

#include "Scintilla.h"
#include "SciLexer.h"
#include <Awesomium\WebCore.h>



HINSTANCE BrowserSourcePlugin::hinstDLL = NULL;
BrowserSourcePlugin *BrowserSourcePlugin::instance = NULL;

#define BROWSER_SOURCE_CLASS TEXT("BrowserSource")


LRESULT SendEditor(HWND editor, UINT Msg, WPARAM wParam=0, LPARAM lParam=0) {
	return SendMessage(editor, Msg, wParam, lParam);
}

const COLORREF black = RGB(0, 0, 0);
const COLORREF white = RGB(0xFF, 0xFF, 0xFF);

void SetAStyle(HWND editor, int style, COLORREF fore, COLORREF back=white, int size=-1, const char *face = 0) {
	SendEditor(editor, SCI_STYLESETFORE, style, fore);
	SendEditor(editor, SCI_STYLESETBACK, style, back);
	if (size >= 1) {
		SendEditor(editor, SCI_STYLESETSIZE, style, size);
	}
	if (face) {
		SendEditor(editor, SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
	}
}

const char htmlKeyWords[] = 
	"a abbr acronym address applet area b base basefont "
	"bdo big blockquote body br button caption center "
	"cite code col colgroup dd del dfn dir div dl dt em "
	"fieldset font form frame frameset h1 h2 h3 h4 h5 h6 "
	"head hr html i iframe img input ins isindex kbd label "
	"legend li link map menu meta noframes noscript "
	"object ol optgroup option p param pre q s samp "
	"script select small span strike strong style sub sup "
	"table tbody td textarea tfoot th thead title tr tt u ul "
	"var xmlns "
	"abbr accept-charset accept accesskey action align alink "
	"alt archive axis background bgcolor border "
	"cellpadding cellspacing char charoff charset checked cite "
	"class classid clear codebase codetype color cols colspan "
	"compact content coords "
	"data datafld dataformatas datapagesize datasrc datetime "
	"declare defer dir disabled enctype "
	"face for frame frameborder "
	"headers height href hreflang hspace http-equiv "
	"id ismap label lang language link longdesc "
	"marginwidth marginheight maxlength media method multiple "
	"name nohref noresize noshade nowrap "
	"object onblur onchange onclick ondblclick onfocus "
	"onkeydown onkeypress onkeyup onload onmousedown "
	"onmousemove onmouseover onmouseout onmouseup "
	"onreset onselect onsubmit onunload "
	"profile prompt readonly rel rev rows rowspan rules "
	"scheme scope shape size span src standby start style "
	"summary tabindex target text title type usemap "
	"valign value valuetype version vlink vspace width "
	"text password checkbox radio submit reset "
	"file hidden image "
	"public !doctype xml "
	"embed allowscriptaccess wmode";

const char jsKeyWords[] = 
	"break case catch continue default "
	"do else for function if return throw try var while";


void InitializeEditor(HWND editor) {
	SendEditor(editor, SCI_SETLEXER, SCLEX_HTML);
	SendEditor(editor, SCI_SETSTYLEBITS, 7);
	SendEditor(editor, SCI_SETTABWIDTH, 2);
	SendEditor(editor, SCI_SETTABINDENTS, 2);
	
	SendEditor(editor, SCI_SETKEYWORDS, 0, 
		reinterpret_cast<LPARAM>(htmlKeyWords));
	SendEditor(editor, SCI_SETKEYWORDS, 1, 
		reinterpret_cast<LPARAM>(jsKeyWords));

	const COLORREF red = RGB(0xFF, 0, 0);
	const COLORREF offWhite = RGB(0xFF, 0xFB, 0xF0);
	const COLORREF darkGreen = RGB(0, 0x80, 0);
	const COLORREF darkBlue = RGB(0, 0, 0x80);

	
	// Unknown tags and attributes are highlighed in red. 
	// If a tag is actually OK, it should be added in lower case to the htmlKeyWords string.
	SetAStyle(editor, SCE_H_TAG, darkBlue);
	SetAStyle(editor, SCE_H_TAGUNKNOWN, red);
	SetAStyle(editor, SCE_H_ATTRIBUTE, darkBlue);
	SetAStyle(editor, SCE_H_ATTRIBUTEUNKNOWN, red);
	SetAStyle(editor, SCE_H_NUMBER, RGB(0x80,0,0x80));
	SetAStyle(editor, SCE_H_DOUBLESTRING, RGB(0,0x80,0));
	SetAStyle(editor, SCE_H_SINGLESTRING, RGB(0,0x80,0));
	SetAStyle(editor, SCE_H_OTHER, RGB(0x80,0,0x80));
	SetAStyle(editor, SCE_H_COMMENT, RGB(0x80,0x80,0));
	SetAStyle(editor, SCE_H_ENTITY, RGB(0x80,0,0x80));

	SetAStyle(editor, SCE_H_TAGEND, darkBlue);
	SetAStyle(editor, SCE_H_XMLSTART, darkBlue);	// <?
	SetAStyle(editor, SCE_H_XMLEND, darkBlue);		// ?>
	SetAStyle(editor, SCE_H_SCRIPT, darkBlue);		// <script
	SetAStyle(editor, SCE_H_ASP, RGB(0x4F, 0x4F, 0), RGB(0xFF, 0xFF, 0));	// <% ... %>
	SetAStyle(editor, SCE_H_ASPAT, RGB(0x4F, 0x4F, 0), RGB(0xFF, 0xFF, 0));	// <%@ ... %>

	SetAStyle(editor, SCE_HB_DEFAULT, black);
	SetAStyle(editor, SCE_HB_COMMENTLINE, darkGreen);
	SetAStyle(editor, SCE_HB_NUMBER, RGB(0,0x80,0x80));
	SetAStyle(editor, SCE_HB_WORD, darkBlue);
	SendEditor(editor, SCI_STYLESETBOLD, SCE_HB_WORD, 1);
	SetAStyle(editor, SCE_HB_STRING, RGB(0x80,0,0x80));
	SetAStyle(editor, SCE_HB_IDENTIFIER, black);
	
		
	// If there is no need to support embedded Javascript, the following code can be dropped.
	// Javascript will still be correctly processed but will be displayed in just the default style.
	
	SetAStyle(editor, SCE_HJ_START, RGB(0x80,0x80,0));
	SetAStyle(editor, SCE_HJ_DEFAULT, black);
	SetAStyle(editor, SCE_HJ_COMMENT, darkGreen);
	SetAStyle(editor, SCE_HJ_COMMENTLINE, darkGreen);
	SetAStyle(editor, SCE_HJ_COMMENTDOC, darkGreen);
	SetAStyle(editor, SCE_HJ_NUMBER, RGB(0,0x80,0x80));
	SetAStyle(editor, SCE_HJ_WORD, black);
	SetAStyle(editor, SCE_HJ_KEYWORD, darkBlue);
	SetAStyle(editor, SCE_HJ_DOUBLESTRING, RGB(0x80,0,0x80));
	SetAStyle(editor, SCE_HJ_SINGLESTRING, RGB(0x80,0,0x80));
	SetAStyle(editor, SCE_HJ_SYMBOLS, black);

	SetAStyle(editor, SCE_HJA_START, RGB(0x80,0x80,0));
	SetAStyle(editor, SCE_HJA_DEFAULT, black);
	SetAStyle(editor, SCE_HJA_COMMENT, darkGreen);
	SetAStyle(editor, SCE_HJA_COMMENTLINE, darkGreen);
	SetAStyle(editor, SCE_HJA_COMMENTDOC, darkGreen);
	SetAStyle(editor, SCE_HJA_NUMBER, RGB(0,0x80,0x80));
	SetAStyle(editor, SCE_HJA_WORD, black);
	SetAStyle(editor, SCE_HJA_KEYWORD, darkBlue);
	SetAStyle(editor, SCE_HJA_DOUBLESTRING, RGB(0x80,0,0x80));
	SetAStyle(editor, SCE_HJA_SINGLESTRING, RGB(0x80,0,0x80));
	SetAStyle(editor, SCE_HJA_SYMBOLS, black);

	// Show the whole section of Javascript with off white background
	for (int jstyle=SCE_HJ_DEFAULT; jstyle<=SCE_HJ_SYMBOLS; jstyle++) {
		SendEditor(editor, SCI_STYLESETBACK, jstyle, offWhite);
		SendEditor(editor, SCI_STYLESETEOLFILLED, jstyle, 1);
	}
	SendEditor(editor, SCI_STYLESETBACK, SCE_HJ_STRINGEOL, RGB(0xDF, 0xDF, 0x7F));
	SendEditor(editor, SCI_STYLESETEOLFILLED, SCE_HJ_STRINGEOL, 1);

	// Show the whole section of Javascript with brown background
	for (int jastyle=SCE_HJA_DEFAULT; jastyle<=SCE_HJA_SYMBOLS; jastyle++) {
		SendEditor(editor, SCI_STYLESETBACK, jastyle, RGB(0xDF, 0xDF, 0x7F));
		SendEditor(editor, SCI_STYLESETEOLFILLED, jastyle, 1);
	}
	SendEditor(editor, SCI_STYLESETBACK, SCE_HJA_STRINGEOL, RGB(0x0,0xAF,0x5F));
	SendEditor(editor, SCI_STYLESETEOLFILLED, SCE_HJA_STRINGEOL, 1);

	

	ShowWindow(editor, SW_SHOW);
}

INT_PTR CALLBACK ConfigureDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
    {
        case WM_INITDIALOG:
        {
			SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
			BrowserSourceConfig *config = (BrowserSourceConfig *)lParam;

			config->hwndAssetWrapTemplateEditor = ::CreateWindow(
				TEXT("Scintilla"),
				TEXT("Source"),
				WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_BORDER,
				420, 50,
				375, 345,
				hwnd,
				0,
				BrowserSourcePlugin::hinstDLL,
				0);

			InitializeEditor(config->hwndAssetWrapTemplateEditor);

			LocalizeWindow(hwnd);

			HWND hwndUrlOrAsset = GetDlgItem(hwnd, IDC_URL_OR_ASSET);
			HWND hwndWidth = GetDlgItem(hwnd, IDC_WIDTH);
            HWND hwndHeight = GetDlgItem(hwnd, IDC_HEIGHT);
			HWND hwndCustomCss = GetDlgItem(hwnd, IDC_CUSTOM_CSS);
			HWND hwndIsWrappingAsset = GetDlgItem(hwnd, IDC_IS_WRAPPING_ASSET);
			HWND hwndAssetWrapTemplate = config->hwndAssetWrapTemplateEditor;

			SendMessage(hwndUrlOrAsset, WM_SETTEXT, 0, (LPARAM)config->url.Array());
			SendMessage(hwndWidth, WM_SETTEXT, 0, (LPARAM)IntString(config->width).Array());
			SendMessage(hwndHeight, WM_SETTEXT, 0, (LPARAM)IntString(config->height).Array());
			SendMessage(hwndCustomCss, WM_SETTEXT, 0, (LPARAM)config->customCss.Array());
			SendMessage(hwndIsWrappingAsset, BM_SETCHECK, config->isWrappingAsset, 0);
			char *utf8String = config->assetWrapTemplate.CreateUTF8String();
			SendEditor(hwndAssetWrapTemplate, SCI_ADDTEXT, config->assetWrapTemplate.Length(), (LPARAM)utf8String);
			Free(utf8String);
			SendEditor(hwndAssetWrapTemplate, SCI_SETREADONLY, !config->isWrappingAsset, 0);

			return TRUE;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam)) 
			{
				case IDC_IS_WRAPPING_ASSET:
				{
					BrowserSourceConfig *config = (BrowserSourceConfig *)GetWindowLongPtr(hwnd, DWLP_USER);
					HWND hwndIsWrappingAsset = GetDlgItem(hwnd, IDC_IS_WRAPPING_ASSET);
					HWND hwndAssetWrapTemplate = config->hwndAssetWrapTemplateEditor;

					bool isWrappingAsset = (SendMessage(hwndIsWrappingAsset, BM_GETCHECK, 0, 0) == 1);
					SendEditor(hwndAssetWrapTemplate, SCI_SETREADONLY, !isWrappingAsset, 0);
					return TRUE;
				}
				case IDOK:
				{
					BrowserSourceConfig *config = (BrowserSourceConfig *)GetWindowLongPtr(hwnd, DWLP_USER);

					HWND hwndUrlOrAsset = GetDlgItem(hwnd, IDC_URL_OR_ASSET);
					HWND hwndWidth = GetDlgItem(hwnd, IDC_WIDTH);
					HWND hwndHeight = GetDlgItem(hwnd, IDC_HEIGHT);
					HWND hwndCustomCss = GetDlgItem(hwnd, IDC_CUSTOM_CSS);
					HWND hwndIsWrappingAsset = GetDlgItem(hwnd, IDC_IS_WRAPPING_ASSET);
					HWND hwndAssetWrapTemplate = config->hwndAssetWrapTemplateEditor;

					String str;
					str.SetLength((UINT)SendMessage(hwndUrlOrAsset, WM_GETTEXTLENGTH, 0, 0));
                    if(!str.Length()) return TRUE;
                    SendMessage(hwndUrlOrAsset, WM_GETTEXT, str.Length()+1, (LPARAM)str.Array());
					config->url = str;

					str.SetLength((UINT)SendMessage(hwndWidth, WM_GETTEXTLENGTH, 0, 0));
                    if(!str.Length()) return TRUE;
                    SendMessage(hwndWidth, WM_GETTEXT, str.Length()+1, (LPARAM)str.Array());
					config->width = str.ToInt();

					str.SetLength((UINT)SendMessage(hwndHeight, WM_GETTEXTLENGTH, 0, 0));
                    if(!str.Length()) return TRUE;
                    SendMessage(hwndHeight, WM_GETTEXT, str.Length()+1, (LPARAM)str.Array());
					config->height = str.ToInt();

					str.SetLength((UINT)SendMessage(hwndCustomCss, WM_GETTEXTLENGTH, 0, 0));
                    if(str.Length()) {
						SendMessage(hwndCustomCss, WM_GETTEXT, str.Length()+1, (LPARAM)str.Array());
					} else {
						str.Clear();
					}

					config->isWrappingAsset = (SendMessage(hwndIsWrappingAsset, BM_GETCHECK, 0, 0) == 1);

					UINT length = (UINT)SendEditor(hwndAssetWrapTemplate, SCI_GETTEXTLENGTH);

					char *utf8String = (char *)Allocate(length + 1);
					utf8String[length] = 0;
					SendMessage(hwndAssetWrapTemplate, SCI_GETTEXT, length + 1, (LPARAM)utf8String);
					TSTR tstr = utf8_createTstr(utf8String);
					str = tstr;
					Free(tstr);
					Free(utf8String);
					
					config->assetWrapTemplate = str;

					EndDialog(hwnd, IDOK);
					return TRUE;
				}
				case IDCANCEL:
				{
					EndDialog(hwnd, IDCANCEL);
					return TRUE;
				}
			}
			break;
		}
		case WM_CLOSE:
		{
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		}
	}
	
	return FALSE;
}

ImageSource* STDCALL CreateBrowserSource(XElement *data)
{
	return new BrowserSource(data);
}

bool STDCALL ConfigureBrowserSource(XElement *element, bool bCreating)
{

	XElement *dataElement = element->GetElement(TEXT("data"));

	bool isMissingDataElement;
	if (isMissingDataElement = !dataElement) {
		dataElement = element->CreateElement(TEXT("data"));
	}

	BrowserSourceConfig *config = new BrowserSourceConfig(dataElement);
	if (isMissingDataElement) {
		config->Populate();
	}

	if(DialogBoxParam(BrowserSourcePlugin::hinstDLL, MAKEINTRESOURCE(IDD_BROWSERCONFIG), API->GetMainWindow(), ConfigureDialogProc, (LPARAM)config) == IDOK)
    {
		config->Save();
		element->SetInt(TEXT("cx"), config->width);
        element->SetInt(TEXT("cy"), config->height);

		delete config;
        return true;
    }

	delete config;
	return false;
}

BrowserSourcePlugin::BrowserSourcePlugin()
{
	isDynamicLocale = false;
	settings = NULL;
	browserManager = new BrowserManager();
	if (!locale->HasLookup(KEY("PluginName"))) {
		isDynamicLocale = true;
		int localizationStringCount = sizeof(localizationStrings) / sizeof(CTSTR);
		Log(TEXT("Browser Source plugin strings not found, dynamically loading %d strings"), sizeof(localizationStrings) / sizeof(CTSTR));
		for(int i = 0; i < localizationStringCount; i += 2) {
			locale->AddLookupString(localizationStrings[i], localizationStrings[i+1]);
		}
		if (!locale->HasLookup(KEY("PluginName"))) {
			AppWarning(TEXT("Uh oh..., unable to dynamically add our localization keys"));
		}
	}

	API->RegisterImageSourceClass(BROWSER_SOURCE_CLASS, STR("ClassName"), (OBSCREATEPROC)CreateBrowserSource, (OBSCONFIGPROC)ConfigureBrowserSource);

}

BrowserSourcePlugin::~BrowserSourcePlugin() 
{
	
	delete browserManager;

	if (isDynamicLocale) {
		int localizationStringCount = sizeof(localizationStrings) / sizeof(CTSTR);
		Log(TEXT("Browser Source plugin instance deleted; removing dynamically loaded localization strings"));
		for(int i = 0; i < localizationStringCount; i += 2) {
			locale->RemoveLookupString(localizationStrings[i]);
		}
	}
	isDynamicLocale = false;
}

bool LoadPlugin()
{
    if(BrowserSourcePlugin::instance != NULL) {
        return false;
	}
    BrowserSourcePlugin::instance = new BrowserSourcePlugin();

    return true;
}

void UnloadPlugin()
{
    if(BrowserSourcePlugin::instance == NULL) {
        return;
	}

    delete BrowserSourcePlugin::instance;
    BrowserSourcePlugin::instance = NULL;
}

void OnStartStream()
{
	if (BrowserSourcePlugin::instance == NULL) {
		return;
	}

	BrowserSourcePlugin *plugin = BrowserSourcePlugin::instance;
	plugin->GetBrowserManager()->Startup();
}

void OnStopStream()
{
}

CTSTR GetPluginName()
{
    return STR("PluginName");
}

CTSTR GetPluginDescription()
{
    return STR("PluginDescription");
}



BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH) {
        BrowserSourcePlugin::hinstDLL = hinstDLL;
	}
		
    return TRUE;
}
