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
            border.color: "purple"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent

                Rectangle {
                    Layout.preferredHeight: 60
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 1

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
                        anchors.centerIn: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 1

                    Text {
                        text: GameManager.player + "'s Turn"
                        anchors.centerIn: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 20
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 1

                    Text {
                        text: "Fuel Remaining: " + GameManager.fuel
                        anchors.centerIn: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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
