/*
    SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.khealthcertificate 1.0 as KHC
import org.kde.itinerary 1.0
import "." as App

Kirigami.Page {
    id: root
    title: i18n("Health Certificates")

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    data: BarcodeScanModeButton {
        page: root
        visible: swipeView.currentIndex === 0
    }

    readonly property bool hasValidCertificate: {
        if (certSelector.count === 0 || !certSelector.currentValue) {
            return false;
        }

        switch (certSelector.currentValue.type) {
            case KHC.HealthCertificate.Vaccination:
            case KHC.HealthCertificate.Test:
            case KHC.HealthCertificate.Recovery:
                return true;
            default:
                return false;
        }
    }

    actions.contextualActions: [
        Kirigami.Action {
            id: importFromClipboardAction
            icon.name: "edit-paste"
            text: i18n("Import from Clipboard")
            onTriggered: ApplicationController.importFromClipboard()
        },
        Kirigami.Action {
            icon.name: "edit-delete"
            text: i18n("Delete")
            onTriggered: deleteWarningDialog.open()
            enabled: certSelector.currentIndex >= 0 && certSelector.count > 0
        }
    ]

    Kirigami.PromptDialog {
        id: deleteWarningDialog

        title: i18n("Delete Certificate")
        subtitle: i18n("Do you really want to delete this certificate?")
        standardButtons: QQC2.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: i18n("Delete")
                icon.name: "edit-delete"
                onTriggered: {
                    ApplicationController.healthCertificateManager.removeCertificate(certSelector.currentIndex);
                    deleteWarningDialog.close();
                }
            }
        ]
    }
    header: ColumnLayout {
        visible: certSelector.count > 0
        spacing: 0

        QQC2.ComboBox {
            id: certSelector
            Layout.margins: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
            model: ApplicationController.healthCertificateManager
            valueRole: "certificate"
            textRole: "display"
            Connections {
                target: ApplicationController.healthCertificateManager
                function onNewCertificateLoaded(index) {
                    certSelector.currentIndex = index;
                }
            }
            onCurrentIndexChanged: Settings.write("HealthCertificatePage/currentCertificateIndex", currentIndex)
            currentIndex: Settings.read("HealthCertificatePage/currentCertificateIndex", 0)
        }
        Kirigami.Separator {
            Layout.fillWidth: true
        }
    }

    Component {
        id: vaccinationDetails
        App.HealthCertificateVaccination {
            certificate: certSelector.currentValue
        }
    }
    Component {
        id: testDetails
        App.HealthCertificateTest {
            certificate: certSelector.currentValue
        }
    }
    Component {
        id: recoveryDetails
        App.HealthCertificateRecovery {
            certificate: certSelector.currentValue
        }
    }

    ColumnLayout {
        visible: !hasValidCertificate
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        spacing: Kirigami.Units.largeSpacing
        Kirigami.Heading {
            text: i18n('No health certificates found')
            horizontalAlignment: Qt.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            level: 2
        }
        QQC2.Label {
            text: i18n("<p>You can import the following health certificates by scanning them with a barcode scanner app:<ul><li>European \"Digital Green Certificates\" for vaccinations, tests or recovery.</li><li>Indian vaccination certificates.</li><li>SMART Health Cards (SHC) as used in parts of Canada and the US.</li></p>")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: Kirigami.Units.gridUnit * 25
        }
        QQC2.Button {
            action: importFromClipboardAction
            Layout.alignment: Qt.AlignHCenter
        }
    }

    footer: Kirigami.NavigationTabBar {
        visible: hasValidCertificate
        actions: [
            Kirigami.Action {
                text: i18n('Certificate')
                icon.name: 'view-barcode-qr'
                onTriggered: swipeView.currentIndex = 0
                checked: swipeView.currentIndex === 0
            },
            Kirigami.Action {
                text: i18n('Detail')
                icon.name: 'view-list-details'
                onTriggered: swipeView.currentIndex = 1;
                checked: swipeView.currentIndex === 1
            }
        ]
    }
    QQC2.SwipeView {
        id: swipeView
        visible: hasValidCertificate
        anchors.fill: parent

        App.HealthCertificateBarcode {
            certificate: certSelector.currentValue
            TapHandler {
                onDoubleTapped: scanModeController.toggle()
            }
            implicitWidth: parent.width
        }

        Kirigami.ScrollablePage {
            padding: Kirigami.Units.largeSpacing
            Loader {
                id: loader
                width: parent.width
                sourceComponent: {
                    if (!hasValidCertificate) {
                        return undefined;
                    }
                    switch (certSelector.currentValue.type) {
                        case KHC.HealthCertificate.Vaccination:
                            return vaccinationDetails;
                        case KHC.HealthCertificate.Test:
                            return testDetails;
                        case KHC.HealthCertificate.Recovery:
                            return recoveryDetails;
                        default:
                            return undefined;
                    }
                }
            }
        }
    }
}
