/*
    SPDX-FileCopyrightText: 2019 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "testhelper.h"
#include <documentmanager.h>

#include <KItinerary/CreativeWork>

#include <QtTest/qtest.h>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>

using namespace KItinerary;

class DocumentManagerTest : public QObject

{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        qputenv("TZ", "UTC");
        QStandardPaths::setTestModeEnabled(true);
    }

    void testAddRemove()
    {
        DocumentManager mgr;
        Test::clearAll(&mgr);
        QSignalSpy addSpy(&mgr, &DocumentManager::documentAdded);
        QSignalSpy rmSpy(&mgr, &DocumentManager::documentRemoved);
        QCOMPARE(mgr.documents().size(), 0);

        DigitalDocument docInfo;
        docInfo.setName(QStringLiteral("../boarding*pass.pdf"));
        docInfo.setDescription(QStringLiteral("Boarding Pass"));
        docInfo.setEncodingFormat(QStringLiteral("application/pdf"));
        mgr.addDocument(QStringLiteral("docId1"), docInfo, QByteArray("%PDF123456"));
        QCOMPARE(addSpy.size(), 1);
        QCOMPARE(addSpy.at(0).at(0).toString(), QLatin1String("docId1"));
        QCOMPARE(rmSpy.size(), 0);
        QCOMPARE(mgr.documents().size(), 1);
        QVERIFY(mgr.documents().contains(QLatin1String("docId1")));

        docInfo = mgr.documentInfo(QStringLiteral("docId1")).value<DigitalDocument>();
        QCOMPARE(docInfo.name(), QLatin1String("boarding_pass.pdf"));
        QCOMPARE(docInfo.encodingFormat(), QLatin1String("application/pdf"));
        QCOMPARE(docInfo.description(), QLatin1String("Boarding Pass"));

        const auto docFilePath = mgr.documentFilePath(QStringLiteral("docId1"));
        QVERIFY(docFilePath.endsWith(QLatin1String("/docId1/boarding_pass.pdf")));
        QVERIFY(QFile::exists(docFilePath));
        QCOMPARE(Test::readFile(docFilePath), QByteArray("%PDF123456"));

        mgr.addDocument(QStringLiteral("docId2"), docInfo, docFilePath);
        QCOMPARE(addSpy.size(), 2);
        QCOMPARE(rmSpy.size(), 0);
        const auto docFilePath2 = mgr.documentFilePath(QStringLiteral("docId2"));
        QVERIFY(docFilePath2.endsWith(QLatin1String("/docId2/boarding_pass.pdf")));
        QVERIFY(docFilePath != docFilePath2);
        QVERIFY(QFile::exists(docFilePath2));
        QCOMPARE(Test::readFile(docFilePath2), QByteArray("%PDF123456"));
        QCOMPARE(mgr.documents().size(), 2);
        QVERIFY(mgr.documents().contains(QLatin1String("docId2")));

        QVERIFY(mgr.hasDocument(QStringLiteral("docId2")));
        QVERIFY(!mgr.hasDocument(QStringLiteral("docId3")));

        mgr.removeDocument(QLatin1String("docId1"));
        QCOMPARE(addSpy.size(), 2);
        QCOMPARE(rmSpy.size(), 1);
        QCOMPARE(rmSpy.at(0).at(0).toString(), QLatin1String("docId1"));
        QVERIFY(!QFile::exists(docFilePath));
        QCOMPARE(mgr.documents().size(), 1);
        QVERIFY(mgr.documents().contains(QLatin1String("docId2")));

        mgr.removeDocument(QLatin1String("docId2"));
        QCOMPARE(addSpy.size(), 2);
        QCOMPARE(rmSpy.size(), 2);
        QCOMPARE(rmSpy.at(1).at(0).toString(), QLatin1String("docId2"));
        QVERIFY(!QFile::exists(docFilePath2));
        QCOMPARE(mgr.documents().size(), 0);
    }
};

QTEST_GUILESS_MAIN(DocumentManagerTest)

#include "documentmanagertest.moc"
