#pragma once

// the chat for the gamepad players: Back + Y opens it with an on-screen keyboard
class CChatGamepad
{
public:
    static void Process();
    static void Draw();

private:
    static void Open();
    static void Close(bool bSubmit);
    static void TypeSelectedKey();
};
