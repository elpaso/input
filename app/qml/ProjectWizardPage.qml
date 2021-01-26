import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Dialogs 1.2
import QgsQuick 0.1 as QgsQuick
import lc 1.0
import "."
import "./components" // import InputStyle singleton
Item {
  id: projectWizardPanel

  signal back

  property real rowHeight: InputStyle.rowHeight
  property var fontColor: InputStyle.fontColor
  property var bgColor: InputStyle.clrPanelMain
  property real panelMargin: 10 * QgsQuick.Utils.dp

  property ListModel widgetsModel: ListModel {}

  //! Inits widgetsModel data just after its created, but before Component.complete is emitted (for both model or components where its used)
  property bool isWidgetModelReady: {
    var types = __fieldsModel.supportedTypes()
    for (var prop in types) {
      projectWizardPanel.widgetsModel.append({ "display": types[prop], "widget": prop })
    }

    true
  }

  // background
  Rectangle {
    width: parent.width
    height: parent.height
    color: projectWizardPanel.bgColor
  }

  PanelHeader {
    id: header
    height: InputStyle.rowHeightHeader
    width: projectWizardPanel.width
    color: InputStyle.clrPanelMain
    rowHeight: InputStyle.rowHeightHeader
    titleText: qsTr("Create Project")

    onBack: {
      projectWizardPanel.back()
    }
  }

  Item {
    height: projectWizardPanel.height - header.height - toolbar.height
    width: projectWizardPanel.width
    y: header.height

    ColumnLayout {
      id: contentLayout
      spacing: InputStyle.panelSpacing
      anchors.fill: parent
      anchors.margins: projectWizardPanel.panelMargin

      Label {
        height: projectWizardPanel.rowheight
        width: parent.width
        Layout.preferredHeight: projectWizardPanel.rowHeight
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text: qsTr("Project name")
        color: InputStyle.fontColor
        font.pixelSize: InputStyle.fontPixelSizeNormal
      }

      TextField {
        id: projectNameField
        width: parent.width
        height: projectWizardPanel.rowHeight
        font.pixelSize: InputStyle.fontPixelSizeNormal
        color: projectWizardPanel.fontColor
        placeholderText: qsTr("Project name")
        font.capitalization: Font.MixedCase
        inputMethodHints: Qt.ImhNoPredictiveText
        Layout.fillWidth: true
        Layout.preferredHeight: projectWizardPanel.rowHeight

        background: Rectangle {
          anchors.fill: parent
          border.color: projectNameField.activeFocus ? InputStyle.fontColor : InputStyle.panelBackgroundLight
          border.width: projectNameField.activeFocus ? 2 : 1
          color: InputStyle.clrPanelMain
          radius: InputStyle.cornerRadius
        }
      }

      Label {
        id: attributesLabel
        height: projectWizardPanel.rowheight
        width: parent.width
        text: qsTr("Attributes")
        color: InputStyle.fontColor
        font.pixelSize: InputStyle.fontPixelSizeNormal
        Layout.preferredHeight: projectWizardPanel.rowHeight
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
      }

      ListView {
        id: fieldList
        model: __fieldsModel
        width: parent.width
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: projectWizardPanel.rowHeight * 0.1 // same as delegateButton "margin"

        delegate: FieldRow {
          height: projectWizardPanel.rowHeight
          width: contentLayout.width
          color: projectWizardPanel.fontColor
          widgetList: projectWizardPanel.widgetsModel

          onRemoveClicked: __fieldsModel.removeField(index)
        }

        footer: DelegateButton {
            height: projectWizardPanel.rowHeight
            width: parent.width
            btnWidth: projectWizardPanel.rowHeight * 3
            btnHeight:projectWizardPanel.rowHeight * 0.8
            text: qsTr("Add field")
            iconSource: InputStyle.plusIcon
            onClicked: {
              __fieldsModel.addField("", "TextEdit")
              if (fieldList.visible) {
                fieldList.positionViewAtEnd()
              }
            }
          }
      }
    }
  }

  // footer toolbar
  Rectangle {
    property int itemSize: toolbar.height * 0.8

    id: toolbar
    height: InputStyle.rowHeightHeader
    width: parent.width
    anchors.bottom: parent.bottom
    color: InputStyle.clrPanelBackground

    MouseArea {
      anchors.fill: parent
      onClicked: {} // dont do anything, just do not let click event propagate
    }

    Row {
      height: toolbar.height
      width: parent.width
      anchors.bottom: parent.bottom

      Item {
        width: parent.width / parent.children.length
        height: parent.height
        MainPanelButton {
          id: createProjectBtn
          width: toolbar.itemSize
          text: qsTr("Create project")
          faded: !projectNameField.text
          imageSource: InputStyle.checkIcon

          onActivated: {
            if (faded) {
              __inputUtils.showNotification(qsTr("Empty project name"))
            } else {
              __projectWizard.createProject(projectNameField.text)
            }
          }
        }
      }
    }
  }
}
