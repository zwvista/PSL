// CopyCProject.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <sstream>
using namespace std;

std::wstring Replacewstring(std::wstring subject, const std::wstring& search,
    const std::wstring& replace) {
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::wstring::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

int main()
{
    wifstream wif(L"../Puzzles9/.cproject");
    wif.imbue(locale(locale::empty(), new codecvt_utf8<wchar_t, 0x10ffff, consume_header>));
    wstringstream wss;
    wss << wif.rdbuf();
    wif.close();
    wstring s = wss.str();

    for(char ch : {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'W', 'X', 'Y', 'Z'}){
        wstring prj = wstring(L"Puzzles") + wchar_t(ch);
        wofstream wof;
        wof.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::generate_header>)); 
        wof.open(L"../" + prj + L"/.cproject");
        wstring t = Replacewstring(s, L"Puzzles9", prj);
        wof << t;
        wof.close();
    }
    return 0;
}

