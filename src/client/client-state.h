#pragma once

#include <QObject>

namespace GUI {

Q_NAMESPACE

enum class ClientState : uint8_t
{
    ConnectScreen,
    LoginScreen,
    Lobby,
    Playing,
    Disconnected
};

Q_ENUM_NS(ClientState)

enum class PopupType : uint8_t
{
    Info
};

Q_ENUM_NS(PopupType)

}

using ClientState = GUI::ClientState;

using PopupType = GUI::PopupType;
