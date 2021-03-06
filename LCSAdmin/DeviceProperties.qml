import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import Utils 1.0

Rectangle {
    id: devicePropertyGroup
    implicitHeight: listView.visible ? textMetrics.itemHeight * listView.count + textMetrics.itemHeight : textMetrics.itemHeight
    border.color: "lightgrey"
    border.width: 1
    visible: listView.count > 0 ? true : false

    property int deviceID: -1
    property int deviceClass: 0

    RouteModel{
        id: routeModel
    }

    DeviceModel {
        id: deviceModel
        deviceClass: devicePropertyGroup.deviceClass == 3 ? 99 : 0
    }

    TextMetrics {
        id: textMetrics
        property int itemHeight: textMetrics.height + 35
        text: "CLASS>MULTI-CONTROLLER"
    }

    DevicePropertyModel {
        id: model
        deviceID:  devicePropertyGroup.deviceID
        deviceClass: devicePropertyGroup.deviceClass
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            listView.visible = !listView.visible;
        }
    }

    Text {
        id: titleText
        text: "<b>Additional Information</b>"
        height: implicitHeight
        font.pixelSize: textMetrics.height + (textMetrics.height * 0.5)
        font.bold: true
        horizontalAlignment: Qt.AlignHCenter
        anchors.topMargin: 5
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }

    ListView {
        id: listView
        anchors.top: titleText.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 5
        spacing: 3
        clip: true

        Component {
            id: detialDelegate
            Item {
                id: wrapper
                width: parent.width
                height: textMetrics.itemHeight
                Text {
                    id: keyText
                    text: "<b>" + key + ":</b>"
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    anchors.left: parent.left
                    width: parent.width * .75
                    height: parent.height
//                    Layout.fillWidth: true
                }
                ControlComboBox {
                    id: deviceCombo
                    visible: false //key === "ITEMID"
                    model: deviceModel
                    lookupField: "deviceID"
                    displayField: "deviceName"
                    currentIndex: deviceModel.getDeviceRow(value);
                    height: parent.height
                    dataValue: value
                    onDataValueChanged: {
                        value = dataValue;
                    }
                    anchors.leftMargin: 5
                    anchors.left: keyText.right
                    anchors.right: parent.right
                }
                ControlComboBox {
                    id: routeCombo
                    visible: key.startsWith("ROUTEID")
                    model: routeModel
                    lookupField: "routeID"
                    displayField: "routeName"
                    currentIndex: routeModel.getRouteRow(value);
                    height: parent.height
                    dataValue: value
                    onDataValueChanged: {
                        value = dataValue;
                    }
                    anchors.leftMargin: 5
                    anchors.left: keyText.right
                    anchors.right: parent.right
                }

                TextField {
                    id: valueField
                    visible: !deviceCombo.visible && !routeCombo.visible
                    anchors.leftMargin: 5
                    anchors.left: keyText.right
                    anchors.right: parent.right
                    height: parent.height
                    text: value ? value : ""
                    onTextChanged: {
                        value = valueField.text
                    }
                }
            }
        }

        model: model
        delegate: detialDelegate
        focus: true
    }

    function saveData()
    {
        model.save();
    }
}
