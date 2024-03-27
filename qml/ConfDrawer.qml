import QtQuick.Controls
import QtQuick
import QtQuick.Layouts
import Esterv.Iota.Account
import Esterv.Styles.Simple
import Esterv.Iota.NodeConnection
import Esterv.CustomControls
import Esterv.Iota.NFTMinter
Drawer
{
    id: drawer
    closePolicy: Popup.CloseOnPressOutside
    focus:true
    modal:true
    background: Rectangle
    {
        color:Style.backColor2
    }

    ColumnLayout {
        anchors.fill: parent


        ThemeSwitch
        {
            Layout.alignment: Qt.AlignTop || Qt.AlignLeft
        }

        NodeConnectionSettings
        {
            id:conn_
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }
        AccountSettings
        {
            id:acc_
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
        }
        UpdateFrame
        {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
        }
    }

}
