import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: friendsListRoot
    width: parent.width
    height: parent.height

    ColumnLayout {
        anchors.fill: parent

        Label {
            text: "Friends"
            font.pointSize: 12
            font.bold: true

            Layout.alignment: Qt.AlignHCenter
            padding: 4
        }

        ListView {
            id: friendsListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: FriendsModel
            clip: true

            delegate: Rectangle {
                width: parent.width
                height: friendsListView.height / 10

                // Give some variety to the elements.
                color: index % 2 === 0 ? "#f9f9f9" : "#ffffff"

                //border.color: Qt.rgba(0, 0, 0, 0.3)
                //border.width: 1

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

                    ToolTip.visible: mouseArea.containsMouse
                    ToolTip.text: model.username
                }

                MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
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
