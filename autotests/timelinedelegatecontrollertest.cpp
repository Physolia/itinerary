/*
    Copyright (C) 2019 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <timelinedelegatecontroller.h>
#include <reservationmanager.h>

#include <KItinerary/Reservation>
#include <KItinerary/TrainTrip>

#include <KPublicTransport/JourneyRequest>

#include <QUrl>
#include <QtTest/qtest.h>
#include <QSignalSpy>
#include <QStandardPaths>

using namespace KItinerary;

class TimelineDelegateControllerTest : public QObject
{
    Q_OBJECT
private:
    void clearReservations(ReservationManager *mgr)
    {
        const auto batches = mgr->batches(); // copy, as this is getting modified in the process
        for (const auto &id : batches) {
            mgr->removeBatch(id);
        }
        QCOMPARE(mgr->batches().size(), 0);
    }

    QByteArray readFile(const QString &fn)
    {
        QFile f(fn);
        f.open(QFile::ReadOnly);
        return f.readAll();
    }

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("TZ", "UTC");
        QStandardPaths::setTestModeEnabled(true);
    }

    void testEmptyController()
    {
        TimelineDelegateController controller;
        QCOMPARE(controller.isCurrent(), false);
        QCOMPARE(controller.progress(), 0.0f);
        QCOMPARE(controller.effectiveEndTime(), QDateTime());
        QCOMPARE(controller.isLocationChange(), false);
        QCOMPARE(controller.isPublicTransport(), false);
        QVERIFY(controller.journeyRequest().isEmpty());

        controller.setBatchId(QStringLiteral("foo"));
        QCOMPARE(controller.isCurrent(), false);
        QCOMPARE(controller.progress(), 0.0f);
        QCOMPARE(controller.effectiveEndTime(), QDateTime());
        QCOMPARE(controller.isLocationChange(), false);
        QCOMPARE(controller.isPublicTransport(), false);

        ReservationManager mgr;
        controller.setReservationManager(&mgr);
        QCOMPARE(controller.isCurrent(), false);
        QCOMPARE(controller.progress(), 0.0f);
        QCOMPARE(controller.effectiveEndTime(), QDateTime());
        QCOMPARE(controller.isLocationChange(), false);
        QCOMPARE(controller.isPublicTransport(), false);
    }

    void testProgress()
    {
        ReservationManager mgr;
        clearReservations(&mgr);

        TrainTrip trip;
        trip.setTrainNumber(QStringLiteral("TGV 1235"));
        trip.setDepartureTime(QDateTime::currentDateTime().addDays(-1));
        TrainReservation res;
        res.setReservationNumber(QStringLiteral("XXX007"));
        res.setReservationFor(trip);

        TimelineDelegateController controller;
        QSignalSpy currentSpy(&controller, &TimelineDelegateController::currentChanged);
        controller.setReservationManager(&mgr);

        mgr.addReservation(res);
        QCOMPARE(mgr.batches().size(), 1);
        const auto batchId = mgr.batches().at(0);

        controller.setBatchId(batchId);
        QCOMPARE(controller.isCurrent(), false);
        QCOMPARE(controller.progress(), 0.0f);
        QCOMPARE(controller.isLocationChange(), true);
        QCOMPARE(controller.isPublicTransport(), true);

        trip.setArrivalTime(QDateTime::currentDateTime().addDays(1));
        res.setReservationFor(trip);
        mgr.updateReservation(batchId, res);

        QCOMPARE(controller.isCurrent(), true);
        QCOMPARE(controller.progress(), 0.5f);
        QCOMPARE(controller.effectiveEndTime(), trip.arrivalTime());
        QCOMPARE(currentSpy.size(), 1);
    }

    void testPreviousLocation()
    {
        ReservationManager mgr;
        clearReservations(&mgr);

        TimelineDelegateController controller;
        controller.setReservationManager(&mgr);

        {
            TrainTrip trip;
            trip.setTrainNumber(QStringLiteral("TGV 1235"));
            trip.setDepartureTime(QDateTime::currentDateTime().addDays(2));
            TrainReservation res;
            res.setReservationNumber(QStringLiteral("XXX007"));
            res.setReservationFor(trip);
            mgr.addReservation(res);
        }

        QCOMPARE(mgr.batches().size(), 1);
        const auto batchId = mgr.batches().at(0);

        controller.setBatchId(batchId);
        QCOMPARE(controller.previousLocation(), QVariant());

        TrainStation arrStation;
        arrStation.setName(QStringLiteral("My Station"));
        TrainTrip prevTrip;
        prevTrip.setTrainNumber(QStringLiteral("ICE 1234"));
        prevTrip.setDepartureTime(QDateTime::currentDateTime().addDays(1));
        prevTrip.setArrivalTime(QDateTime::currentDateTime().addDays(1));
        prevTrip.setArrivalStation(arrStation);
        TrainReservation prevRes;
        prevRes.setReservationNumber(QStringLiteral("XXX007"));
        prevRes.setReservationFor(prevTrip);

        QSignalSpy changeSpy(&controller, &TimelineDelegateController::previousLocationChanged);
        mgr.addReservation(prevRes);

        QCOMPARE(changeSpy.size(), 1);
        QVERIFY(!controller.previousLocation().isNull());
        QCOMPARE(controller.previousLocation().value<TrainStation>().name(), QLatin1String("My Station"));
    }

    void testJourneyRequest()
    {
        ReservationManager mgr;
        clearReservations(&mgr);
        mgr.importReservation(readFile(QLatin1String(SOURCE_DIR "/../tests/randa2017.json")));

        TimelineDelegateController controller;
        controller.setReservationManager(&mgr);
        controller.setBatchId(mgr.batches().at(0)); // flight
        QVERIFY(controller.journeyRequest().isEmpty());

        controller.setBatchId(mgr.batches().at(1)); // first train segment
        QCOMPARE(controller.isLocationChange(), true);
        QCOMPARE(controller.isPublicTransport(), true);

        auto jnyReq = controller.journeyRequest();
        QCOMPARE(jnyReq.isEmpty(), false);
        QCOMPARE(jnyReq.from().name(), QStringLiteral("Zürich Flughafen"));
        QCOMPARE(jnyReq.to().name(), QLatin1String("Randa"));

        controller.setBatchId(mgr.batches().at(2)); // second train segment
        jnyReq = controller.journeyRequest();
        QCOMPARE(jnyReq.isEmpty(), false);
        QCOMPARE(jnyReq.from().name(), QLatin1String("Visp"));
        QCOMPARE(jnyReq.to().name(), QLatin1String("Randa"));
    }
};

QTEST_GUILESS_MAIN(TimelineDelegateControllerTest)

#include "timelinedelegatecontrollertest.moc"
