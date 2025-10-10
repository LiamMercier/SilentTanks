import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Item {
    id: gamePageRoot
    anchors.fill: parent

    property double nowMs: Date.now()

    Timer {
        id: timeoutTimer
        interval: 100
        running: true
        repeat: true

        onTriggered: {
            nowMs = Date.now()
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Map area (80% of screen)
        Rectangle {
            id: mapArea
            Layout.preferredWidth: parent.width * 0.8
            Layout.fillHeight: true

            BoardView {
                anchors.fill: parent
            }
        }

        // Divider between map and side side panel
        Rectangle {
            id: panelDivider
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: "#1b1c1d"
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
                    Layout.preferredHeight: 35
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 0

                    color: "#26282a"

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
                        color: "#f2f2f2"
                        anchors.centerIn: parent

                        transform: Translate { y: 3 }

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 0

                    color: "#26282a"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Text {
                            text: GameManager.player + "'s turn"

                            font.pointSize: 10
                            font.weight: Font.DemiBold

                            color: "#f2f2f2"

                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        Text {
                            text: "Fuel Remaining: " + GameManager.fuel

                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            color: "#f2f2f2"

                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 0
                    color: "#202122"

                    Text {
                        text: "Players"
                        font.weight: Font.DemiBold
                        font.pointSize: 12
                        anchors.centerIn: parent

                        color: "#f2f2f2"

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    color: "#202122"

                    ListView {
                        id: playerListView
                        anchors.fill: parent
                        model: PlayersModel
                        clip: true

                        delegate: Rectangle {
                            id: playerDelegate

                            width: playerListView.width
                            height: playerListView.height / 5
                            color: "#323436"

                            property double endTimeMs: Date.now() + model.timer

                            property double remainingMs: {
                                if (!model || typeof model.timer === "undefined"
                                    || typeof GameManager === "undefined")
                                {
                                    return 0
                                }

                                if (model.username != GameManager.player)
                                {
                                    return model.timer
                                }

                                return Math.max(0, endTimeMs - gamePageRoot.nowMs)
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 0

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 0.5 * parent.height
                                    color: "transparent"

                                    Text {
                                        anchors.fill: parent
                                        anchors.margins: 3

                                        text: model.username
                                        font.family: "Roboto"
                                        font.weight: Font.DemiBold
                                        color: "#f2f2f2"
                                        elide: Text.ElideRight

                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter

                                        width: parent.width - 6
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 0.5 * parent.height
                                    color: "transparent"

                                    Text {
                                        anchors.fill: parent
                                        anchors.margins: 3

                                        text: {
                                            var msTotal = playerDelegate.remainingMs

                                            var minutes = Math.floor(msTotal / 60000)
                                            var seconds = Math.floor((msTotal % 60000) / 1000)
                                            var ms = Math.round((msTotal % 1000) / 100)

                                            // Fix rounding errors.
                                            var msStr = (ms == 10) ? 9 : ms

                                            // Append leading 0 if necessary
                                            var secondsStr = seconds < 10 ? "0" + seconds : seconds

                                            return minutes + ":" + secondsStr + "." + msStr
                                        }
                                        font.family: "Roboto"
                                        font.weight: Font.DemiBold
                                        color: "#f2f2f2"
                                        elide: Text.ElideRight

                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter

                                        width: parent.width - 6
                                    }
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: 1
                                color: Qt.rgba(0, 0, 0, 0.3)
                            }

                            Connections {
                                target: GameManager
                                function onTimers_changed() {
                                    endTimeMs = Date.now() + model.timer
                                }
                            }

                            Component.onCompleted: {
                                endTimeMs = Date.now() + model.timer
                            }
                        }
                    }

                }

                Rectangle {
                    Layout.preferredHeight: 50
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    color: "#202122"

                    SvgButton {
                        id: forfeitButton
                        implicitHeight: 40
                        implicitWidth: 80

                        anchors.centerIn: parent

                        buttonText: "Forfeit"
                        fontChoice: "#1f1f1f"
                        unfocusedOpacity: 1.00

                        normalSource: "qrc:/svgs/buttons/forfeit_button.svg"
                        hoverSource: "qrc:/svgs/buttons/forfeit_button_hovered.svg"
                        pressedSource: "qrc:/svgs/buttons/forfeit_button_pressed.svg"

                        toggled: false

                        onClicked: {
                            Client.send_forfeit()
                        }
                    }

                }

                Rectangle {
                    id: chatBoxBackground
                    color: "#2a2c2e"

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
