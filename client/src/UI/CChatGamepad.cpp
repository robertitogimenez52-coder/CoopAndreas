#include "stdafx.h"
#include "CChatGamepad.h"
#include <Xinput.h>

// the game doesn't update CPad while the chat is open (see CChat::ToggleInput), so the gamepad is read with XInput
typedef DWORD(WINAPI* XInputGetState_t)(DWORD dwUserIndex, XINPUT_STATE* pState);

static XInputGetState_t ms_pfnXInputGetState = nullptr;
static bool ms_bXInputLoaded = false;

static constexpr int KEYBOARD_COLUMNS = 10;
static constexpr int KEYBOARD_ROWS = 5;  // the last row is the special keys
static constexpr int NUM_PAGES = 3;

static const wchar_t* ms_aaKeyboard[NUM_PAGES][KEYBOARD_ROWS - 1] = {
    {L"1234567890", L"qwertyuiop", L"asdfghjkl'", L"zxcvbnm,.?"},
    {L"1234567890", L"QWERTYUIOP", L"ASDFGHJKL\"", L"ZXCVBNM,.!"},
    {L"!@#$%^&*()", L"-_=+[]{};:", L"<>/\\|~`'\"?", L"0123456789"},
};

enum eSpecialKey
{
    SPECIAL_KEY_SPACE,
    SPECIAL_KEY_BACKSPACE,
    SPECIAL_KEY_PAGE,
    SPECIAL_KEY_SEND,
    SPECIAL_KEY_CLOSE,

    SPECIAL_KEY_COUNT
};

static const wchar_t* ms_aSpecialKeys[SPECIAL_KEY_COUNT] = {L"Space", L"Del", L"Aa#", L"Send", L"Close"};

static bool ms_bActive = false;
static bool ms_bClosePending = false;  // waiting for the buttons to be released, so the game doesn't get them
static int ms_nPage = 0;
static int ms_nRow = 1;
static int ms_nColumn = 0;

static WORD ms_wLastButtons = 0;
static WORD ms_wRepeatButton = 0;
static unsigned int ms_nRepeatTime = 0;

static constexpr SHORT STICK_DEAD_ZONE = 16000;
static constexpr unsigned int REPEAT_DELAY = 400;
static constexpr unsigned int REPEAT_INTERVAL = 90;

static bool ReadGamepad(XINPUT_STATE& state)
{
    if (!ms_bXInputLoaded)
    {
        ms_bXInputLoaded = true;

        for (const char* szDll : {"xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"})
        {
            if (HMODULE hModule = LoadLibraryA(szDll))
            {
                ms_pfnXInputGetState = (XInputGetState_t)GetProcAddress(hModule, "XInputGetState");
                if (ms_pfnXInputGetState)
                    break;
            }
        }
    }

    if (!ms_pfnXInputGetState)
        return false;

    memset(&state, 0, sizeof(state));
    return ms_pfnXInputGetState(0, &state) == ERROR_SUCCESS;
}

