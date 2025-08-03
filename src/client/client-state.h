#pragma once

enum class ClientState : uint8_t
{
    ConnectScreen,
    LoginScreen,
    Lobby,
    Playing,
    Disconnected
};
