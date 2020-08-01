/*
    SPDX-FileCopyrightText: 2020 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.12
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1 as QQC2
import org.kde.kirigami 2.7 as Kirigami
import org.kde.kosmindoormap 1.0
import org.kde.itinerary 1.0

Kirigami.OverlaySheet {
    id: elementDetailsSheet
    property var model

    header: Column {
        Kirigami.Heading {
            text: elementDetailsSheet.model.name
        }
        Kirigami.Heading {
            text: elementDetailsSheet.model.category
            level: 4
            visible: text != ""
        }
    }

    ListView {
        model: elementDetailsSheet.model

        Component {
            id: infoStringDelegate
            RowLayout {
                QQC2.Label {
                    visible: row.keyLabel != ""
                    text: row.keyLabel + ":"
                    color: row.category == OSMElementInformationModel.DebugCategory ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
                    Layout.alignment: Qt.AlignTop
                }
                QQC2.Label {
                    text: row.value
                    color: row.category == OSMElementInformationModel.DebugCategory ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        Component {
            id: infoLinkDelegate
            RowLayout {
                QQC2.Label {
                    visible: row.keyLabel != ""
                    text: row.keyLabel + ":"
                    color: row.category == OSMElementInformationModel.DebugCategory ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
                    Layout.alignment: Qt.AlignTop
                }
                QQC2.Label {
                    text: "<a href=\"" + row.url + "\">" + row.value + "</a>"
                    color: row.category == OSMElementInformationModel.DebugCategory ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
                    onLinkActivated: Qt.openUrlExternally(link)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        Component {
            id: infoAddressDelegate
            QQC2.Label {
                text: Localizer.formatAddress(row.value)
            }
        }

        Component {
            id: infoOpeningHoursDelegate
            // TODO this is temporary until we have a proper parser and UI for this
            QQC2.Label {
                text: row.value.includes(";") ? row.keyLabel + ":\n  " + row.value.replace(/;\s*/g, "\n  ") : row.keyLabel + ": " + row.value
            }
        }

        section.property: "categoryLabel"
        section.delegate: Kirigami.Heading {
            x: Kirigami.Units.largeSpacing
            level: 4
            text: section
            color: section == "Debug" ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.textColor
            height: implicitHeight + Kirigami.Units.largeSpacing
            verticalAlignment: Qt.AlignBottom
        }
        section.criteria: ViewSection.FullString
        section.labelPositioning: ViewSection.CurrentLabelAtStart | ViewSection.InlineLabels

        delegate: Loader {
            property var row: model
            x: Kirigami.Units.largeSpacing
            width: ListView.view.width - 2 * x
            sourceComponent: {
                switch (row.type) {
                    case OSMElementInformationModel.Link:
                        return infoLinkDelegate;
                    case OSMElementInformationModel.PostalAddress:
                        return infoAddressDelegate;
                    case OSMElementInformationModel.OpeningHoursType:
                        return infoOpeningHoursDelegate;
                    case OSMElementInformationModel.String:
                    default:
                        return infoStringDelegate;
                }
            }
        }
    }

    onSheetOpenChanged: {
        console.log(elementDetailsSheet.model);
        if (sheetOpen == false) {
            elementDetailsSheet.model.clear()
        }
    }
}
