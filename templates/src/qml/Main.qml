import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

// View 层：只做声明式 UI。
// 状态与命令一律来自 ViewModel（组合根注入的根上下文属性 "App"），
// 这里不写业务规则，也不直接引用任何 C++ 领域类型。
ApplicationWindow {
    id: window
    width: 640
    height: 480
    minimumWidth: 240
    minimumHeight: 200
    visible: true
    title: qsTr("{{APP_DISPLAY_NAME}}")

    property bool lightMode: Application.styleHints.colorScheme === Qt.Light
    property color background: lightMode ? "#e7e7e7" : "#1f1f1f"
    property color foreground: lightMode ? "#262626" : "#e0e0e0"

    Rectangle {
        anchors.fill: parent
        color: window.background

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 16

            // 单向绑定：ViewModel 属性 -> View
            Label {
                Layout.alignment: Qt.AlignHCenter
                color: window.foreground
                font.pixelSize: 24
                text: App.greeting
            }

            // 只读派生状态：来自领域模型，经 ViewModel 透出
            Label {
                Layout.alignment: Qt.AlignHCenter
                color: window.foreground
                opacity: 0.7
                text: qsTr("Model title: %1").arg(App.title)
            }

            // 命令：View -> ViewModel
            Button {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Refresh")
                onClicked: App.refreshGreeting()
            }
        }
    }
}
