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
                border.color: "red"
                border.width: 1

                Text {
                    text: "Background"
                    anchors.centerIn: parent
                }
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
                border.color: "green"
                border.width: 1

                Layout.preferredWidth: lobbyBackground.width * 0.5
                Layout.fillHeight: true

                Layout.margins: 6
                Layout.rightMargin: 3
                Layout.topMargin: 0

                opacity: 0.8

                Text {
                    anchors.centerIn: parent
                    text: "Chat area"
                }

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
                border.color: "blue"
                border.width: 1

                Layout.fillWidth: true
                Layout.fillHeight: true

                Layout.margins: 6
                Layout.leftMargin: 3
                Layout.topMargin: 0

                opacity: 0.8

                Text {
                    anchors.centerIn: parent
                    text: "Queue status"
                }

                Button {
                        text: Client.queued_mode === QueueType.NO_MODE ? "Queue for match" : "Cancel queue"
                        font.pointSize: 16
                        implicitHeight: parent.height * 0.175
                        anchors.horizontalCenter: parent.horizontalCenter

                        y: parent.height * 0.70

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

            // Profile header.
            Rectangle {
                id: userHeader
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                border.color: "yellow"
                border.width: 1

                Column {
                    anchors.fill: parent
                    spacing: 6
                    anchors.topMargin: 8

                    Text {
                        id: usernameText
                        text: Client.username !== "" ? Client.username : "Not signed in"
                        horizontalAlignment: Text.AlignHCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Button {
                        text: "Profile"
                        implicitHeight: 20
                        anchors.horizontalCenter: parent.horizontalCenter

                        property bool showingProfile: false

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

            // Friends list on the right side
            FriendsList {
                Layout.fillHeight: true
                Layout.fillWidth: true
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
