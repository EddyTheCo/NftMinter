import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Esterv.CustomControls.QrGen
import Esterv.Iota.Account
import Esterv.Styles.Simple
import Esterv.Iota.AddrBundle
import Esterv.Iota.Wallet
import Esterv.Iota.NodeConnection
import Esterv.Iota.NFTMinter

Rectangle
{
    id:root
    property bool incolum: height>300
    signal showSettings()
    color:Style.backColor2
    radius:3

    SendDialog
    {
        id:senddiallog
        width:350
        height:450
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside
        focus:true
        modal:true
    }

    ColumnLayout
    {
        anchors.fill: parent
        RowLayout
        {
            Layout.fillWidth: true
            Layout.minimumWidth: 300
            Layout.margins: 5

            Label
            {
                id:accl
                text:qsTr("Account:")
                Layout.alignment: Qt.AlignHCenter|Qt.AlignTop
                horizontalAlignment:Text.AlignRight
            }
            QrText
            {
                text:Wallet.addresses.length?Wallet.addresses[0].bech32Address:""
                visible:Wallet.addresses.length
                Layout.fillWidth: true
                Layout.maximumWidth: implicitWidth
                Layout.alignment: Qt.AlignHCenter|Qt.AlignTop
            }
        }

        Label
        {
            Layout.alignment: Qt.AlignHCenter|Qt.AlignTop
            font:Style.h3
            text:qsTr("Available Balance: ")
        }
        TextAmount
        {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter|Qt.AlignTop
            font:Style.h2
            amount:Wallet.amount
            horizontalAlignment:Text.AlignHCenter
        }

        GridLayout
        {
            rowSpacing: 5
            columnSpacing: 15
            Layout.margins: 5
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            Layout.minimumHeight: 45+100*root.incolum
            flow: root.incolum ? GridLayout.TopToBottom : GridLayout.LeftToRight
            Button
            {
                id:settings_
                text:qsTr("Settings")
                onReleased:root.showSettings();
                Layout.fillWidth: true
                Layout.maximumWidth: 200
                Layout.maximumHeight:  50
                ToolTip.text: text
                ToolTip.visible: hovered
            }
            Button
            {
                text:qsTr("Add New")
                Layout.fillWidth: true
                Layout.maximumWidth: 200
                Layout.maximumHeight:  50
                ToolTip.text: text
                ToolTip.visible: hovered
                onClicked:BoxModel.newBox();
            }
            Button
            {
                text:qsTr((BoxModel.selecteds)?("Send NFT"+((BoxModel.selecteds===1)?"":"s")):"Send")
                enabled:(NodeConnection.state&&((Object.keys(Wallet.amount.json).length != 0)&&Wallet.amount.json.largeValue.value>0))
                Layout.fillWidth: true
                Layout.maximumWidth: 200
                Layout.maximumHeight:  50
                ToolTip.text: text
                ToolTip.visible: hovered
                onClicked: senddiallog.visible=true;
            }
        }

    }


}


