/*
    SPDX-FileCopyrightText: 2019 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1 as QQC2
import org.kde.kirigami 2.17 as Kirigami
import org.kde.kpublictransport 1.0
import org.kde.itinerary 1.0
import "." as App

RowLayout {
    id: root
    property var journey

    function maxLoad(loadInformation) {
        var load = Load.Unknown;
        for (var i = 0; i < loadInformation.length; ++i) {
            load = Math.max(load, loadInformation[i].load);
        }
        return load;
    }

    readonly property int sectionWithMaxLoad: {
        var loadMax = Load.Unknown;
        var loadMaxIdx = -1;
        for (var i = 0; journey != undefined && i < journey.sections.length; ++i) {
            var l = maxLoad(journey.sections[i].loadInformation);
            if (l > loadMax) {
                loadMax = l;
                loadMaxIdx = i;
            }
        }
        return loadMaxIdx;
    }

    Repeater {
        model: journey.sections
        delegate: Kirigami.Icon {
            source: PublicTransport.journeySectionIcon(modelData)
            color: PublicTransport.warnAboutSection(modelData) ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.textColor
            isMask: modelData.mode != JourneySection.PublicTransport || (!modelData.route.line.hasLogo && !modelData.route.line.hasModeLogo)
            Layout.preferredWidth: Kirigami.Units.iconSizes.small * Util.svgAspectRatio(source)
            Layout.preferredHeight: Kirigami.Units.iconSizes.small
            Accessible.name: PublicTransport.journeySectionLabel(modelData)
        }
    }
    QQC2.Label {
        text: i18ncp("number of switches to another transport", "One change", "%1 changes", journey.numberOfChanges)
        Layout.fillWidth: true
        Accessible.ignored: !parent.visible
    }

    App.VehicleLoadIndicator {
        loadInformation: sectionWithMaxLoad < 0 ? undefined : root.journey.sections[sectionWithMaxLoad].loadInformation
    }
}
