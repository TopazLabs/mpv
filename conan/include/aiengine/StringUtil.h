#pragma once
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <date/date.h>
#include <locale>
#include <codecvt>
#include <string>

#include "aieGlobal.h"

#define STRING_TRIM_CHARS " \t\n\r\f\v"
#define AIE_DATE_FORMAT "%Y-%m-%d"

namespace aiutils {
    namespace strutils {
        using namespace std;

        inline wstring toWString(string str) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.from_bytes(str);
        }

        inline string toString(wstring wstr) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.to_bytes(wstr);
        }

        inline bool startsWith(string const& value, string const& prefix) {
            return ((prefix.size() <= value.size()) && equal(prefix.begin(), prefix.end(), value.begin()));
        }

        inline bool endsWith(string value, string ending) {
            return ((ending.size() <= value.size()) && equal(ending.rbegin(), ending.rend(), value.rbegin()));
        }

        inline string toLower(string str) {
            transform(str.begin(), str.end(), str.begin(), ::tolower);
            return str;
        }

        inline string toUpper(string str) {
            transform(str.begin(), str.end(), str.begin(), ::toupper);
            return str;
        }

        inline bool contains(string str, string findStr) {
            return str.find(findStr) != string::npos;
        }

        inline string replaceAll(string subject, const string& search, const string& replace) {
           size_t pos = 0;
           while ((pos = subject.find(search, pos)) != string::npos) {
                subject.replace(pos, search.length(), replace);
                pos += replace.length();
           }
           return subject;
        }

        inline vector<string> split(string str, char delim) {
            vector<string> parts;
            istringstream f(str);
            string s;
            while (getline(f, s, delim)) {
                parts.push_back(s);
            }
            return parts;
        }

        inline string join(const vector<string>& strings, const char* const delim = ", ") {
            ostringstream imploded;
            copy(strings.begin(), strings.end(),
                       ostream_iterator<string>(imploded, delim));
            return imploded.str();
        }

        inline vector<string> keys(const Json& obj) {
            vector<string> keys;
            for(const auto& kv: obj.items())
                keys.push_back(kv.key());
            return keys;
        }

        inline string rtrim(const string& str, const char* t = STRING_TRIM_CHARS) {
            string s = str;
            s.erase(s.find_last_not_of(t) + 1);
            return s;
        }

        inline string ltrim(const string& str, const char* t = STRING_TRIM_CHARS) {
            string s = str;
            s.erase(0, s.find_first_not_of(t));
            return s;
        }

        inline string trim(const string& str, const char* t = STRING_TRIM_CHARS) {
            return ltrim(rtrim(str, t), t);
        }

        inline string getEnv(const string& name) {
            char* pStr = getenv(name.c_str());
            return pStr == nullptr ? "" : string(pStr);
        }

        inline int parseInt(const string& str, int defaultValue = 0) {
            try {
            return stoi(str);
            } catch (const invalid_argument& e) {
            return defaultValue;
            } catch (const out_of_range& e) {
            return defaultValue;
            }
        }
        
        inline date::sys_days parseDate(const string& dateString, const string& format = AIE_DATE_FORMAT) {
            date::sys_days ymd;
            istringstream ss(dateString);
            ss >> date::parse(format, ymd);
			if (ss.fail())
				AIE_WARNING() << "Error parsing date: " << dateString;
            return ymd;
        }

        inline string formatDate(const date::sys_days& ymd, const string& format = AIE_DATE_FORMAT) {
            stringstream ss;
            ss << date::format(format, ymd);
            return ss.str();
        }

        inline date::sys_days getTodayDate() {
            auto now = std::chrono::system_clock::now();
            return date::floor<date::days>(now);
        }

    }
}
