#pragma once

#include <string>

namespace BSP {

	extern void ReplaceStringInPlace(std::string& subject, const std::string& search, const std::string& replace);
	extern std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace);
	extern std::string IntegerToString(int integer);
}