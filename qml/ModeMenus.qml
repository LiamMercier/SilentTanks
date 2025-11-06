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
    id: modeMenuRoot

    property string label: "modes label"
    property var model: null
    property var callback: null

    implicitWidth: triggerButton.implicitWidth
    implicitHeight: triggerButton.implicitHeight

    Button {
        id: triggerButton
        text: modeMenuRoot.label
        font.bold: true

        implicitHeight: 40
        implicitWidth: 140

        onClicked: {
            popup.x = triggerButton.mapToItem(parent, triggerButton.x, triggerButton.y).x
            popup.y = triggerButton.mapToItem(parent, triggerButton.x, triggerButton.height).y
            popup.width = Math.max(triggerButton.width, 120)
            popup.open()
        }

        background: Rectangle {
            color: "#323436"
            radius: 2
            implicitHeight: triggerButton,implicitHeight
            implicitWidth: 140
        }

        contentItem: Text {
            text: triggerButton.text
            font: triggerButton.font
            color: "#eaeaea"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    Popup {
        id: popup
        modal: false
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        leftPadding: 7
        rightPadding: 7

        background: Rectangle {
            color: "#202122"
            height: popup.height - 4
            radius: 4
        }

        ColumnLayout {
            id: mainRepeaterColumn
            anchors.fill: parent
            spacing: 4

            Repeater {
                model: modeMenuRoot.model
                delegate: Button {
                    // list model role
                    id: modeButton
                    text: label
                    Layout.alignment: Qt.AlignCenter
                    font.pointSize: 11

                    Layout.preferredHeight: 40

                    Layout.fillWidth: true

                    background: Rectangle {
                        radius: 2
                        anchors.fill: parent
                        color: "#3e4042"
                    }

                    contentItem: Text {
                        text: modeButton.text
                        font: modeButton.font
                        color: "#f2f2f2"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        if (modeMenuRoot.callback)
                        {
                            modeMenuRoot.callback(mode)
                        }
                        popup.close()
                    }
                }

            }
        }

    }
}
