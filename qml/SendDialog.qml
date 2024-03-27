import QtQuick
import QtQuick.Layouts
import Esterv.CustomControls
import QtQuick.Controls


import Esterv.Iota.NFTMinter
import Esterv.CustomControls.QrDec
import Esterv.Iota.AddrBundle
import Esterv.Iota.Wallet
import Esterv.CustomControls.DateTimePickers

Popup
{
    id:control
    visible:false
    onClosed:  {

        recAddress.text="";
        timeselector.ocustom = new Date(undefined);
        datetimepicker.selDate = new Date();
        datetimepicker.initDate = new Date();
        timeselector.currentIndex=0;
    }
    Popup
    {
        id: timepop
        width:350
        height:450
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnPressOutside
        focus:true
        modal:true
        visible:false

        ColumnLayout
        {
            anchors.fill:parent
            DateTimePicker
            {
                id:datetimepicker
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            Button
            {
                text: qsTr("Confirm")
                onClicked:
                {
                    timeselector.ocustom=datetimepicker.selDate
                    timeselector.currentIndex=4;
                    timepop.close();
                }
            }
        }

    }


    ColumnLayout
    {
        anchors.fill: parent
        TabBar {
            id:chooseSendAll
            property bool sendAll:false
            Layout.fillWidth: true
            visible:!BoxModel.selecteds
            TabButton {
                text: qsTr("Send")
                onClicked:
                {
                    chooseSendAll.sendAll=false;
                }
            }
            TabButton {
                text: qsTr("Send all")
                onClicked:
                {
                    chooseSendAll.sendAll=true;
                }

            }
        }
        Frame
        {
            id:tokenAmount
            Layout.fillWidth: true
            visible:!chooseSendAll.sendAll&&!BoxModel.selecteds
            ColumnLayout
            {
                Label
                {
                    text:qsTr("Amount:")
                }
                RowLayout
                {

                    TextField
                    {
                        id:sendAmount

                        inputMethodHints:Qt.ImhDigitsOnly
                        inputMask: "00000000000000000"
                    }
                    Label
                    {
                        text: Wallet.amount.json.largeValue.unit
                    }
                }
            }

        }

        Frame
        {
            Layout.fillWidth: true
            ColumnLayout
            {
                anchors.fill:parent
                Label
                {
                    text:qsTr("Recipient:")
                }
                QrTextField
                {
                    id:recAddress
                    Layout.fillWidth: true
                    AddressChecker
                    {
                        id:recAddrCheck
                        address:recAddress.text
                    }
                    ToolTip.text: qsTr("Not an address")
                    ToolTip.visible: (!recAddrCheck.valid)&&(recAddrCheck.address!=="")
                }
            }
        }
        Button
        {
            id:expbutt
            text:qsTr("Expiration "+ ((expFrame.visible)?"-":"+"))
            onClicked: expFrame.visible=!expFrame.visible
            visible:BoxModel.selecteds||!chooseSendAll.sendAll
            onVisibleChanged: {
                if(!expbutt.visible)expFrame.visible=false;
            }
        }
        Frame
        {
            id:expFrame
            visible: false
            Layout.fillHeight: true
            Layout.fillWidth: true
            ColumnLayout
            {
                anchors.fill: parent
                Label
                {
                    text:qsTr("Expiration Time:")
                }
                ComboBox
                {
                    Layout.fillWidth: true
                    id:timeselector
                    textRole: "text"
                    valueRole: "value"

                    function plusHours(n){
                        const today = new Date();
                        const reday = new Date(today);
                        reday.setHours(today.getHours() + n);
                        return reday;
                    }
                    property date now: new Date()
                    property date onone:new Date(undefined)
                    property date onehour: plusHours(1)
                    property date oneday: plusHours(24)
                    property date oneweek:plusHours(24*7)
                    property date ocustom:new Date(undefined)
                    model: [
                        { value: new Date(undefined), text: qsTr("No Expiration Time") },
                        { value: onehour,
                            text: qsTr("In one hour \n" +
                                       timeselector.onehour.toLocaleDateString(Qt.locale(),"yyyy\n MMM dd ") +
                                       timeselector.onehour.toLocaleTimeString(Qt.locale(),"h:mm a")) },
                        { value: oneday,
                            text: qsTr("In one day \n" +
                                       oneday.toLocaleDateString(Qt.locale(),"yyyy\n MMM dd ") +
                                       oneday.toLocaleTimeString(Qt.locale(),"h:mm a")) },
                        { value: oneweek,
                            text: qsTr("In one week \n" +
                                       oneweek.toLocaleDateString(Qt.locale(),"yyyy\n MMM dd ") +
                                       oneweek.toLocaleTimeString(Qt.locale(),"h:mm a")) },
                        { value: ocustom,
                            text: qsTr("Custom date \n" + ocustom.toLocaleDateString(Qt.locale(),"yyyy\n MMM dd ") +
                                       ocustom.toLocaleTimeString(Qt.locale(),"h:mm a")) },
                    ]
                    onActivated: (index) =>
                                 {
                                     if(index===4)
                                     {
                                         timepop.open();
                                     }
                                 }
                }

            }
        }
        DelayButton
        {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Send")
            delay: 2000
            ToolTip.text: qsTr("Hold to Send")
            ToolTip.visible: hovered
            enabled: (!tokenAmount.visible||parseInt(sendAmount.text)>0)&&recAddrCheck.valid&&((expFrame.visible&&timeselector.currentIndex)||!expFrame.visible)
            onActivated:
            {

                BoxModel.send((tokenAmount.visible)?sendAmount.text:"0",recAddress.text, timeselector.currentValue);

                control.close();
            }
        }

    }

}


