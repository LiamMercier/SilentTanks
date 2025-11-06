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
    id: lobbyRoot

    // Divide the lobby screen into multiple sections.
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Main lobby area
        Item {
            id: mainLobby
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Background for main lobby. Takes up the entire
            // remaining space except for the friends list.
            Rectangle {
                id: lobbyBackground
                anchors.fill: parent
                color: "#1b1c1d"
                border.width: 0
            }

            // Layout for queue's on top and then the bottom UI row
            // will hold the chat and queue status.
            ColumnLayout
            {
            anchors.fill: parent
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Queue area / profile area
            Loader {
                id: focusAreaLoader
                Layout.fillWidth: true
                Layout.preferredHeight: lobbyBackground.height * 0.6
                Layout.margins: 6
                source: "QueueArea.qml"
            }

            // Bottom UI row.
            RowLayout
            {
                id: bottomBar
                z: 1
                spacing: 0

                // Allocate remaining 40% of height
                Layout.fillWidth: true
                Layout.preferredHeight: lobbyBackground.height * 0.4

            // Chat area.
            Rectangle {
                id: chatArea
                color: "#2a2c2e"

                Layout.preferredWidth: lobbyBackground.width * 0.5
                Layout.fillHeight: true

                Layout.margins: 6
                Layout.rightMargin: 3
                Layout.topMargin: 0

                ChatBox {
                    id: chatBoxRoot
                    anchors.fill: parent
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

            }

            // Queue status area.
            Rectangle {
                id: queueStatusArea
                color: "#2a2c2e"

                Layout.fillWidth: true
                Layout.fillHeight: true

                Layout.margins: 6
                Layout.leftMargin: 3
                Layout.topMargin: 0

                SvgButton {
                    toggled: !(Client.queued_mode === QueueType.NO_MODE)

                    normalSource: "qrc:/svgs/buttons/queue_button.svg"
                    hoverSource: "qrc:/svgs/buttons/queue_button_hovered.svg"
                    pressedSource: "qrc:/svgs/buttons/queue_button_pressed.svg"

                    normalSourceToggled: "qrc:/svgs/buttons/cancel_button.svg"
                    hoverSourceToggled: "qrc:/svgs/buttons/cancel_button_hovered.svg"
                    pressedSourceToggled: "qrc:/svgs/buttons/cancel_button_pressed.svg"

                    height: parent.height * 0.25
                    width: parent.width * 0.8

                    anchors.horizontalCenter: parent.horizontalCenter
                    y: parent.height * 0.70

                    doSmooth: true

                    onClicked: {
                        Client.toggle_queue()
                    }
                }

            }

            }
            }
        }

        // Right UI bar for profile and friends.
        ColumnLayout{
            id: rightBar
            Layout.preferredWidth: 0.2 * lobbyRoot.width
            Layout.maximumWidth: 0.2 * lobbyRoot.width
            Layout.fillHeight: true

            spacing: 0

            // Profile header.
            Rectangle {
                id: userHeader
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: "#2a2c2e"
                border.width: 0

                Column {
                    anchors.fill: parent
                    spacing: 6
                    anchors.topMargin: 8

                    Text {
                        id: usernameText
                        text: Client.username !== "" ? Client.username : "Not signed in"
                        color: "#f2f2f2"
                        horizontalAlignment: Text.AlignHCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    SvgButton {
                        id: profileButton
                        implicitHeight: 20
                        implicitWidth: 80
                        anchors.horizontalCenter: parent.horizontalCenter

                        property bool showingProfile: false

                        buttonText: "Profile"

                        normalSource: "qrc:/svgs/buttons/profile_button.svg"
                        hoverSource: "qrc:/svgs/buttons/profile_button_hovered.svg"
                        pressedSource: "qrc:/svgs/buttons/profile_button_pressed.svg"

                        toggled: false

                        onClicked: {
                            if (showingProfile)
                            {
                                focusAreaLoader.source = "QueueArea.qml"
                                showingProfile = false
                            }
                            else {
                                focusAreaLoader.source = "Profile.qml"
                                showingProfile = true
                            }

                        }
                    }

                }
            }

            Rectangle {
                color: "#202122"
                Layout.fillWidth: true
                Layout.minimumHeight: 8
                Layout.preferredHeight: 8
            }

            // Friends list on the right side
            Rectangle {
                id: friendsListRect
                Layout.fillHeight: true
                Layout.fillWidth: true
                color: "#202122"
                border.width: 0

                FriendsList {
                    // Layout.fillHeight: true
                    // Layout.fillWidth: true
                    anchors.fill: parent

                    onMessageFriend: function(username) {
                        let input = chatBoxRoot.messageInput
                        input.text = "/msg " + username + " "
                        // Set current position of cursor to the end instead of start.
                        input.cursorPosition = input.length
                        input.forceActiveFocus()
                    }
                }
            }
        }
    }

}
