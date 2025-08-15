import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

// Queue selection area
Rectangle {
    id: queueArea
    border.color: "purple"
    border.width: 1
    opacity: 0.8

    Layout.fillWidth: true
    // Give queues area 60% of height
    Layout.preferredHeight: lobbyBackground.height * 0.6

    ColumnLayout {

        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: topQueueBar
            Layout.preferredHeight: 56
            Layout.fillWidth: true
            border.width: 1
            clip: true

            RowLayout {
                anchors.fill: parent
                Layout.fillWidth: true
                Layout.fillHeight: true

                spacing: 8

                Item {
                    Layout.fillWidth: true
                }

                QueueMenus {
                    id: queueMenus
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter

                    onSelectedModeChanged:
                    {
                        console.log("Selected mode: ",  queueMenus.selectedMode)
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "transparent"
            Text {
                text: "Queue selection"
                anchors.centerIn: parent
            }
        }
    }
}
