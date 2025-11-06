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

// Queue selection area
Rectangle {
    id: queueArea
    border.width: 0
    color: "#323436"

    Layout.fillWidth: true
    // Give queues area 60% of height
    Layout.preferredHeight: lobbyBackground.height * 0.6

    ColumnLayout {

        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: topQueueBar
            Layout.preferredHeight: 56
            Layout.fillWidth: true
            clip: true

            color: "#202122"

            RowLayout {
                anchors.fill: parent
                Layout.fillWidth: true
                Layout.fillHeight: true

                spacing: 8

                Item {
                    Layout.fillWidth: true
                }

                QueueMenus {
                    id: queueMenus
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

                    onSelectedModeChanged:
                    {
                        print("Current selected mode:",  queueMenus.selectedMode)
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "transparent"
        }
    }
}
