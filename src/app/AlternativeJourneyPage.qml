/*
    SPDX-FileCopyrightText: 2019-2021 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kpublictransport 1.0
import org.kde.itinerary 1.0
import "." as App

App.JourneyQueryPage {
    id: root

    required property QtObject controller

    title: i18n("Alternative Connections")

    journeyRequest: controller.journeyRequestFull

    function updateRequest() {
        root.journeyRequest = fullJourneyAction.chhecked ? controller.journexRequestFull : controller.journeyRequestOne;

        let allLineModes = true;
        for (const s of [longDistanceModeAction, localTrainModeAction, rapidTransitModeAction, busModeAction, ferryModeAction]) {
            if (!s.checked) {
                allLineModes = false;
            }
        }

        let lineModes = [];
        if (!allLineModes) {
            if (longDistanceModeAction.checked)
                lineModes.push(Line.LongDistanceTrain, Line.Train);
            if (localTrainModeAction.checked)
                lineModes.push(Line.LocalTrain);
            if (rapidTransitModeAction.checked)
                lineModes.push(Line.RapidTransit, Line.Metro, Line.Tramway, Line.RailShuttle);
            if (busModeAction.checked)
                lineModes.push(Line.Bus, Line.Coach);
            if (ferryModeAction.checked)
                lineModes.push(Line.Ferry, Line.Boat);
        }
        root.journeyRequest.lineModes = lineModes;
        return req;
    }

    onJourneyChanged: replaceWarningDialog.open()

    Component.onCompleted: {
        for (const action of [fullJourneyAction, oneJourneyAction, actionSeparator, longDistanceModeAction, localTrainModeAction, rapidTransitModeAction, busModeAction, ferryModeAction]) {
                actions.contextualActions.push(action);
        }
    }

    data: [
        QQC2.ActionGroup { id: journeyActionGroup },
        Kirigami.Action {
            id: fullJourneyAction
            text: i18nc("to travel destination", "To %1", controller.journeyRequestFull.to.name)
            checkable: true
            checked: controller.journeyRequestFull.to.name == root.journeyRequest.to.name
            icon.name: "go-next-symbolic"
            visible: controller.journeyRequestFull.to.name != controller.journeyRequestOne.to.name
            QQC2.ActionGroup.group: journeyActionGroup
            onTriggered: updateRequest()
        },
        Kirigami.Action {
            id: oneJourneyAction
            text: i18nc("to travel destination", "To %1", controller.journeyRequestOne.to.name)
            checkable: true
            checked: controller.journeyRequestOne.to.name == root.journeyRequest.to.name
            icon.name: "go-next-symbolic"
            visible: controller.journeyRequestFull.to.name != controller.journeyRequestOne.to.name
            QQC2.ActionGroup.group: journeyActionGroup
            onTriggered: updateRequest()
        },

        Kirigami.Action {
            id: actionSeparator
            separator: true
        },

        Kirigami.Action {
            id: longDistanceModeAction
            text: i18nc("journey query search constraint, title", "Long distance trains")
            icon.source: PublicTransport.lineModeIcon(Line.LongDistanceTrain)
            checkable: true
            checked: true
            onTriggered: updateRequest()
        },
        Kirigami.Action {
            id: localTrainModeAction
            text: i18nc("journey query search constraint, title", "Local trains")
            icon.source: PublicTransport.lineModeIcon(Line.LocalTrain)
            checkable: true
            checked: true
            onTriggered: updateRequest()
        },
        Kirigami.Action {
            id: rapidTransitModeAction
            text: i18nc("journey query search constraint, title", "Rapid transit")
            icon.source: PublicTransport.lineModeIcon(Line.Tramway)
            checkable: true
            checked: true
            onTriggered: updateRequest()
        },
        Kirigami.Action {
            id: busModeAction
            text: i18nc("journey query search constraint, title", "Bus")
            icon.source: PublicTransport.lineModeIcon(Line.Bus)
            checkable: true
            checked: true
            onTriggered: updateRequest()
        },
        Kirigami.Action {
            id: ferryModeAction
            text: i18nc("journey query search constraint, title", "Ferry")
            icon.source: PublicTransport.lineModeIcon(Line.Ferry)
            checkable: true
            checked: true
            onTriggered: updateRequest()
        },

        Kirigami.PromptDialog {
            id: replaceWarningDialog

            title: i18n("Replace Journey")
            subtitle: i18n("Do you really want to replace your existing reservation with the newly selected journey?")
            standardButtons: QQC2.Dialog.No
            customFooterActions: [
                Kirigami.Action {
                    text: i18n("Replace")
                    icon.name: "document-save"
                    onTriggered: {
                        controller.applyJourney(root.journey, root.journeyRequest.to.name == controller.journeyRequestFull.to.name);
                        applicationWindow().pageStack.pop();
                    }
                }
            ]
        }
    ]
}
