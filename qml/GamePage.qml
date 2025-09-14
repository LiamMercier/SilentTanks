import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Item {
    id: gamePageRoot
    anchors.fill: parent

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Map area (80% of screen)
        Rectangle {
            id: mapArea
            Layout.preferredWidth: parent.width * 0.8
            Layout.fillHeight: true
            border.width: 1

            BoardView {
                anchors.fill: parent
            }
        }

        Rectangle {
            id: sidePanel
            Layout.preferredWidth: parent.width * 0.2
            Layout.fillHeight: true
            border.width: 0

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 0

                    Text {
                        text: {
                            switch (GameManager.state) {
                                case 0: return "Setup Phase";
                                case 1: return "Game Phase";
                                case 2: return "Finished";
                                default: return "unknown";
                            }
                        }
                        font.bold: true
                        font.pointSize: 12
                        anchors.centerIn: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                    color: Qt.rgba(0, 0, 0, 1)
                }

                Rectangle {
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 0

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Text {
                            text: GameManager.player + "'s Turn"
                            font.pointSize: 10
                            font.weight: Font.DemiBold

                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        Text {
                            text: "Fuel Remaining: " + GameManager.fuel

                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                    color: Qt.rgba(0, 0, 0, 1)
                }

                Rectangle {
                    Layout.preferredHeight: 26
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 0
                    color: "transparent"

                    Text {
                        text: "Players"
                        font.weight: Font.DemiBold
                        font.pointSize: 12
                        anchors.centerIn: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 1
                    Layout.fillWidth: true
                    color: Qt.rgba(0, 0, 0, 1)
                }

                ListView {
                    id: playerListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: PlayersModel
                    clip: true

                    delegate: Rectangle {
                        width: playerListView.width
                        height: playerListView.height / 5
                        color: "transparent"

                        Text {
                            anchors.fill: parent
                            anchors.margins: 3

                            text: model.username
                            font.family: "Roboto"
                            font.weight: Font.DemiBold
                            color: "black"
                            elide: Text.ElideRight

                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter

                            width: parent.width - 6
                        }

                        Rectangle {
                            width: parent.width
                            height: 1
                            color: Qt.rgba(0, 0, 0, 0.3)
                        }
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 1

                    Button {
                        text: "Forfeit"

                        anchors.centerIn: parent

                        onClicked: {
                            Client.send_forfeit()
                        }
                    }
                }

                Rectangle {
                    id: chatBoxBackground
                    border.width: 1
                    border.color: "green"

                    Layout.preferredHeight: sidePanel.height * 0.4
                    Layout.maximumHeight: sidePanel.height * 0.4
                    Layout.fillWidth: true

                    ChatBox {
                        id: chatBoxRoot
                        anchors.fill: parent
                    }
                }

            }

        }

    }
}
