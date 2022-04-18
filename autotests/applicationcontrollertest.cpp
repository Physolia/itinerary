/*
    SPDX-FileCopyrightText: 2018 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "testhelper.h"

#include <applicationcontroller.h>
#include <pkpassmanager.h>
#include <reservationmanager.h>
#include <documentmanager.h>
#include <favoritelocationmodel.h>
#include <healthcertificatemanager.h>
#include <passmanager.h>
#include <transfermanager.h>
#include <tripgroupmanager.h>

#include <KItinerary/ExtractorCapabilities>
#include <KItinerary/File>

#include <QUrl>
#include <QtTest/qtest.h>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>

class AppControllerTest : public QObject
{
    Q_OBJECT
private:
    bool hasZxing()
    {
        return KItinerary::ExtractorCapabilities::capabilitiesString().contains(QLatin1String("zxing"), Qt::CaseInsensitive);
    }

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("TZ", "UTC");
        QStandardPaths::setTestModeEnabled(true);
    }

    void testImportData()
    {
        PkPassManager passMgr;
        Test::clearAll(&passMgr);
        QSignalSpy passSpy(&passMgr, &PkPassManager::passAdded);
        QVERIFY(passSpy.isValid());

        ReservationManager resMgr;
        Test::clearAll(&resMgr);
        QSignalSpy resSpy(&resMgr, &ReservationManager::reservationAdded);
        QVERIFY(resSpy.isValid());

        DocumentManager docMgr;
        Test::clearAll(&docMgr);

        HealthCertificateManager healthCertMgr;

        ApplicationController appController;
        QSignalSpy infoSpy(&appController, &ApplicationController::infoMessage);
        appController.setPkPassManager(&passMgr);
        appController.setReservationManager(&resMgr);
        appController.setDocumentManager(&docMgr);
        appController.setHealthCertificateManager(&healthCertMgr);

        appController.importData(Test::readFile(QLatin1String(SOURCE_DIR "/data/4U8465-v1.json")));
        QCOMPARE(resSpy.size(), 1);
        QCOMPARE(passSpy.size(), 0);
        QCOMPARE(infoSpy.size(), 1);
        appController.importData(Test::readFile(QLatin1String(SOURCE_DIR "/data/boardingpass-v1.pkpass")));
        QCOMPARE(resSpy.size(), 2);
        QCOMPARE(passSpy.size(), 1);
        QCOMPARE(infoSpy.size(), 2);
        appController.importData("M1DOE/JOHN            EXXX007 TXLBRUSN 2592 110Y");
        QCOMPARE(resSpy.size(), 3);
        QCOMPARE(passSpy.size(), 1);
        QCOMPARE(infoSpy.size(), 3);
        appController.importData(Test::readFile(QLatin1String(SOURCE_DIR "/data/iata-bcbp-demo.pdf")));
        if (!hasZxing()) {
            QSKIP("Built without ZXing");
        }
        QCOMPARE(resSpy.size(), 4);
        QCOMPARE(passSpy.size(), 1);
        QCOMPARE(infoSpy.size(), 4);
        QCOMPARE(docMgr.documents().size(), 1);
    }

    void testImportFile()
    {
        PkPassManager passMgr;
        Test::clearAll(&passMgr);
        QSignalSpy passSpy(&passMgr, &PkPassManager::passAdded);
        QVERIFY(passSpy.isValid());

        ReservationManager resMgr;
        Test::clearAll(&resMgr);
        QSignalSpy resSpy(&resMgr, &ReservationManager::reservationAdded);
        QVERIFY(resSpy.isValid());

        DocumentManager docMgr;
        Test::clearAll(&docMgr);

        HealthCertificateManager healthCertMgr;

        ApplicationController appController;
        QSignalSpy infoSpy(&appController, &ApplicationController::infoMessage);
        appController.setPkPassManager(&passMgr);
        appController.setReservationManager(&resMgr);
        appController.setDocumentManager(&docMgr);
        appController.setHealthCertificateManager(&healthCertMgr);

        appController.importFromUrl(QUrl::fromLocalFile(QLatin1String(SOURCE_DIR "/data/4U8465-v1.json")));
        QCOMPARE(resSpy.size(), 1);
        QCOMPARE(passSpy.size(), 0);
        QCOMPARE(infoSpy.size(), 1);
        appController.importFromUrl(QUrl::fromLocalFile(QLatin1String(SOURCE_DIR "/data/boardingpass-v1.pkpass")));
        QCOMPARE(resSpy.size(), 2);
        QCOMPARE(passSpy.size(), 1);
        QCOMPARE(infoSpy.size(), 2);
        appController.importFromUrl(QUrl::fromLocalFile(QLatin1String(SOURCE_DIR "/data/iata-bcbp-demo.pdf")));
        if (!hasZxing()) {
            QSKIP("Built without ZXing");
        }
        QCOMPARE(resSpy.size(), 3);
        QCOMPARE(passSpy.size(), 1);
        QCOMPARE(infoSpy.size(), 3);
        QCOMPARE(docMgr.documents().size(), 1);
    }

    void testExportFile()
    {
        PkPassManager pkPassMgr;
        Test::clearAll(&pkPassMgr);

        ReservationManager resMgr;
        Test::clearAll(&resMgr);

        DocumentManager docMgr;
        Test::clearAll(&docMgr);

        TripGroupManager tripGroupMgr;
        tripGroupMgr.setReservationManager(&resMgr);
        TransferManager transferMgr;
        transferMgr.setReservationManager(&resMgr);
        transferMgr.setTripGroupManager(&tripGroupMgr);

        FavoriteLocationModel favLoc;

        PassManager passMgr;
        Test::clearAll(&passMgr);

        HealthCertificateManager healthCertMgr;

        ApplicationController appController;
        QSignalSpy infoSpy(&appController, &ApplicationController::infoMessage);
        appController.setPkPassManager(&pkPassMgr);
        appController.setReservationManager(&resMgr);
        appController.setDocumentManager(&docMgr);
        appController.setTransferManager(&transferMgr);
        appController.setFavoriteLocationModel(&favLoc);
        appController.setPassManager(&passMgr);
        appController.setHealthCertificateManager(&healthCertMgr);

        appController.importFromUrl(QUrl::fromLocalFile(QLatin1String(SOURCE_DIR "/data/4U8465-v1.json")));
        appController.importFromUrl(QUrl::fromLocalFile(QLatin1String(SOURCE_DIR "/data/boardingpass-v1.pkpass")));
        appController.importFromUrl(QUrl::fromLocalFile(QLatin1String(SOURCE_DIR "/data/bahncard.json")));
        QCOMPARE(resMgr.batches().size(), 2);
        QCOMPARE(pkPassMgr.passes().size(), 1);
        QCOMPARE(passMgr.rowCount(), 1);
        QCOMPARE(infoSpy.size(), 3);

        QTemporaryFile tmp;
        QVERIFY(tmp.open());
        tmp.close();
        appController.exportToFile(QUrl::fromLocalFile(tmp.fileName()));
        QCOMPARE(infoSpy.size(), 4);

        KItinerary::File f(tmp.fileName());
        QVERIFY(f.open(KItinerary::File::Read));
        QCOMPARE(f.reservations().size(), 2);
        QCOMPARE(f.passes().size(), 1);

        Test::clearAll(&pkPassMgr);
        Test::clearAll(&resMgr);
        Test::clearAll(&docMgr);
        Test::clearAll(&passMgr);
        QCOMPARE(resMgr.batches().size(), 0);
        QCOMPARE(pkPassMgr.passes().size(), 0);

        appController.importFromUrl(QUrl::fromLocalFile(tmp.fileName()));
        QCOMPARE(resMgr.batches().size(), 2);
        QCOMPARE(pkPassMgr.passes().size(), 1);
        QCOMPARE(passMgr.rowCount(), 1);
        QCOMPARE(infoSpy.size(), 5);

        Test::clearAll(&pkPassMgr);
        Test::clearAll(&resMgr);
        Test::clearAll(&passMgr);
        QCOMPARE(resMgr.batches().size(), 0);
        QCOMPARE(pkPassMgr.passes().size(), 0);

        QFile bundle(tmp.fileName());
        QVERIFY(bundle.open(QFile::ReadOnly));
        appController.importData(bundle.readAll());
        QCOMPARE(resMgr.batches().size(), 2);
        QCOMPARE(pkPassMgr.passes().size(), 1);
        QCOMPARE(passMgr.rowCount(), 1);
        QCOMPARE(infoSpy.size(), 6);
    }
};

QTEST_GUILESS_MAIN(AppControllerTest)

#include "applicationcontrollertest.moc"
