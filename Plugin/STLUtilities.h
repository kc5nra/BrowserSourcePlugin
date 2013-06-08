/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include <string>
#include <vector>
namespace BSP {

    extern void ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace);
    extern std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace);

    std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems);
    std::vector<std::string> Split(const std::string &s, char delim); 

    extern std::string IntegerToString(int integer);
}