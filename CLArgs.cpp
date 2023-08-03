#include "StdAfx.h"
#include ".\clargs.h"

CCLArgs::CCLArgs(int argc, _TCHAR* argv[])
{
    Init(argc, argv);
}

CCLArgs::CCLArgs()
{
}

void CCLArgs::Init(int argc, _TCHAR* argv[])
{
    m_bRelease = false;
    m_bPrintMatch = false;
    m_iTimeout = 0;
    m_bBoth = false;
    m_bAny = false;
    m_bVKRef = false;
    m_bExactMatch = false;
    m_bHook = false;
    m_bRegex = false;
    m_bCaseInsensitive = false;
    m_strCaption = _T("");
    m_strClass = _T("");
    m_bPreventPassthrough = false;
    m_bSkipMouse = false;

    int iArgIndex = 1;
    m_iNormalArgIndex = 0;

    while (iArgIndex < argc)
    {
        switch(argv[iArgIndex][0])
        {
            case '-':
            case '/':
                switch(argv[iArgIndex][1])
                {
                    case 't':
                    case 'T':
                        if (iArgIndex < argc-1)
                        {
                            iArgIndex++;
                            m_iTimeout = _ttoi(argv[iArgIndex]);
                        }
                        else
                            AddError(_T("You must provide a timeout with the -t option."));
                        break;
                    case 'c':
                    case 'C':
                        if (iArgIndex < argc-1)
                        {
                            iArgIndex++;
                            m_strCaption = argv[iArgIndex];
                        }
                        else
                            AddError(_T("You must provide a caption with the -c option."));
                        break;
                    case 's':
                    case 'S':
                        if (iArgIndex < argc-1)
                        {
                            iArgIndex++;
                            m_strClass = argv[iArgIndex];
                        }
                        else
                            AddError(_T("You must provide a class with the -s option."));
                        break;
                    default:
                        ProcessDashArgs(&argv[iArgIndex][1]);
                        break;
                }
                break;
            default:
                iArgIndex += ProcessNormalArg(argv[iArgIndex]);
        }

        iArgIndex++;
    }

    if (m_bRegex)
    {
        m_preCaption.Attach(new Regexp(m_strCaption.c_str(), m_bCaseInsensitive));
        m_preClass.Attach(new Regexp(m_strClass.c_str(), m_bCaseInsensitive));
    }

    if (m_arrayKeys.size() < 1 && !m_bVKRef)
    {
        AddError(_T("You must provide at least one key."));
    }

    if (!m_bHook && m_bExactMatch)
    {
        AddWarning(_T("-o is ignored if -k/-K is not present."));
    }
}

int CCLArgs::ProcessDashArgs(_TCHAR* szArgs)
{
    int iArg = 0;

    while (szArgs[iArg] != '\0')
    {
        switch(szArgs[iArg])
        {
            case 'r':
            case 'R':
                m_bRelease = true;
                break;
            case 'b':
            case 'B':
                m_bBoth = true;
                break;
            case 'a':
            case 'A':
                m_bAny = true;
                break;
            case 'v':
            case 'V':
                m_bVKRef = true;
                break;
            case 'k':
                m_bHook = true;
                m_bPreventPassthrough = true;
                break;
            case 'K':
                m_bHook = true;
                m_bPreventPassthrough = false;
                break;
            case 'm':
            case 'M':
                m_bSkipMouse = true;
                break;
            case 'o':
            case 'O':
                m_bExactMatch = true;
                break;
            case 'p':
            case 'P':
                m_bPrintMatch = true;
                break;
            case 'x':
            case 'X':
                m_bRegex = true;
                break;
            case 'i':
            case 'I':
                m_bCaseInsensitive = true;
                break;
            default:
                WarnInvalidArg(szArgs[iArg]);
                break;
        }
        iArg++;
    }

    return 0;
}

int CCLArgs::ProcessNormalArg(_TCHAR* szArg)
{
    m_iNormalArgIndex++;
    KeyInfo ki;
    ki.bAny = m_bAny;
    ki.bBoth = m_bBoth;
    ki.bRelease = m_bRelease;
    ki.strKeys = szArg;
    m_arrayKeys.push_back(ki);
    return 0;
}

CCLArgs::~CCLArgs(void)
{
}

bool CCLArgs::HasErrors()
{
    return (m_strErrors.length() > 0);
}

bool CCLArgs::HasWarnings()
{
    return (m_strWarnings.length() > 0);
}

TString CCLArgs::Errors()
{
    return m_strErrors;
}

TString CCLArgs::Warnings()
{
    return m_strWarnings;
}

void CCLArgs::WarnInvalidArg(_TCHAR c)
{
    _TCHAR szTemp[1024];

    _stprintf_s(szTemp, 1024, _T("Warning:  Ignoring invalid argument: %c\n"), c);

    m_strWarnings.append(szTemp);
}

void CCLArgs::WarnInvalidArg(_TCHAR *sz)
{
    _TCHAR szTemp[1024];

    _stprintf_s(szTemp, 1024, _T("Warning:  Ignoring invalid argument: %s\n"), sz);

    m_strWarnings.append(szTemp);
}

void CCLArgs::AddError(_TCHAR *sz)
{
    m_strErrors.append(_T("Error: "));
    m_strErrors.append(sz);
    m_strErrors.append(_T("\n"));
}

void CCLArgs::AddWarning(_TCHAR *sz)
{
    m_strWarnings.append(_T("Warning: "));
    m_strWarnings.append(sz);
    m_strWarnings.append(_T("\n"));
}

