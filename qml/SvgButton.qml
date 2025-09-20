import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15

Item
{
    id: svgButton

    property bool toggled
    property bool doSmooth: false

    // Button sources to be set by the parent.
    property url normalSource: ""
    property url hoverSource: ""
    property url pressedSource: ""

    property url normalSourceToggled: ""
    property url hoverSourceToggled: ""
    property url pressedSourceToggled: ""

    signal clicked

    Image {
        id: icon
        anchors.fill: parent
        source: {
            var src = ""
            if (toggled)
            {
                if (svgButtonMouseArea.pressed)
                {
                    src = svgButton.pressedSourceToggled
                }
                else if (svgButtonMouseArea.containsMouse)
                {
                    src = svgButton.hoverSourceToggled
                }
                else
                {
                    src = svgButton.normalSourceToggled
                }
            }
            else
            {
                if (svgButtonMouseArea.pressed)
                {
                    src = svgButton.pressedSource
                }
                else if (svgButtonMouseArea.containsMouse)
                {
                    src = svgButton.hoverSource
                }
                else
                {
                    src = svgButton.normalSource
                }
            }

            return src
        }
        fillMode: Image.PreserveAspectFit
        smooth: doSmooth
    }

    MouseArea {
        id: svgButtonMouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: svgButton.clicked()
    }

}
