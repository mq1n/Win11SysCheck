#pragma once
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <stdarg.h>

namespace Win11SysCheck
{
    static void FileLog(const std::string& stFileName, const std::string& stLogData)
    {
        std::ofstream f(stFileName.c_str(), std::ofstream::out | std::ofstream::app);
        f << stLogData.c_str() << std::endl;
        f.close();
    }

    static void FileLogf(const std::string& stFileName, const char* c_szFormat, ...)
    {
        char szBuffer[8192] = { 0 };

        va_list vaArgList;
        va_start(vaArgList, c_szFormat);
        vsprintf_s(szBuffer, c_szFormat, vaArgList);
        va_end(vaArgList);

        FileLog(stFileName.c_str(), szBuffer);
    }

    static void DebugLog(const char* c_szLogData)
    {
        OutputDebugStringA(c_szLogData);
    }

    static void DebugLogf(const char* c_szFormat, ...)
    {
        char szBuffer[8192] = { 0 };

        va_list vaArgList;
        va_start(vaArgList, c_szFormat);
        vsprintf_s(szBuffer, c_szFormat, vaArgList);
        va_end(vaArgList);

        DebugLog(szBuffer);
    }

    static void ConsoleLog(const char* c_szLogData)
    {
        fputs(c_szLogData, stdout);
    }

    static void ConsoleLogf(const char* c_szFormat, ...)
    {
        char szBuffer[8192] = { 0 };

        va_list vaArgList;
        va_start(vaArgList, c_szFormat);
        vsprintf_s(szBuffer, c_szFormat, vaArgList);
        va_end(vaArgList);

        ConsoleLog(szBuffer);
    }

    static void Logf(const std::string& stFileName, const char* c_szFormat, ...)
    {
        char szBuffer[8192] = { 0 };

        va_list vaArgList;
        va_start(vaArgList, c_szFormat);
        vsprintf_s(szBuffer, c_szFormat, vaArgList);
        va_end(vaArgList);

#ifdef _DEBUG
        DebugLog(szBuffer);
#endif
        ConsoleLog(szBuffer);
        FileLog(stFileName.c_str(), szBuffer);
    }
}
