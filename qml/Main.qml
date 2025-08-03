import QtQuick 2.15

ApplicationWindow {
    visible: true
    width: 400
    height: 300
    title: qsTr("QML Build Test")

    // A centered Text item to confirm everything loaded
    Text {
        anchors.centerIn: parent
        text: "Hello world"
        font.pixelSize: 24
    }
}
