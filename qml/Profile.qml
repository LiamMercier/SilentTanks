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
            border.color: "black"
            border.width: 1

            Layout.fillHeight: true
            Layout.fillWidth: true

            Text {
                text: "Match History"
                anchors.centerIn: parent
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 4

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
                            spacing: 8

                            Text {
                                text: model.MatchID
                            }

                            Text {
                                text: model.finished_at
                            }

                            Text {
                                text: model.placement
                            }

                            Text {
                                text: model.elo_change
                            }

                        }
                    }
                }
            }
        }

    }

}
