// WaitKey.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "CLArgs.h"
#include <windows.h>
#include <winuser.h>
#include <stdio.h>
#include <conio.h>
#include "Regexp.h"

#define FILETIME_MILLISECONDS  (ULONGLONG) 10000

//   //   //   // 20//   //   //   // 40//   //   //   // 60//   //   //   // 80//
void Usage(_TCHAR* argv[])
{
    printf("\
WaitKey v1.09\n\
\n\
Usage:   WaitKey.exe [options] keys [keys [keys ...]]\n\
\n\
Options: -r  Wait for the keys to be released rather than pressed.\n\
         -b  Wait for the keys to be both pressed and released.\n\
         -a  Wait for any of the keys in a set, instead of all of them.\n\
         -k  Use hook instead of polling (prevents last key passthrough).\n\
         -K  Use hook instead of polling (no passthrough prevention).\n\
         -m  Do not set the mouse hook, only the keyboard hook.\n\
         -o  Use with -k/-K to require that ONLY the given key(s) are pressed\n\
         -p  Print the match to the console\n\
         -v  Print a virtual-key code reference and exit\n\
         -x  Use regular expressions for -c and -s matching options\n\
         -i  Ignore case for -c and -s matching options\n\
\n\
         -t  ms   Timeout after 'ms' milliseconds\n\
         -c  cap  Window with caption \"cap\" must be in the foreground\n\
         -s  cls  Window with class \"cls\" must be in the foreground\n\
\n\
The keys can be any ASCII characters.  WaitKey will set the errorlevel to the\n\
index of whichever set of keys was pressed/released first.\n\
\n\
Also predefined are:\n\
         \\c, \\s, \\a, \\w   (control, shift, alt, windows)\n\
         \\t, \\n, \\e  (tab, enter, escape)\n\
         \\l, \\r, \\u, \\d  (arrow keys - left, right, up, down)\n\
         \\F#  Function key #, where # is a hex digit, 1-c\n\
         \\v## Predefined virtual key, where ## is a hex value, 00-ff\n\
\n\
Example: WaitKey -r \\ca\n\
\n\
Note:  The -k option will stop the last keyboard message of a match from\n\
       being sent to the in-focus application.  However, if -b is used,\n\
       the last message is WM_KEYUP, which can cause odd behavior.  As such,\n\
       -b is not recommended when using -k.\n\
");
}

void PrintVKRef()
{
    printf("\
L-Mouse   01   Cancel    03   L-Win     5B   Net/Media:     OEM 8     DF\n\
R-Mouse   02   Clear     0C   R-Win     5C                  Process   E5\n\
M-Mouse   04   Help      2F   L-Shift   A0   Back      A6   ICO Clear E6\n\
X1-Mouse  05   Select    29   R-Shift   A1   Forward   A7   Packet    E7\n\
X2-Mouse  06   Capital   14   L-Control A2   Refresh   A8   Reset     E9\n\
               Convert   1C   R-Control A3   Stop      A9   Jump      EA\n\
Backspace 08   Nonconvt  1D   L-Alt     A4   Search    AA   PA1       EB\n\
Tab       09   Accept    1E   R-Alt     A5   Favorites AB   PA2       EC\n\
Return    0D   Modechg   1F                  Home      AC   PA3       ED\n\
Shift     10   Execute   2B   Numpad:        Mute      AD   WSCtrl    EE\n\
Control   11   Snapshot  2C                  Vol Down  AE   CUSel     EF\n\
Menu      12   Kana      15   0-9    60-69   Vol Up    AF   Attn      F0\n\
Pause     13   Junja     17   *         6A   Next Trk  B0   Finish    F1\n\
Escape    1B   Final     18   +         6B   Prev Trk  B1   Copy      F2\n\
Space     20   Kanji     19   -         6D   Stop      B2   Auto      F3\n\
Page Up   21   0-9    30-39   .         6E   Play      B3   ENLW      F4\n\
Page Down 22   A-Z    41-5A   /         6F   Mail      B4   BackTab   F5\n\
End       23   ;         BA   =         92   Media Sel B5   Attn      F6\n\
Home      24   =         BB                  App1      B6   CRSel     F7\n\
Left      25   ,         BC   Separator 6C   App2      B7   EXSel     F8\n\
Up        26   -         BD                                 EREOF     F9\n\
Right     27   .         BE   F1-F24 70-87   L-Oyayubi 95   Play      FA\n\
Down      28   /         BF                  R-Oyayubi 96   Zoom      FB\n\
Print     2a   `         C0   Numlock   90                  NoName    FC\n\
Insert    2D   [         DB   Scroll    91   Jap AX    E1   PA1       FD\n\
Delete    2E   \\         DC   Jisho     92   OEM 102   E2   OEM Clear FE\n\
Apps      5D   ]         DD   Masshou   93   ICO Help  E3\n\
Sleep     5F   '         DE   Touroku   94   ICO 00    E4\n\
");
}

