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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Item {
    id: rootQueueMenu

    // Hold the selected mode, possibly a no-op.
    property int selectedMode: Client.selected_mode

    implicitWidth: contentRow.implicitWidth
    implicitHeight: contentRow.implicitHeight

    // Start of queue buttons, lay them out in
    // a row for the user.
    RowLayout {
        id: contentRow
        spacing: 10
        anchors.centerIn: parent

        ListModel {
            id: casualModel
        }

        ListModel {
            id: rankedModel
        }

        Component.onCompleted:
        {
            casualModel.append({label:"Casual 1v1", mode: QueueType.ClassicTwoPlayer})
            casualModel.append({label:"Casual 3 player", mode: QueueType.ClassicThreePlayer})
            casualModel.append({label:"Casual 5 player", mode: QueueType.ClassicFivePlayer})

            rankedModel.append({label:"Ranked 1v1", mode: QueueType.RankedTwoPlayer})
        }

        ModeMenus {
            label: "Casual Modes"
            model: casualModel
            callback: function (selectedMode) {
                Client.set_selected_mode(selectedMode)
            }
        }

        ModeMenus {
            label: "Ranked Modes"
            model: rankedModel
            callback: function (selectedMode) {
                Client.set_selected_mode(selectedMode)
            }
        }

    }
}
