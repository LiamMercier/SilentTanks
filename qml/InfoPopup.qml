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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import GUICommon 1.0

Window
{
    id: infoPopup

    width: 350
    height: 175

    // Prevent resizing.
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height

    // Stop the main application until response occurs.
    // modality: Qt.ApplicationModal
    modality: Qt.WindowModal

    // Flags to setup a popup style environment.
    flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowSystemMenuHint

    // Alias for other files.
    property alias popupTitle: titleText.text
    property alias popupText: bodyText.text

    // Tell our popup queue in C++ that we closed a popup.
    onClosing: {
        Client.notify_popup_closed()
    }

    Rectangle {
        anchors.fill: parent
        color: "#202122"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Label {
                id: titleText
                text: ""
                font.bold: true
                color: "#ffffff"

                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap

                Layout.fillWidth: true
                palette: infoPopup.palette
            }

            Label {
                id: bodyText
                text: ""
                color: "#f2f2f2"

                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap

                Layout.fillWidth: true
                palette: infoPopup.palette
            }

            Button {
                id: popupOkButton
                text: qsTr("Ok")
                Layout.alignment: Qt.AlignHCenter
                onClicked: infoPopup.close()

                background: Rectangle {
                    implicitWidth: 80
                    implicitHeight: 32
                    radius: 4
                    color: popupOkButton.down ? "#1b1c1d" :
                            popupOkButton.hovered ? "#3e4042" : "#323436"
                }

                contentItem: Text {
                    text: popupOkButton.text
                    color: "#f2f2f2"

                    horizontalAlignment: Text.AlignHCenter

                    anchors.centerIn: parent
                }
            }
        }
    }
}
