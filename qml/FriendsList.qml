import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

Item {
    id: friendsListRoot
    width: parent.width
    height: parent.height

    // Friend requests popup.
    UserListPopup {
        id: friendRequestsPopup
        headerText: "Requests"
        listModel: RequestsModel
    }

    // Blocked users popup.
    UserListPopup {
        id: blockedUsersPopup
        headerText: "Blocked"
        listModel: BlockedModel
        showActionButtons: false
    }

    signal messageFriend(string username)

    FriendMenu {
        id: friendMenu
        onMessageFriend: function(username) {
            friendsListRoot.messageFriend(username)
        }
    }

    ColumnLayout {
        anchors.fill: parent

        // Friend requests and blocked users buttons.
        Item
        {
            id: friendsLabelRow
            Layout.fillWidth: true
            implicitHeight: Math.max(friendsLabel.implicitHeight, buttonsRow.implicitHeight)

            GridLayout {
                anchors.fill: parent
                columns: 3
                columnSpacing: 0

            Item {
                id: leftSpacer
                // Button row width plus right margin.
                Layout.preferredWidth: buttonsRow.implicitWidth + 8
                Layout.fillHeight: true
            }

            Label {
                id: friendsLabel
                text: "Friends"
                font.pointSize: 12
                font.bold: true

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Layout.alignment: Qt.AlignCenter
                Layout.fillWidth: true
                padding: 4
            }

            // Friend request and blocks related buttons.
            Row
            {
                id: buttonsRow
                spacing: 4

                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

                Layout.rightMargin: 8

                Button {
                    text: "+"
                    font.pointSize: 8
                    implicitHeight: 20
                    implicitWidth: 20
                    onClicked: {
                        friendRequestsPopup.open()
                    }
                }

                Button {
                    text: "X"
                    font.pointSize: 6
                    implicitHeight: 20
                    implicitWidth: 20
                    onClicked: {
                        blockedUsersPopup.open()
                    }
                }
            }
            }

        }

        ListView {
            id: friendsListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: FriendsModel
            clip: true

            delegate: Rectangle {
                width: ListView.view.width
                height: friendsListView.height / 10

                // Give some variety to the elements.
                //
                // TODO: replace this with a graphic.
                color: index % 2 === 0 ? "#f9f9f9" : "#ffffff"

                Text {
                    anchors.fill: parent
                    anchors.margins: 6

                    text: model.username
                    font.family: "Roboto"
                    font.pointSize: 10
                    font.weight: Font.DemiBold
                    color: "#333333"
                    elide: Text.ElideRight

                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter

                    width: parent.width - 12
                }

                MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.RightButton

                        // Center the friend menu and open it.
                        onPressed: function(mouse) {
                            var center = mouseArea.mapToItem(friendsListRoot,
                                                             mouseArea.width / 2,
                                                             mouseArea.height / 2)

                            // Push to the left of the area.
                            var targetX = -friendMenu.implicitWidth
                            // Center vertically against the friend element.
                            var targetY = center.y - friendMenu.implicitHeight / 2

                            friendMenu.friendUsername = model.username
                            friendMenu.friendID = model.uuid

                            friendMenu.x = targetX
                            friendMenu.y = targetY

                            friendMenu.open()
                        }

                    }

                Rectangle {

                    width: parent.width
                    height: 1
                    color: Qt.rgba(0, 0, 0, 0.3)
                }
            }
        }
    }
}
