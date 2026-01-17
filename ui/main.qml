import QtQuick
import QtQuick.Window

Window {
    width: 420
    height: 320
    visible: true
    title: qsTr("IR Trigger (Python)")

    Column {
        anchors.centerIn: parent
        spacing: 16

        Text {
            text: backend.detected ? "Status: OBJECT DETECTED" : "Status: NO OBJECT"
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
            width: 360
        }
        Text {
            text: backend.statusText
            font.pixelSize: 12
            color: "gray"
            wrapMode: Text.WordWrap
            width: 360
        }
        Rectangle {
            width: 160
            height: 160
            radius: width / 2
            color: backend.detected ? "green" : "red"
            border.width: 2
            border.color: "black"
        }
    }
}
