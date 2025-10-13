import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Item {
    id: replayPageRoot
    anchors.fill: parent

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Map area (80% of screen)
        Rectangle {
            id: mapArea
            Layout.preferredWidth: parent.width * 0.8
            Layout.fillHeight: true

            ReplayView {
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

            color: "#26282a"

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    id: selectorRect
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 0

                    color: "#26282a"

                    ComboBox {
                        id: matchSelector
                        anchors.centerIn: parent
                        model: ReplayManager

                        implicitHeight: 30
                        implicitWidth: selectorRect.width * 0.8

                        textRole: "MatchID"

                        indicator: Text {
                            text: "\u25BC"
                            color: "#f2f2f2"
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 8
                        }

                        contentItem: Label {
                            anchors.fill: parent
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter

                            text: {
                                ReplayManager.replay_id >= 0 ? "Match "
                                    + ReplayManager.replay_id : "Select Match"
                            }

                            color: "#f2f2f2"
                        }

                        background: Rectangle {
                            implicitWidth: matchSelector.implicitWidth
                            implicitHeight: matchSelector.implicitHeight
                            radius: 4
                            color: "#323436"
                            border.width: 0
                        }

                        popup: Popup {
                            id: matchSelectorPopup

                            y: matchSelector.height
                            width: matchSelector.width
                            implicitHeight: matchComboListView.contentHeight
                            padding: 1
                            background: Rectangle {
                                color: "#323436"
                                radius: 4
                                border.width: 0
                            }

                            contentItem: ListView {
                                id: matchComboListView
                                anchors.fill: parent
                                model: matchSelector.model

                                delegate: ItemDelegate {
                                    id: dropdownDelegate
                                    width: matchSelector.width

                                    contentItem: Text {
                                        anchors.centerIn: parent
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter

                                        text: "Match " + model.MatchID
                                        color: "#f2f2f2"
                                    }

                                    background: Rectangle {
                                        color: "#323436"
                                        radius: 4
                                    }

                                    onClicked: {
                                        ReplayManager.set_replay(model.MatchID)
                                        matchSelectorPopup.close()
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 30
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                    border.width: 0

                    color: "#26282a"

                    Text {
                        text: {
                            switch (ReplayManager.state) {
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

                        transform: Translate { y: -3 }

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
                            text: ReplayManager.player + "'s Turn"
                            font.pointSize: 10
                            font.weight: Font.DemiBold

                            color: "#f2f2f2"

                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight

                            transform: Translate { y: -6 }
                        }

                        Text {
                            text: {
                                var txt = ""
                                if (ReplayManager.state != 0)
                                {
                                    txt = "Fuel Remaining: " + ReplayManager.fuel
                                }
                                return txt
                            }

                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            color: "#f2f2f2"

                            transform: Translate { y: -6 }

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
                        model: ReplayPlayers
                        clip: true

                        delegate: Rectangle {
                            id: playerDelegate

                            width: playerListView.width
                            height: playerListView.height / 5
                            color: "#323436"

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
                            }

                            Rectangle {
                                width: parent.width
                                height: 1
                                color: Qt.rgba(0, 0, 0, 0.3)
                            }
                        }
                    }

                }

                Rectangle {
                    Layout.preferredHeight: 50
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                    color: "#202122"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        SvgButton {
                            id: previousMoveButton

                            implicitHeight: 32
                            implicitWidth: 32

                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            normalSource: "qrc:/pngs/previous_button.png"
                            hoverSource: "qrc:/pngs/previous_button_hovered.png"
                            pressedSource: "qrc:/pngs/previous_button_pressed.png"

                            toggled: false

                            onClicked: {
                                ReplayManager.step_backward_turn()
                            }
                        }

                        SvgButton {
                            id: forfeitButton
                            implicitHeight: 40
                            implicitWidth: 80

                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            buttonText: "Close"
                            fontChoice: "#1f1f1f"
                            unfocusedOpacity: 1.00

                            normalSource: "qrc:/svgs/buttons/forfeit_button.svg"
                            hoverSource: "qrc:/svgs/buttons/forfeit_button_hovered.svg"
                            pressedSource: "qrc:/svgs/buttons/forfeit_button_pressed.svg"

                            toggled: false

                            onClicked: {
                                Client.close_replay()
                            }
                        }

                        SvgButton {
                            id: nextMoveButton

                            implicitHeight: 32
                            implicitWidth: 32

                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                            normalSource: "qrc:/pngs/next_button.png"
                            hoverSource: "qrc:/pngs/next_button_hovered.png"
                            pressedSource: "qrc:/pngs/next_button_pressed.png"

                            toggled: false

                            onClicked: {
                                ReplayManager.step_forward_turn()
                            }
                        }
                    }

                }

                Rectangle {
                    id: perspectiveAreaBackground
                    color: "#2a2c2e"

                    Layout.preferredHeight: sidePanel.height * 0.4
                    Layout.maximumHeight: sidePanel.height * 0.4
                    Layout.fillWidth: true

                    ComboBox {
                        id: perspectiveSelector
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 4

                        model: ReplayManager.player_count + 1

                        implicitHeight: 30
                        implicitWidth: selectorRect.width * 0.8

                        indicator: Text {
                            text: "\u25BC"
                            color: "#f2f2f2"
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.rightMargin: 8
                        }

                        contentItem: Label {
                            anchors.fill: parent
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter

                            // UINT8_MAX === 255 and represents the global perspective.
                            text: {
                                ReplayManager.perspective !== 255 ? "Player "
                                    + ReplayManager.perspective : "Global Perspective"
                            }

                            color: "#f2f2f2"
                        }

                        background: Rectangle {
                            implicitWidth: perspectiveSelector.implicitWidth
                            implicitHeight: perspectiveSelector.implicitHeight
                            radius: 4
                            color: "#323436"
                            border.width: 0
                        }

                        popup: Popup {
                            id: perspectiveSelectorPopup

                            y: perspectiveSelector.height
                            width: perspectiveSelector.width
                            implicitHeight: perspectiveComboListView.contentHeight
                            padding: 1
                            background: Rectangle {
                                color: "#323436"
                                radius: 4
                                border.width: 0
                            }

                            contentItem: ListView {
                                id: perspectiveComboListView
                                anchors.fill: parent
                                model: perspectiveSelector.model

                                delegate: ItemDelegate {
                                    id: perspectiveDropdownDelegate
                                    width: perspectiveSelector.width

                                    contentItem: Text {
                                        anchors.centerIn: parent
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter

                                        text: (index !== ReplayManager.player_count) ?
                                              "Player " + index : "Global Perspective"
                                        color: "#f2f2f2"
                                    }

                                    background: Rectangle {
                                        color: "#323436"
                                        radius: 4
                                    }

                                    onClicked: {
                                        ReplayManager.set_perspective(index)
                                        perspectiveSelectorPopup.close()
                                    }
                                }
                            }
                        }
                    }


                }

            }

        }

    }
}
