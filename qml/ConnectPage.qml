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

Rectangle {
    width: parent.width
    height: parent.height

    color: "#1b1c1d"

    ServerDetailsPopup {
        id: detailsPopup
        anchors.centerIn: parent
    }

    ColumnLayout {
        id: connectColumn

        property bool addButtonVisible: false

        anchors.centerIn: parent
        width: parent.width * 0.65
        height: parent.height
        spacing: 8

        Item {
            id: connectTopSpacer

            Layout.preferredHeight: connectColumn.height * 0.02
            Layout.maximumHeight: connectColumn.height * 0.02
        }

        Text {
            Layout.alignment: Qt.AlignHCenter

            text: "Servers"
            font.pointSize: 20
            color: "#f2f2f2"
        }

        Rectangle {
            Layout.preferredWidth: connectColumn.width * 0.65
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter

            color: "#202122"

            ListView {
                anchors.fill: parent

                id: serverListView
                model: ServerListModel
                clip: true

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 55

                    color: "#2a2c2e"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6

                        Item {
                            id: serverLeftPadding
                            Layout.preferredWidth: 4
                            Layout.maximumWidth: 4
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Text {
                                id: serverName
                                text: model.name
                                font.pointSize: 12

                                color: "#f2f2f2"
                                elide: Text.ElideRight

                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter

                                width: parent.width - 12
                            }

                            Text {
                                id: serverAddress
                                text: model.address
                                font.pointSize: 10

                                color: "#f2f2f2"
                                elide: Text.ElideRight

                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter

                                width: parent.width - 12
                            }
                        }

                        Item {
                            id: serverButtonSpacer
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }

                        SvgButton {
                            Layout.preferredWidth: 32
                            Layout.maximumWidth: 32

                            Layout.preferredHeight: 32
                            Layout.maximumHeight: 32

                            Layout.alignment: Qt.AlignHCenter

                            fontChoice: "#1f1f1f"
                            unfocusedOpacity: 1.00

                            normalSource: "qrc:/pngs/connect_button.png"
                            hoverSource: "qrc:/pngs/connect_button_hovered.png"
                            pressedSource: "qrc:/pngs/connect_button_pressed.png"

                            toggled: false

                            onClicked: {
                                Client.connect_to_server(model.address,
                                                         model.port,
                                                         model.fingerprint)
                            }
                        }

                        SvgButton {
                            id: serverDetailsButton

                            Layout.preferredWidth: 20
                            Layout.maximumWidth: 20

                            Layout.preferredHeight: 35
                            Layout.maximumHeight: 35

                            Layout.alignment: Qt.AlignHCenter

                            fontChoice: "#1f1f1f"
                            unfocusedOpacity: 1.00

                            normalSource: "qrc:/pngs/vertical_dots.png"
                            hoverSource: "qrc:/pngs/vertical_dots_hovered.png"
                            pressedSource: "qrc:/pngs/vertical_dots_pressed.png"

                            toggled: false

                            onClicked: {
                                detailsPopup.serverName = model.name
                                detailsPopup.serverAddress = model.address
                                detailsPopup.serverPort = model.port
                                detailsPopup.serverFingerprint = model.fingerprint
                                detailsPopup.open()
                            }
                        }

                        Item {
                            id: serverRightPadding
                            Layout.preferredWidth: 4
                            Layout.maximumWidth: 4
                        }
                    }
                }
            }
        }

        Button {
            id: addButton

            visible: !connectColumn.addButtonVisible

            Layout.alignment: Qt.AlignHCenter

            text: "Add Server"

            implicitHeight: 34
            implicitWidth: 84

            contentItem: Text {
                text: addButton.text
                font: addButton.font
                color: "#eaeaea"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: addButton.down ? "#202122" :
                            addButton.hovered ? "#3e4042" : "#323436"
                radius: 2
            }

            onClicked: {
                connectColumn.addButtonVisible = true
                return
            }
        }

        TextField {
            id: nameField

            Layout.alignment: Qt.AlignHCenter

            visible: connectColumn.addButtonVisible

            placeholderText: "Server Name"
            placeholderTextColor: "#a2a3a4"
            color: "#f2f2f2"

            // Above two buttons plus spacing
            Layout.preferredWidth: 362
            Layout.maximumWidth: 362

            background: Rectangle {
                color: "#2a2c2e"
                radius: 2
            }

            onAccepted: {

            }
        }

        RowLayout {
            id: dnsConnectRow
            visible: connectColumn.addButtonVisible

            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            TextField {
                id: addressField

                Layout.alignment: Qt.AlignHCenter

                placeholderText: "Server Domain"
                placeholderTextColor: "#a2a3a4"
                color: "#f2f2f2"

                Layout.preferredWidth: 175
                Layout.maximumWidth: 175

                background: Rectangle {
                    color: "#2a2c2e"
                    radius: 2
                }

                onAccepted: {

                }
            }

            TextField {
                id: portField

                Layout.alignment: Qt.AlignHCenter

                placeholderText: "Port (empty for default)"
                placeholderTextColor: "#a2a3a4"
                color: "#f2f2f2"

                Layout.preferredWidth: 175
                Layout.maximumWidth: 175

                background: Rectangle {
                    color: "#2a2c2e"
                    radius: 2
                }

                onAccepted: {

                }
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter

            visible: connectColumn.addButtonVisible

            text: "or"
            font.pointSize: 16
            color: "#f2f2f2"
        }

        TextField {
            id: identityField

            Layout.alignment: Qt.AlignHCenter

            visible: connectColumn.addButtonVisible

            placeholderText: "Server Identity String"
            placeholderTextColor: "#a2a3a4"
            color: "#f2f2f2"

            // Above two buttons plus spacing
            Layout.preferredWidth: 362
            Layout.maximumWidth: 362

            background: Rectangle {
                color: "#2a2c2e"
                radius: 2
            }

            onAccepted: {

            }
        }

        RowLayout {
            id: acceptServerRow

            visible: connectColumn.addButtonVisible

            Layout.alignment: Qt.AlignHCenter

            spacing: 20

            Button {
                id: saveButton

                Layout.alignment: Qt.AlignHCenter

                text: "Save"

                implicitHeight: 34
                implicitWidth: 80

                contentItem: Text {
                    text: saveButton.text
                    font: saveButton.font
                    color: "#eaeaea"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: saveButton.down ? "#202122" :
                            saveButton.hovered ? "#3e4042" : "#323436"
                    radius: 2
                }

                onClicked: {
                    if (connectColumn.addButtonVisible)
                        // If done, try to save the server details.

                        // Save using the address and port if no identity
                        // was given.
                        if (identityField.text == "")
                        {
                            // If no address, stop now.
                            if (addressField.text == "")
                            {
                                // Clean up for next time.
                                nameField.text = ""
                                addressField.text = ""
                                portField.text = ""
                                identityField.text = ""
                                connectColumn.addButtonVisible = false
                                return
                            }

                            // Set default port if none provided
                            var port = portField.text

                            if (portField.text == "")
                            {
                                port = "49656"
                            }

                            var res = addressField.text + ":" + port
                            Client.save_server_domain(res,
                                                      nameField.text)
                        }
                        // Otherwise try to save using the identity string.
                        else
                        {
                            if (nameField.text == "")
                            {
                                Client.save_server_identity(identityField.text)
                            }
                            else
                            {
                                var res = "{"
                                          + nameField.text
                                          + "}:"
                                          + identityField.text

                                Client.save_server_identity(res)
                            }
                        }

                        // Clean up for next time.
                        nameField.text = ""
                        addressField.text = ""
                        portField.text = ""
                        identityField.text = ""
                        connectColumn.addButtonVisible = false
                        return
                }
            }

            Button {
                id: cancelButton

                Layout.alignment: Qt.AlignHCenter

                text: "Cancel"

                implicitHeight: 34
                implicitWidth: 80

                contentItem: Text {
                    text: cancelButton.text
                    font: cancelButton.font
                    color: "#eaeaea"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: cancelButton.down ? "#202122" :
                            cancelButton.hovered ? "#3e4042" : "#323436"
                    radius: 2
                }

                onClicked: {
                    // Close and clean up for next time.
                    addressField.text = ""
                    portField.text = ""
                    identityField.text = ""
                    connectColumn.addButtonVisible = false
                    return
                }
            }
        }

        Item
        {
            id: connectBottomSpacer

            Layout.preferredHeight: connectColumn.height * 0.05
            Layout.maximumHeight: connectColumn.height * 0.05
        }
    }
}