// the left stick works like the d-pad
static WORD GetButtons(const XINPUT_GAMEPAD& gamepad)
{
    WORD buttons = gamepad.wButtons;

    if (gamepad.sThumbLX < -STICK_DEAD_ZONE)
        buttons |= XINPUT_GAMEPAD_DPAD_LEFT;
    if (gamepad.sThumbLX > STICK_DEAD_ZONE)
        buttons |= XINPUT_GAMEPAD_DPAD_RIGHT;
    if (gamepad.sThumbLY > STICK_DEAD_ZONE)
        buttons |= XINPUT_GAMEPAD_DPAD_UP;
    if (gamepad.sThumbLY < -STICK_DEAD_ZONE)
        buttons |= XINPUT_GAMEPAD_DPAD_DOWN;
    if (gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        buttons |= 0x0400;  // unused by XInput, the left trigger

    return buttons;
}

static constexpr WORD BUTTON_LEFT_TRIGGER = 0x0400;

void CChatGamepad::Open()
{
    ms_bActive = true;
    ms_bClosePending = false;
    ms_nRow = 1;
    ms_nColumn = 0;

    if (!CChat::m_bInputActive)
    {
        CChat::ToggleInput(true);
    }
}

void CChatGamepad::Close(bool bSubmit)
{
    ms_bActive = false;
    ms_bClosePending = true;

    if (bSubmit && CNetwork::m_bAuthenticated)
    {
        CChat::SubmitInput();
    }
}

void CChatGamepad::TypeSelectedKey()
{
    if (ms_nRow < KEYBOARD_ROWS - 1)
    {
        const wchar_t* row = ms_aaKeyboard[ms_nPage][ms_nRow];
        if (ms_nColumn >= (int)wcslen(row))
            return;

        if (CChat::m_sInputText.size() < Config::MAX_CHAT_MESSAGE_LENGTH)
        {
            CChat::m_sInputText.insert(CChat::m_nCaretPos, 1, row[ms_nColumn]);
            CChat::m_nCaretPos++;
        }
        return;
    }

    switch (ms_nColumn / 2)
    {
        case SPECIAL_KEY_SPACE:
            if (CChat::m_sInputText.size() < Config::MAX_CHAT_MESSAGE_LENGTH)
            {
                CChat::m_sInputText.insert(CChat::m_nCaretPos, 1, L' ');
                CChat::m_nCaretPos++;
            }
            break;
        case SPECIAL_KEY_BACKSPACE:
            CChat::EraseCharacter(CChat::m_sInputText, 0);
            break;
        case SPECIAL_KEY_PAGE:
            ms_nPage = (ms_nPage + 1) % NUM_PAGES;
            break;
        case SPECIAL_KEY_SEND:
            Close(true);
            break;
        case SPECIAL_KEY_CLOSE:
            Close(false);
            break;
    }
}

void CChatGamepad::Process()
{
    XINPUT_STATE state;
    if (!ReadGamepad(state))
    {
        if (ms_bActive)
            Close(false);

        if (ms_bClosePending)
        {
            ms_bClosePending = false;
            CChat::ToggleInput(false);
        }
        return;
    }

    WORD buttons = GetButtons(state.Gamepad);
    WORD pressed = buttons & ~ms_wLastButtons;
    unsigned int nTime = GetTickCount();

    // the held d-pad repeats
    const WORD navigation = XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT |
                            XINPUT_GAMEPAD_LEFT_SHOULDER | XINPUT_GAMEPAD_RIGHT_SHOULDER | XINPUT_GAMEPAD_X;
    if (pressed & navigation)
    {
        ms_wRepeatButton = pressed & navigation;
        ms_nRepeatTime = nTime + REPEAT_DELAY;
    }
    else if (ms_wRepeatButton && (buttons & ms_wRepeatButton) == ms_wRepeatButton && nTime >= ms_nRepeatTime)
    {
        pressed |= ms_wRepeatButton;
        ms_nRepeatTime = nTime + REPEAT_INTERVAL;
    }
    else if (!(buttons & ms_wRepeatButton))
    {
        ms_wRepeatButton = 0;
    }

    ms_wLastButtons = buttons;

    if (ms_bClosePending)
    {
        if (buttons == 0)
        {
            ms_bClosePending = false;
            CChat::ToggleInput(false);
        }
        return;
    }

    if (!ms_bActive)
    {
        // Back + Y, the menu and the cutscenes use the other buttons
        if ((buttons & XINPUT_GAMEPAD_BACK) && (pressed & XINPUT_GAMEPAD_Y) && !FrontEndMenuManager.m_bMenuActive)
        {
            Open();
        }
        return;
    }

    // the chat was closed with the keyboard
    if (!CChat::m_bInputActive)
    {
        ms_bActive = false;
        return;
    }

    if (pressed & XINPUT_GAMEPAD_DPAD_UP)
        ms_nRow = (ms_nRow + KEYBOARD_ROWS - 1) % KEYBOARD_ROWS;
    if (pressed & XINPUT_GAMEPAD_DPAD_DOWN)
        ms_nRow = (ms_nRow + 1) % KEYBOARD_ROWS;
    if (pressed & XINPUT_GAMEPAD_DPAD_LEFT)
        ms_nColumn = (ms_nColumn + KEYBOARD_COLUMNS - 1) % KEYBOARD_COLUMNS;
    if (pressed & XINPUT_GAMEPAD_DPAD_RIGHT)
        ms_nColumn = (ms_nColumn + 1) % KEYBOARD_COLUMNS;

    if ((pressed & XINPUT_GAMEPAD_LEFT_SHOULDER) && CChat::m_nCaretPos > 0)
        CChat::MoveCaretDirection(false);
    if ((pressed & XINPUT_GAMEPAD_RIGHT_SHOULDER) && CChat::m_nCaretPos < CChat::m_sInputText.size())
        CChat::MoveCaretDirection(true);

    if (pressed & BUTTON_LEFT_TRIGGER)
        ms_nPage = (ms_nPage + 1) % NUM_PAGES;

    if (pressed & XINPUT_GAMEPAD_A)
        TypeSelectedKey();
    if (pressed & XINPUT_GAMEPAD_X)
        CChat::EraseCharacter(CChat::m_sInputText, 0);
    if ((pressed & XINPUT_GAMEPAD_Y) && CChat::m_sInputText.size() < Config::MAX_CHAT_MESSAGE_LENGTH)
    {
        CChat::m_sInputText.insert(CChat::m_nCaretPos, 1, L' ');
        CChat::m_nCaretPos++;
    }

    if (ms_bActive && (pressed & XINPUT_GAMEPAD_START))
        Close(true);
    else if (ms_bActive && (pressed & XINPUT_GAMEPAD_B))
        Close(false);
}

void CChatGamepad::Draw()
{
    if (!ms_bActive || !CDXFont::m_pD3DXFont)
        return;

    const int keySize = CDXFont::m_fFontSize * 2;
    const int x = 10;
    const int y = RsGlobal.maximumHeight / 5 + 17 * CDXFont::m_fFontSize + CDXFont::m_fFontSize / 2;

    CSprite2d::DrawRect(CRect((float)x, (float)y, (float)(x + keySize * KEYBOARD_COLUMNS), (float)(y + keySize * KEYBOARD_ROWS)),
        CRGBA(0, 0, 0, 160));

    for (int row = 0; row < KEYBOARD_ROWS; row++)
    {
        for (int column = 0; column < KEYBOARD_COLUMNS; column++)
        {
            bool bSpecial = row == KEYBOARD_ROWS - 1;
            if (bSpecial && column % 2 != 0)
                continue;  // the special keys are two keys wide

            int keyX = x + column * keySize;
            int keyY = y + row * keySize;
            int keyWidth = bSpecial ? keySize * 2 : keySize;

            bool bSelected = ms_nRow == row && (bSpecial ? ms_nColumn / 2 == column / 2 : ms_nColumn == column);
            if (bSelected)
            {
                CSprite2d::DrawRect(CRect((float)keyX, (float)keyY, (float)(keyX + keyWidth), (float)(keyY + keySize)),
                    CRGBA(144, 98, 16, 220));
            }

            std::wstring label;
            if (bSpecial)
            {
                label = ms_aSpecialKeys[column / 2];
            }
            else
            {
                const wchar_t* keys = ms_aaKeyboard[ms_nPage][row];
                if (column < (int)wcslen(keys))
                    label = keys[column];
            }

            int textWidth = CDXFont::GetTextWidth(label);
            CDXFont::DrawSegmentWithShadow(keyX + (keyWidth - textWidth) / 2, keyY + (keySize - CDXFont::m_fFontSize) / 2,
                label.c_str(), D3DCOLOR_RGBA(255, 255, 255, 255));
        }
    }

    CDXFont::Draw(x, y + keySize * KEYBOARD_ROWS + 4,
        L"A type  X delete  Y space  LB/RB caret  LT Aa#  START send  B close", D3DCOLOR_RGBA(200, 200, 200, 255));
}
