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

Item
{
    id: svgButton

    property bool toggled: false
    property bool doSmooth: false

    // Button sources to be set by the parent.
    property url normalSource: ""
    property url hoverSource: ""
    property url pressedSource: ""

    property url normalSourceToggled: ""
    property url hoverSourceToggled: ""
    property url pressedSourceToggled: ""

    property string buttonText: ""

    property string fontChoice: "#ffffff"

    property real unfocusedOpacity: 0.87

    signal clicked

    Image {
        id: icon
        anchors.fill: parent
        source: {
            var src = ""
            if (toggled)
            {
                if (svgButtonMouseArea.pressed)
                {
                    src = svgButton.pressedSourceToggled
                }
                else if (svgButtonMouseArea.containsMouse)
                {
                    src = svgButton.hoverSourceToggled
                }
                else
                {
                    src = svgButton.normalSourceToggled
                }
            }
            else
            {
                if (svgButtonMouseArea.pressed)
                {
                    src = svgButton.pressedSource
                }
                else if (svgButtonMouseArea.containsMouse)
                {
                    src = svgButton.hoverSource
                }
                else
                {
                    src = svgButton.normalSource
                }
            }

            return src
        }
        fillMode: Image.PreserveAspectFit
        smooth: doSmooth
    }

    MouseArea {
        id: svgButtonMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: svgButton.clicked()
    }

    Text {
        id: svgButtonText
        text: buttonText
        color: fontChoice
        anchors.centerIn: parent

        opacity: {
            var op = 1.0

            if (!svgButtonMouseArea.containsMouse)
            {
                op = unfocusedOpacity
            }

            return op
        }

        font.family: root.font.family
        font.pixelSize: root.font.pixelSize
        font.bold: root.font.bold
    }

}
