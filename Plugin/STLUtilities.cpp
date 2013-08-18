/**
* John Bradley (jrb@turrettech.com)
*/
#include "STLUtilities.h"
#include <windows.h>

#include <sstream>
#include <codecvt>

void BSP::ReplaceStringInPlace(
    std::string& subject, 
    const std::string& search,
    const std::string& replace) 
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

std::string BSP::ReplaceString(
    std::string subject, 
    const std::string& search,
    const std::string& replace) 
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

std::vector<std::string> &BSP::Split(
    const std::string &s, 
    char delim, 
    std::vector<std::string> &elems) 
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> BSP::Split(
    const std::string &s, 
    char delim) 
{
    std::vector<std::string> elems;
    BSP::Split(s, delim, elems);
    return elems;
}

std::string BSP::IntegerToString(int integer)
{
    std::stringstream s;
    s << integer;
    return s.str();
}

void BSP::ReplaceStringInPlace(
    std::wstring& subject, 
    const std::wstring& search,
    const std::wstring& replace) 
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

std::wstring BSP::ReplaceString(
    std::wstring subject, 
    const std::wstring& search,
    const std::wstring& replace) 
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

std::wstring BSP::IntegerToWString(int integer)
{
    std::wstringstream s;
    s << integer;
    return s.str();
}

extern std::string BSP::UTF16ToUTF8(std::wstring &utf16String)
{
    int len;
    int slength = (int)utf16String.length() + 1;
    len = WideCharToMultiByte(CP_UTF8, 0, utf16String.c_str(), slength, 0, 0, 0, 0); 
    char* buf = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, utf16String.c_str(), slength, buf, len, 0, 0); 
    std::string r(buf);
    delete[] buf;
    return r;
}

std::wstring BSP::UTF8ToUTF16(const std::string& utf8String)
{
    int len;
    int slength = (int)utf8String.length() + 1;
    len = MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), slength, 0, 0); 
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}