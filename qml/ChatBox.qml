import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

import GUICommon 1.0

ColumnLayout {
    spacing: 0

    property alias messageInput: messageInput

    // Display all messages in a scrollable area.
    ListView {
        id: messageList
        model: MessagesModel

        Layout.fillWidth: true
        // Reserve 80% for message history.
        Layout.fillHeight: true

        clip: true
        interactive: true
        highlightFollowsCurrentItem: false

        delegate: Rectangle {
            property int padding: 4

            width: messageList.width
            // draw the text height + padding on both sides.
            height: messageText.paintedHeight + padding * 2

            color: "transparent"

            Text {
                id: messageText
                text: modelData
                wrapMode: Text.Wrap
                font.pixelSize: 14

                color: "#ffffff"
                opacity: 0.87

                anchors.fill: parent
                anchors.margins: parent.padding

                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    // Input area for new messages.
    RowLayout {
        Layout.fillWidth: true
        spacing: 0

        TextArea {
            id: messageInput

            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(implicitHeight, 200)

            placeholderText: "..."
            wrapMode: Text.Wrap
            font.pixelSize: 14

            color: "#f2f2f2"

            topPadding: 4
            bottomPadding: 8
            leftPadding: 6
            rightPadding: 6

            background: Rectangle {
                anchors.fill:parent
                color: "#26282a"
                border.width: 1
                border.color: "#3e4042"
                radius: 0
            }

            // Submit text when pressing enter, but not shift enter.
            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Return
                    && !(event.modifiers & Qt.ShiftModifier)) {

                    event.accepted = true
                    Client.write_message(text)
                    messageInput.text = ""
                }
            }
        }
    }
}
