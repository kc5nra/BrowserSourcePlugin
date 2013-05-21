/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include <Awesomium\WebString.h>
#include <Awesomium\DataSource.h>
#include <Awesomium\WebString.h>
#include <Awesomium\STLHelpers.h>

#include "STLUtilities.h"

#include <cctype>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

using namespace Awesomium;


class DataSourceWithMimeType : public DataSource 
{

public:
    virtual ~DataSourceWithMimeType() {};

protected:
    std::map<std::string, std::string> mimeTypes;

public:
    void AddMimeType(const std::string &mimeType, const std::string &fileType) 
    {
        // keyed by file extension
        mimeTypes[fileType] = mimeType;
    }

    virtual WebString GetHost() = 0;
};



class BlankDataSource : public DataSourceWithMimeType 
{

private:
    bool isWrappingAsset;
    std::string assetWrapTemplate;
    int width;
    int height;

public:
    BlankDataSource(bool isWrappingAsset, const std::string &assetWrapTemplate, int width, int height) 
    { 
        this->isWrappingAsset = isWrappingAsset;
        this->assetWrapTemplate = assetWrapTemplate;
        this->width = width;
        this->height = height;
    }

public:
    virtual void OnRequest(int request_id, const WebString& path) {

        if (isWrappingAsset) {
            isWrappingAsset = false;

		    BSP::ReplaceStringInPlace(assetWrapTemplate, "$(WIDTH)", BSP::IntegerToString(width));
		    BSP::ReplaceStringInPlace(assetWrapTemplate, "$(HEIGHT)", BSP::IntegerToString(height));

            SendResponse(request_id,
			    assetWrapTemplate.length(),
                (unsigned char *)assetWrapTemplate.c_str(),
                WSLit("text/html"));
        } else {
            
            std::string filePath = ToString(path);
		    filePath = filePath.substr(0, filePath.find('?'));

		    std::ifstream ifs;
		    ifs.open(filePath, std::ifstream::binary);
		    std::vector<char> data;
		    if (ifs.good()) {
			    ifs.seekg(0, std::ifstream::end);
			    size_t file_size_in_byte = (size_t)ifs.tellg();
			    data.resize(file_size_in_byte);
			    ifs.seekg(0, std::ifstream::beg);
			    ifs.read(&data[0], file_size_in_byte);
		    }
		    ifs.close();

            std::string mimeType = "text/html";

		    // chop off ext and lower case it
            std::string fileExtension = filePath.substr(filePath.find_last_of(".") + 1);
		    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), std::tolower);

		    auto itor = mimeTypes.find(fileExtension);
		    if (itor != mimeTypes.end()) {
			    mimeType = itor->second;
		    }

            SendResponse(request_id,
                data.size(),
               (data.size()) ? (unsigned char *)&data[0] : NULL,
                ToWebString(mimeType));
        }
    }

public: //DataSourceWithMimeType

    WebString GetHost() 
    {
        return WSLit("blank");
    }
};

class BrowserDataSource : public DataSourceWithMimeType 
{

private:
    bool isWrappingAsset;
    std::string assetWrapTemplate;
    int width;
    int height;

public:
    BrowserDataSource(bool isWrappingAsset, const std::string &assetWrapTemplate, int width, int height) 
    { 
        this->isWrappingAsset = isWrappingAsset;
        this->assetWrapTemplate = assetWrapTemplate;
        this->width = width;
        this->height = height;
    }

public:
    virtual void OnRequest(int request_id, const WebString& path) 
    {
        
		std::string filePath = ToString(path);
		filePath = filePath.substr(0, filePath.find('?'));

		std::ifstream ifs;
		ifs.open(filePath, std::ifstream::binary);
		std::vector<char> data;
		if (ifs.good()) {
			ifs.seekg(0, std::ifstream::end);
			size_t file_size_in_byte = (size_t)ifs.tellg();
			data.resize(file_size_in_byte);
			ifs.seekg(0, std::ifstream::beg);
			ifs.read(&data[0], file_size_in_byte);
		}
		ifs.close();

        if (isWrappingAsset) {
            isWrappingAsset = false;

            std::string fileName = filePath.substr(filePath.find_last_of("/") + 1);

			BSP::ReplaceStringInPlace(assetWrapTemplate, "$(FILE)", fileName);
			BSP::ReplaceStringInPlace(assetWrapTemplate, "$(WIDTH)", BSP::IntegerToString(width));
            BSP::ReplaceStringInPlace(assetWrapTemplate, "$(HEIGHT)", BSP::IntegerToString(height));

            //TODO: Figure out what to do with this information
            // Since a lot of flash is vector art, it ends up 
            // making it super blurry if the actual size
            // is pretty small.

            //SwfReader swfReader((unsigned char *)lpFileDataUTF8);
            //if (!swfReader.HasError()) {
            //	swfWidth = swfReader.GetWidth();
            //	swfHeight = swfReader.GetHeight();
            //}

            SendResponse(request_id,
                strlen(assetWrapTemplate.c_str()),
                (unsigned char *)assetWrapTemplate.c_str(),
                WSLit("text/html"));

        } else {

            std::string mimeType = "text/html";

		    // chop off ext and lower case it
            std::string fileExtension = filePath.substr(filePath.find_last_of(".") + 1);
		    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), std::tolower);

		    auto itor = mimeTypes.find(fileExtension);
		    if (itor != mimeTypes.end()) {
			    mimeType = itor->second;
		    }

            SendResponse(request_id,
                data.size(),
               (data.size()) ? (unsigned char *)&data[0] : NULL,
                ToWebString(mimeType));
        }
    }


public: //DataSourceWithMimeType

    WebString GetHost() 
    {
        return WSLit("local");
    }
};