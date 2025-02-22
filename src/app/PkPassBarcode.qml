/*
    SPDX-FileCopyrightText: 2018 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1 as QQC2
import org.kde.pkpass 1.0 as KPkPass
import org.kde.prison 1.0 as Prison

Rectangle {
    id: root
    property var pass

    implicitHeight: barcodeLayout.implicitHeight
    implicitWidth: barcodeLayout.implicitWidth
    color: "white"
    radius: 6
    Layout.alignment: Qt.AlignCenter

    ColumnLayout {
        id: barcodeLayout
        anchors.fill: parent
        Prison.Barcode {
            Layout.alignment: Qt.AlignCenter
            Layout.margins: 4
            Layout.preferredWidth: 0.8 * root.parent.width
            Layout.preferredHeight: implicitHeight * (Layout.preferredWidth / implicitWidth)
            barcodeType: {
                switch(pass.barcodes[0].format) {
                    case KPkPass.Barcode.QR: return Prison.Barcode.QRCode
                    case KPkPass.Barcode.Aztec: return Prison.Barcode.Aztec
                    case KPkPass.Barcode.PDF417: return Prison.Barcode.PDF417;
                    case KPkPass.Barcode.Code128: return Prison.Barcode.Code128;
                }
                return Prison.Barcode.Null;
            }
            content: pass.barcodes[0].message
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignCenter
            text: pass.barcodes[0].alternativeText
            color: "black"
            visible: text.length > 0
        }
    }
}
