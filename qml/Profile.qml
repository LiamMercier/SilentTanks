import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

Item {
    id: profileRoot
    Layout.fillWidth: true
    Layout.fillHeight: true

    // Elo's on the left and match history on the right.
    RowLayout {
        id: profileContent

        anchors.fill: parent
        spacing: 4

        Rectangle {
            Layout.preferredHeight: profileRoot.height
            Layout.preferredWidth: profileRoot.width * 0.3
            border.color: "red"
            border.width: 1

            // Area for mode selection and elo display.
            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                ListModel {
                    id: allModes
                }

                Component.onCompleted:
                {
                    allModes.append({label:"Casual 1v1", mode: QueueType.ClassicTwoPlayer})
                    allModes.append({label:"Ranked 1v1", mode: QueueType.RankedTwoPlayer})
                }

                ModeMenus {
                    id: profileModeMenu
                    label: "Modes"
                    model: allModes
                    callback: function (selectedMode) {
                        // Set to this mode's name.
                        var modeData = allModes.get(selectedMode)
                        modeText.text = modeData.label

                        // Fetch elo for display.
                        var v = Client.get_elo(modeData.mode)

                        if (v === null || v === undefined)
                        {
                            eloText.visible = false
                        }
                        else
                        {
                            eloText.text = v
                            eloText.visible = true
                        }

                        HistoryModel.set_current_history_mode(modeData.mode)

                        Client.fetch_match_history(modeData.mode)
                    }

                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 8
                }

                Text {
                    id: modeText
                    font.pointSize: 14
                    text: ""
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    id: eloText
                    font.pointSize: 14
                    text: ""
                    Layout.alignment: Qt.AlignHCenter
                }

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle {
            id: matchHistoryRoot
            border.color: "black"
            border.width: 1

            Layout.fillHeight: true
            Layout.fillWidth: true

            // Width values to keep header and items in same width.
            property int matchIDWidth: 70
            property int timeWidth: 130
            property int placementWidth: 80
            property int eloWidth: 70
            property int actionRowWidth: 120

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Text {
                    text: "Match History"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                }

                // Header row.
                Rectangle {
                    border.color: "black"
                    border.width: 1
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 0
                        spacing: 0

                        Rectangle {
                            Layout.preferredWidth: matchHistoryRoot.matchIDWidth
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                            border.width: 1

                            Text {
                                text: "Match ID"
                                font.bold: true
                                anchors.centerIn: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: matchHistoryRoot.timeWidth
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                            border.width: 1

                            Text {
                                text: "Finished Time"
                                font.bold: true
                                anchors.centerIn: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: matchHistoryRoot.placementWidth
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                            border.width: 1

                            Text {
                                text: "Placement"
                                font.bold: true
                                anchors.centerIn: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: matchHistoryRoot.eloWidth
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                            border.width: 1

                            Text {
                                text: "Elo\nChange"
                                font.bold: true
                                anchors.centerIn: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: matchHistoryRoot.actionRowWidth
                            Layout.fillHeight: true
                            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                            border.width: 1

                            Text {
                                text: "Actions"
                                font.bold: true
                                anchors.centerIn: parent
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                    }
                }

                ListView {
                    id: historyList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: HistoryModel
                    clip: true

                    delegate: Rectangle {
                        height: 40
                        width:  historyList.width
                        border.color: "purple"
                        border.width: 1

                        // [ID | Placement | Elo diff | Time | Download Button]
                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            Rectangle {
                                Layout.preferredWidth: matchHistoryRoot.matchIDWidth
                                Layout.fillHeight: true
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                border.width: 1

                                Text {
                                    text: model.MatchID
                                    anchors.centerIn: parent
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: matchHistoryRoot.timeWidth
                                Layout.fillHeight: true
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                border.width: 1

                                Text {
                                    text: model.finished_at
                                    anchors.centerIn: parent
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: matchHistoryRoot.placementWidth
                                Layout.fillHeight: true
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                border.width: 1

                                Text {
                                    text: model.placement
                                    anchors.centerIn: parent
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: matchHistoryRoot.eloWidth
                                Layout.fillHeight: true
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                border.width: 1

                                Text {
                                    text: model.elo_change
                                    anchors.centerIn: parent
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: matchHistoryRoot.actionRowWidth
                                Layout.fillHeight: true
                                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                                border.width: 1

                                Button {
                                    icon.name: "download"
                                    anchors.centerIn: parent
                                    onClicked: {
                                        console.log("trying to download match id", model.MatchID)
                                        Client.download_match_by_id(model.MatchID)
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
