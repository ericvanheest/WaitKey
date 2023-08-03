#pragma once

#include "TString.h"
#include <vector>
#include "Regexp.h"

typedef struct
{
    bool bRelease;
    bool bBoth;
    bool bAny;
    std::wstring strKeys;
} KeyInfo;

class CCLArgs
{
public:
    CCLArgs(void);
    CCLArgs(int argc, _TCHAR* argv[]);
    ~CCLArgs(void);

    bool HasWarnings();
    bool HasErrors();
    TString Warnings();
    TString Errors();
    void Init(int argc, _TCHAR* argv[]);

private:
    int ProcessDashArgs(_TCHAR* szArgs);
    int ProcessNormalArg(_TCHAR* szArg);
    void WarnInvalidArg(_TCHAR c);
    void WarnInvalidArg(_TCHAR *sz);
    void AddError(_TCHAR *sz);
    void AddWarning(_TCHAR *sz);

    int m_iNormalArgIndex;
    TString m_strWarnings;
    TString m_strErrors;

public:
    bool m_bRelease;
    bool m_bVKRef;
    bool m_bExactMatch;
    bool m_bHook;
    unsigned int m_iTimeout;
    bool m_bBoth;
    bool m_bAny;
    bool m_bPrintMatch;
    bool m_bRegex;
    bool m_bCaseInsensitive;
    bool m_bPreventPassthrough;
    bool m_bSkipMouse;
    TString m_strCaption;
    TString m_strClass;
    CAutoPtr<Regexp> m_preCaption;
    CAutoPtr<Regexp> m_preClass;
    std::vector<KeyInfo> m_arrayKeys;

};
