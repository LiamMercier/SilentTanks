import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Popup {
    id: userListPopup
    modal: true
    focus: true

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Decide if we should display accept/decline buttons.
    property bool showActionButtons: true

    // Let others set the model
    property alias listModel: userListView.model

    // Set text depending on usage.
    property alias headerText: headerLabel.text

    parent: lobbyRoot

    // Center inside calling window.
    width: parent ? parent.width * 0.3 : 205
    height: parent ? parent.height * 0.7 : 550
    anchors.centerIn: parent

    background: Rectangle {
        color: "white"
        radius: 8
        border.width: 1
    }

    // Empty text fields when closed.
    onClosed: {
        searchInput.text = ""
        searchInput.visible = false
    }

    // Reduce background colors.
    dim: true

    // Start the user list.
    ColumnLayout {
        id: listColumn
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // Title and close button.
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "\u{1F50D}"
                font.pixelSize: 14
                implicitHeight: 35
                implicitWidth: 35

                background: null

                Layout.alignment: Qt.AlignLeft | Qt.AlignBottom

                // Open the search bar for usernames.
                onClicked: {
                    searchInput.visible = !searchInput.visible
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                id: headerLabel
                text: "Users"
                font.pointSize: 14
                font.bold: true
                Layout.alignment: Qt.AlignCenter
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: "X"
                font.pointSize: 10
                implicitHeight: 25
                implicitWidth: 25
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

                onClicked: {
                    userListPopup.close()
                }
            }

        }

        TextField {
            id: searchInput
            placeholderText: "Enter username to " + (showActionButtons ? "add" : "block")
            visible: false
            Layout.fillWidth: true
            focus: visible
            onAccepted: {
                // Send request.
                if (showActionButtons) {
                    Client.friend_user(text)
                }
                else {
                    Client.block_user(text)
                }
                searchInput.text = ""
                searchInput.visible = false
            }
        }

        // Actual user elements (next column item).
        ListView {
            id: userListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignHCenter
            model: null
            clip: true

            delegate: Rectangle {
                width: ListView.view.width
                height: 48
                color: "white"

                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    spacing: 8
                    anchors.rightMargin: 16

                    Label {
                        text: model.username
                        font.family: "Roboto"
                        font.pointSize: 10
                        font.weight: Font.DemiBold
                        padding: 4

                        Layout.alignment: Qt.AlignVCenter
                    }

                    // Create space between the buttons and username.
                    Item {
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        spacing: 8
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        anchors.rightMargin: 8

                        Button {
                            text: "Y"
                            font.pointSize: 6
                            implicitHeight: 20
                            implicitWidth: 20
                            visible: userListPopup.showActionButtons
                            onClicked: {
                                Client.respond_friend_request(model.uuid, true)
                            }
                        }

                        Button {
                            text: "X"
                            font.pointSize: 6
                            implicitHeight: 20
                            implicitWidth: 20
                            onClicked: {
                                if (userListPopup.showActionButtons)
                                {
                                    Client.respond_friend_request(model.uuid, false)
                                }
                                else {
                                    Client.unblock_user(model.uuid)
                                }
                            }
                        }

                    }

                }
            }

        }

    }

}