HHOOK g_hhkKeyboard = NULL;
HHOOK g_hhkMouse = NULL;
CCLArgs g_clArgs;
bool g_bComplete = false;
short g_kbState[256] = {0};
short g_kbStateMatch[256] = {0};
int g_iResult = 0;

int HexCharToInt(char c)
{
    if (c >= '0' && c <= '9')
        return (c - '0');
    return c - 'a' + 10;
}

int HexCharToInt(wchar_t c)
{
    if (c >= L'0' && c <= L'9')
        return (c - L'0');
    return c - L'a' + 10;
}

SHORT GetWaitKeyState(int vKey)
{
    if (!g_clArgs.m_bHook)
        return GetAsyncKeyState(vKey);

    if (vKey < 256)
        return g_kbState[vKey];

    return 0;
}

void GetKBState()
{
    byte pbKeyState[256];
    GetKeyState(0);
    if (GetKeyboardState(pbKeyState))
    {
        for(int i = 0; i < 256; i++)
            g_kbState[i] = ((pbKeyState[i] & 0x80) << 8) | (pbKeyState[i] & 1);
    }
}

bool CheckKeys(KeyInfo *pKI)
{
    // If the selected window is not in the foreground, ignore all keys
    if (g_clArgs.m_strCaption.length() != 0 || g_clArgs.m_strClass.length() != 0)
    {
        HWND hWnd = GetForegroundWindow();
        TCHAR sz[1024];
        if (g_clArgs.m_strClass.length() != 0)
        {
            GetClassName(hWnd, sz, 1024);
            if (g_clArgs.m_bRegex)
            {
                if (!g_clArgs.m_preClass->Match(sz))
                    return false;
            }
            else if (g_clArgs.m_bCaseInsensitive)
            {
                if (_tcsicmp(sz, g_clArgs.m_strClass.c_str()))
                    return false;
            }
            else if (g_clArgs.m_strClass.compare(sz))
                return false;
        }
        if (g_clArgs.m_strCaption.length() != 0)
        {
            GetWindowText(hWnd, sz, 1024);
            if (g_clArgs.m_bRegex)
            {
                if (!g_clArgs.m_preCaption->Match(sz))
                    return false;
            }
            else if (g_clArgs.m_bCaseInsensitive)
            {
                if (_tcsicmp(sz, g_clArgs.m_strCaption.c_str()))
                    return false;
            }
            else if (g_clArgs.m_strCaption.compare(sz))
                return false;
        }
    }

    memcpy(g_kbStateMatch, g_kbState, sizeof(g_kbState));

    short vkAny = 0;
    bool bComplete = true;
    size_t iKey = 0;
    while (iKey < pKI->strKeys.length())
    {
        TCHAR c = pKI->strKeys.at(iKey);

        if (isalpha(c))
            c = toupper(c);

        short vk = c;
        if (c == '\\')
        {
            iKey++;
            c = pKI->strKeys.at(iKey);
            switch(c)
            {
                case _T('c'):
                case _T('C'):
                    vk = VK_LCONTROL;
                    break;
                case _T('a'):
                case _T('A'):
                    vk = VK_LMENU;
                    break;
                case _T('s'):
                case _T('S'):
                    vk = VK_LSHIFT;
                    break;
                case _T('t'):
                case _T('T'):
                    vk = VK_TAB;
                    break;
                case _T('e'):
                case _T('E'):
                    vk = VK_ESCAPE;
                    break;
                case _T('n'):
                case _T('N'):
                    vk = VK_RETURN;
                    break;
                case _T('l'):
                case _T('L'):
                    vk = VK_LEFT;
                    break;
                case _T('r'):
                case _T('R'):
                    vk = VK_RIGHT;
                    break;
                case _T('u'):
                case _T('U'):
                    vk = VK_UP;
                    break;
                case _T('d'):
                case _T('D'):
                    vk = VK_DOWN;
                    break;
                case _T('w'):
                case _T('W'):
                    vk = VK_LWIN;
                    break;
                case _T('F'):
                case _T('f'):
                    vk = VK_F1 + HexCharToInt(pKI->strKeys.at(iKey+1)) - 1;
                    iKey++;
                    break;
                case _T('v'):
                case _T('V'):
                    vk = HexCharToInt(pKI->strKeys.at(iKey+1)) << 4 | HexCharToInt(pKI->strKeys.at(iKey+2));
                    iKey += 2;
                    break;
                default:
                    vk = c;
                    break;
            }
        }
        else switch(c)  // Convert some common OEM characters
        {
            case ';': vk = 0xBA; break;
            case '=': vk = 0xBB; break;
            case ',': vk = 0xBC; break;
            case '-': vk = 0xBD; break;
            case '.': vk = 0xBE; break;
            case '/': vk = 0xBF; break;
            case '`': vk = 0xC0; break;
            case '[': vk = 0xDB; break;
            case '\\': vk = 0xDC; break;
            case ']': vk = 0xDD; break;
            case '\'': vk = 0xDE; break;
            default: break;
        }

        iKey++;

        if (vkAny != 0)
            vk = vkAny;

        unsigned short iKeyDown = GetWaitKeyState(vk);

        switch(vk)
        {
            case VK_LCONTROL:
                iKeyDown |= GetWaitKeyState(VK_RCONTROL);
                break;
            case VK_LMENU:
                iKeyDown |= GetWaitKeyState(VK_RMENU);
                break;
            case VK_LSHIFT:
                iKeyDown |= GetWaitKeyState(VK_RSHIFT);
                break;
            case VK_LWIN:
                iKeyDown |= GetWaitKeyState(VK_RWIN);
                break;
        }

        if (iKeyDown > 0x7fff)
            g_kbStateMatch[vk] = 0;

        bool bAny = true;

        if (pKI->bRelease && (iKeyDown > 0x7fff))
            bComplete = bAny = false;
        else if (!pKI->bRelease && (iKeyDown < 0x8000))
            bComplete = bAny = false;

        if (pKI->bAny && bAny)
        {
            bComplete = true;
            vkAny = vk;
            break;
        }
    }

    if (bComplete && g_clArgs.m_bExactMatch && g_clArgs.m_bHook)
    {
        for(int i = 0; i < 256; i++)
        {
            if ((g_kbStateMatch[i] & 0x8000) != 0)
                bComplete = false;      // Extra keys are down; not an exact match
        }
    }

    if (bComplete && pKI->bBoth)
    {
        pKI->bBoth = false;
        pKI->bRelease = true;
        bComplete = false;
    }

    return bComplete;
}

