#pragma once

#include "OBSApi.h"
#include <Awesomium\WebString.h>
#include <Awesomium\DataSource.h>
#include <Awesomium\WebString.h>
#include <Awesomium\STLHelpers.h>

using namespace Awesomium;

struct FileMimeType {
	FileMimeType(const String &fileType, const String &mimeType) {
		this->fileType = fileType;
		this->mimeType = mimeType;
	}

	String fileType;
	String mimeType;
};

class DataSourceWithMimeType : public DataSource {

public:
    virtual ~DataSourceWithMimeType() {
        for(UINT i = 0; i < mimeTypes.Num(); i++) {
			delete mimeTypes[i];
		}
    }

protected:
    List<FileMimeType *> mimeTypes;

public:
    void AddMimeType(const String &fileType, const String &mimeType) {
		mimeTypes.Add(new FileMimeType(fileType, mimeType));
	}
    
    virtual WebString GetHost() = 0;
};



class BlankDataSource : public DataSourceWithMimeType 
{

private:
	bool isWrappingAsset;
	String assetWrapTemplate;
	int width;
	int height;

public:
	BlankDataSource(bool isWrappingAsset, const String &assetWrapTemplate, int width, int height) 
	{ 
		this->isWrappingAsset = isWrappingAsset;
		this->assetWrapTemplate = assetWrapTemplate;
		this->width = width;
		this->height = height;
	}
	
public:
	virtual void OnRequest(int request_id, const WebString& path) {
		String pathString;
		char buffer[1025];
		path.ToUTF8(buffer, 1024);
		String filePath(buffer);
		
		String mimeType = TEXT("text/html");

		for(UINT i = 0; i < mimeTypes.Num(); i++) {
			FileMimeType *fileMimeType = mimeTypes.GetElement(i);
			if (fileMimeType->fileType.Length() > filePath.Length()) {
				continue;
			}
			String extractedType = filePath.Right(fileMimeType->fileType.Length());
			if (extractedType.CompareI(fileMimeType->fileType)) {
				mimeType = fileMimeType->mimeType;
				break;
			}
		}

		WebString wsMimeType = WebString((wchar16 *)mimeType.Array());

		if (isWrappingAsset) {
			isWrappingAsset = false;

			assetWrapTemplate.FindReplace(TEXT("$(WIDTH)"), IntString(width));
			assetWrapTemplate.FindReplace(TEXT("$(HEIGHT)"), IntString(height));

			LPSTR lpAssetWrapTemplate = assetWrapTemplate.CreateUTF8String();
			SendResponse(request_id,
				strlen(lpAssetWrapTemplate),
				(unsigned char *)lpAssetWrapTemplate,
				WSLit("text/html"));

			Free(lpAssetWrapTemplate);
				
			
		} else {
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
			    Log(TEXT("BrowserDataSource::OnRequest: could not open specified file %s (invalid file name or access violation)"), filePath);
		    }

			SendResponse(request_id,
				dwFileSize,
				(unsigned char *)lpFileDataUTF8,
				wsMimeType);

            Free(lpFileDataUTF8);
            file.Close();
		}

	}

public: //DataSourceWithMimeType

    WebString GetHost() {
        return WSLit("blank");
    }
};

class BrowserDataSource : public DataSourceWithMimeType 
{

private:
	bool isWrappingAsset;
	String assetWrapTemplate;
	int width;
	int height;

	List<FileMimeType *> mimeTypes;

public:
	BrowserDataSource(bool isWrappingAsset, const String &assetWrapTemplate, int width, int height) 
	{ 
		this->isWrappingAsset = isWrappingAsset;
		this->assetWrapTemplate = assetWrapTemplate;
		this->width = width;
		this->height = height;
	}
	
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
			Log(TEXT("BrowserDataSource::OnRequest: could not open specified file %s (invalid file name or access violation)"), filePath);
		}

		String mimeType = TEXT("text/html");

		for(UINT i = 0; i < mimeTypes.Num(); i++) {
			FileMimeType *fileMimeType = mimeTypes.GetElement(i);
			if (fileMimeType->fileType.Length() > filePath.Length()) {
				continue;
			}
			String extractedType = filePath.Right(fileMimeType->fileType.Length());
			if (extractedType.CompareI(fileMimeType->fileType)) {
				mimeType = fileMimeType->mimeType;
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
			//if (!swfReader.HasError()) {
			//	swfWidth = swfReader.GetWidth();
			//	swfHeight = swfReader.GetHeight();
			//}

			assetWrapTemplate.FindReplace(TEXT("$(WIDTH)"), IntString(width));
			assetWrapTemplate.FindReplace(TEXT("$(HEIGHT)"), IntString(height));

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


public: //DataSourceWithMimeType

    WebString GetHost() {
        return WSLit("local");
    }
};