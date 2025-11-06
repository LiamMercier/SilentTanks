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

Popup {
    id: popup
    modal: false
    focus:true
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    implicitWidth: 180

    property string friendUsername: ""
    property string friendID: ""

    signal messageFriend(string username)

    background: Rectangle {
        color: "#202122"
        radius: 4
    }

    ColumnLayout {
        id: menuColumn
        spacing: 4
        anchors.fill: parent
        anchors.margins: 4

        Button {
            id: messageButton
            text: "Message"
            Layout.fillWidth: true

            implicitHeight: 34
            implicitWidth: 148

            background: Rectangle {
                radius: 2
                anchors.fill: parent
                color: "#3e4042"
            }

            contentItem: Text {
                text: messageButton.text
                font: messageButton.font
                color: "#f2f2f2"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                messageFriend(friendUsername)
                popup.close()
            }
        }

        Button {
            id: unfriendButton
            text: "Remove"
            Layout.fillWidth: true

            implicitHeight: 34
            implicitWidth: 148

            background: Rectangle {
                radius: 2
                anchors.fill: parent
                color: "#3e4042"
            }

            contentItem: Text {
                text: unfriendButton.text
                font: unfriendButton.font
                color: "#f2f2f2"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                Client.unfriend_user(friendID)
                popup.close()
            }
        }


    }
}