int CheckAllKeys()
{
    for(size_t iKeyIndex = 0; iKeyIndex < g_clArgs.m_arrayKeys.size(); iKeyIndex++)
    {
        if (CheckKeys(&(g_clArgs.m_arrayKeys.at(iKeyIndex))))
            return iKeyIndex;
    }
    return -1;
}

ULONGLONG GetLocalTimeInt64()
{
    SYSTEMTIME st;
    FILETIME ft;
    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &ft);
    LARGE_INTEGER li;
    li.HighPart = ft.dwHighDateTime;
    li.LowPart = ft.dwLowDateTime;
    return li.QuadPart;
}

LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) 
{
    if (nCode < 0)  // do not process message 
        return CallNextHookEx(g_hhkKeyboard, nCode, wParam, lParam); 

    // Store this key press or release in our local keyboard state array
    // wParam: windows message, i.e. WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, or WM_SYSKEYUP
    // lParam: KBDLLHOOKSTRUCT pointer

    if (lParam == 0)
        return CallNextHookEx(g_hhkKeyboard, nCode, wParam, lParam);

    KBDLLHOOKSTRUCT * pKB = (KBDLLHOOKSTRUCT *) lParam;

    g_kbState[pKB->vkCode & 0xff] = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN ? 0x8000 : 0x0000);

//    printf("VK[%2.2x] = %d\n", pKB->vkCode & 0xff, g_kbState[pKB->vkCode & 0xff]);

    g_iResult = CheckAllKeys();
    g_bComplete = (g_iResult > -1);

    if (g_bComplete && g_clArgs.m_bPreventPassthrough)
        return -1;

    return CallNextHookEx(g_hhkKeyboard, nCode, wParam, lParam);
}

LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam) 
{
    if (nCode < 0)  // do not process message 
        return CallNextHookEx(g_hhkMouse, nCode, wParam, lParam); 

    // Store this button press or release in our local keyboard state array
    // wParam: windows message, e.g. WM_LBUTTONDOWN, WM_LBUTTONUP, WM_MOUSEMOVE, WM_MOUSEWHEEL, WM_MOUSEHWHEEL, WM_RBUTTONDOWN, or WM_RBUTTONUP.
    // lParam: MSLLHOOKSTRUCT pointer

    if (lParam == 0)
        return CallNextHookEx(g_hhkMouse, nCode, wParam, lParam);

    MSLLHOOKSTRUCT * pMouse = (MSLLHOOKSTRUCT *) lParam;
    
    DWORD vKey = 0;
    short iDown = 0;
    const short KeyDown = (short) 0x8000;

    switch(wParam)
    {
        case WM_LBUTTONDOWN:
        case WM_NCLBUTTONDOWN:
            vKey = VK_LBUTTON;
            iDown = KeyDown;
            break;
        case WM_RBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
            vKey = VK_RBUTTON;
            iDown = KeyDown;
            break;
        case WM_MBUTTONDOWN:
        case WM_NCMBUTTONDOWN:
            vKey = VK_MBUTTON;
            iDown = KeyDown;
            break;
        case WM_XBUTTONDOWN:
        case WM_NCXBUTTONDOWN:
            vKey = (HIWORD(pMouse->mouseData) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
            iDown = KeyDown;
            break;
        case WM_LBUTTONUP:
        case WM_NCLBUTTONUP:
            vKey = VK_LBUTTON;
            break;
        case WM_RBUTTONUP:
        case WM_NCRBUTTONUP:
            vKey = VK_RBUTTON;
            break;
        case WM_MBUTTONUP:
        case WM_NCMBUTTONUP:
            vKey = VK_MBUTTON;
            break;
        case WM_XBUTTONUP:
        case WM_NCXBUTTONUP:
            vKey = (HIWORD(pMouse->mouseData) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2);
            break;
        default:
            break;
    }

    g_kbState[vKey & 0xff] = iDown;

    g_iResult = CheckAllKeys();
    g_bComplete = (g_iResult > -1);

    if (g_bComplete && g_clArgs.m_bPreventPassthrough)
        return -1;

    return CallNextHookEx(g_hhkMouse, nCode, wParam, lParam);
}

void DoEvents()
{
    MSG msg;
    BOOL result;

    while ( ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE ) )
    {
        result = ::GetMessage(&msg, NULL, 0, 0);
        if (result == 0) // WM_QUIT
        {                
            ::PostQuitMessage(msg.wParam);
            break;
        }
        else if (result == -1)
        {
             // Handle errors/exit application, etc.
        }
        else 
        {
            ::TranslateMessage(&msg);
            :: DispatchMessage(&msg);
        }
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    if (argc < 2)
    {
        Usage(argv);
        return 0;
    }

    g_clArgs.Init(argc, argv);

    if (g_clArgs.HasErrors())
    {
        _tprintf(g_clArgs.Errors().c_str());
        return -1;
    }

    if (g_clArgs.HasWarnings())
    {
        _tprintf(g_clArgs.Warnings().c_str());
    }

    if (g_clArgs.m_bVKRef)
    {
        PrintVKRef();
        return 0;
    }

    GetKBState();

    //printf("Waiting for keys...\n");
    g_bComplete = false;

    ULONGLONG llStart = GetLocalTimeInt64();
    bool bTimeout = false;

    if (g_clArgs.m_bHook)
    {
        g_hhkKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, NULL, 0);
        if (g_hhkKeyboard == NULL)
        {
            fprintf(stderr, "Error setting keyboard hook: %d\n", GetLastError());
            return -1;
        }
        if (!g_clArgs.m_bSkipMouse)
        {
            g_hhkMouse = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, NULL, 0);
            if (g_hhkMouse == NULL)
            {
                fprintf(stderr, "Error setting mouse hook: %d\n", GetLastError());
                return -1;
            }
        }
    }

    while (!g_bComplete)
    {
        if (!g_clArgs.m_bHook)
        {
            g_iResult = CheckAllKeys();
            g_bComplete = (g_iResult > -1);
        }

        if (g_clArgs.m_bHook)
        {
            DoEvents();
        }

        if (g_bComplete)
            break;

        Sleep(10);

        while (_kbhit())
            _getch();

        if (g_clArgs.m_iTimeout > 0)
        {
            ULONGLONG llNow = GetLocalTimeInt64();
            if ((llNow - llStart) > (FILETIME_MILLISECONDS * g_clArgs.m_iTimeout))
            {
                printf("Timed out.\n");
                bTimeout = true;
                break;
            }
        }
    }

    if (g_clArgs.m_bPrintMatch && g_iResult > -1)
    {
        printf("Matched arg %d (%s)\n", g_iResult, g_clArgs.m_arrayKeys.at(g_iResult).strKeys.c_str());
    }

    while (_kbhit())
        _getch();

    if (g_hhkKeyboard)
    {
        UnhookWindowsHookEx(g_hhkKeyboard);
        g_hhkKeyboard = NULL;
    }

    if (g_hhkMouse)
    {
        UnhookWindowsHookEx(g_hhkMouse);
        g_hhkMouse= NULL;
    }

    if (bTimeout)
        return -1;
    else
    	return (int) g_iResult+1;
}

