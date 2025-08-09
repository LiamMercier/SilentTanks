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

enum class QueueType : uint8_t
{
    ClassicTwoPlayer = 0,
    RankedTwoPlayer,
    NO_MODE
};

Q_ENUM_NS(QueueType)

}

using ClientState = GUI::ClientState;

using PopupType = GUI::PopupType;

using QueueType = GUI::QueueType;
