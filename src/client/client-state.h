// Copyright (c) 2025 Liam Mercier
//
// This file is part of SilentTanks.
//
// SilentTanks is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License Version 3.0
// as published by the Free Software Foundation.
//
// SilentTanks is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License v3.0
// for more details.
//
// You should have received a copy of the GNU Affero General Public License v3.0
// along with SilentTanks. If not, see <https://www.gnu.org/licenses/agpl-3.0.txt>

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
    Replaying,
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
    ClassicThreePlayer,
    ClassicFivePlayer,
    RankedTwoPlayer,
    NO_MODE
};

Q_ENUM_NS(QueueType)

enum class SoundType : uint8_t
{
    NotifyTurn,
    Move,
    CannonFiring,
    OutOfAmmo,
    Reload,
    Rotate
};

Q_ENUM_NS(SoundType)

}

using ClientState = GUI::ClientState;

using PopupType = GUI::PopupType;

using QueueType = GUI::QueueType;

using SoundType = GUI::SoundType;
