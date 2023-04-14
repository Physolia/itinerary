/*
    SPDX-FileCopyrightText: 2018 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "applicationcontroller.h"
#include "documentmanager.h"
#include "favoritelocationmodel.h"
#include "filehelper.h"
#include "genericpkpass.h"
#include "gpxexport.h"
#include "healthcertificatemanager.h"
#include "importexport.h"
#include "livedatamanager.h"
#include "logging.h"
#include "passmanager.h"
#include "pkpassmanager.h"
#include "reservationmanager.h"
#include "transfermanager.h"
#include "tripgroup.h"
#include "tripgroupmanager.h"
#include <itinerary_version_detailed.h>

#include <kitinerary_version.h>
#include <KItinerary/CreativeWork>
#include <KItinerary/DocumentUtil>
#include <KItinerary/ExtractorCapabilities>
#include <KItinerary/ExtractorEngine>
#include <KItinerary/ExtractorDocumentNode>
#include <KItinerary/ExtractorPostprocessor>
#include <KItinerary/File>
#include <KItinerary/JsonLdDocument>
#include <KItinerary/Place>
#include <KItinerary/Reservation>

#include <KPkPass/Pass>

#include <KAboutData>
#include <KLocalizedString>

#include <QBuffer>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QScopedValueRollback>
#include <QStandardPaths>
#include <QUuid>
#include <QUrl>
#include <QUrlQuery>

#include <KMime/Message>
#include <KMime/Types>

#ifdef Q_OS_ANDROID
#include "android/itineraryactivity.h"

#include <kandroidextras/activity.h>
#include <kandroidextras/contentresolver.h>
#include <kandroidextras/intent.h>
#include <kandroidextras/jniarray.h>
#include <kandroidextras/jnisignature.h>
#include <kandroidextras/jnitypes.h>
#include <kandroidextras/uri.h>
#endif

#include <memory>

using namespace KItinerary;

#ifdef Q_OS_ANDROID

static void importFromIntent(JNIEnv *env, jobject that, jobject data)
{
    Q_UNUSED(that)
    Q_UNUSED(env)
    ApplicationController::instance()->importFromIntent(KAndroidExtras::Jni::fromHandle<KAndroidExtras::Intent>(data));
}

static const JNINativeMethod methods[] = {
    {"importFromIntent", "(Landroid/content/Intent;)V", (void*)importFromIntent},
};

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void*)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    JNIEnv *env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        qCWarning(Log) << "Failed to get JNI environment.";
        return -1;
    }
    jclass cls = env->FindClass(KAndroidExtras::Jni::typeName<ItineraryActivity>());
    if (env->RegisterNatives(cls, methods, sizeof(methods) / sizeof(JNINativeMethod)) < 0) {
        qCWarning(Log) << "Failed to register native functions.";
        return -1;
    }

    return JNI_VERSION_1_4;
}
#endif

ApplicationController* ApplicationController::s_instance = nullptr;

ApplicationController::ApplicationController(QObject* parent)
    : QObject(parent)
{
    s_instance = this;

    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &ApplicationController::clipboardContentChanged);
}

ApplicationController::~ApplicationController()
{
    s_instance = nullptr;
}

ApplicationController* ApplicationController::instance()
{
    return s_instance;
}

void ApplicationController::requestOpenPage(const QString &page)
{
    Q_EMIT openPageRequested(page);
}

void ApplicationController::setReservationManager(ReservationManager* resMgr)
{
    m_resMgr = resMgr;
}

void ApplicationController::setPkPassManager(PkPassManager* pkPassMgr)
{
    m_pkPassMgr = pkPassMgr;
    connect(m_pkPassMgr, &PkPassManager::passUpdated, this, &ApplicationController::importPass);
}

void ApplicationController::setDocumentManager(DocumentManager* docMgr)
{
    m_docMgr = docMgr;
}

void ApplicationController::setTransferManager(TransferManager* transferMgr)
{
    m_transferMgr = transferMgr;
}

void ApplicationController::setFavoriteLocationModel(FavoriteLocationModel *favLocModel)
{
    m_favLocModel = favLocModel;
}

void ApplicationController::setLiveDataManager(LiveDataManager *liveDataMgr)
{
    m_liveDataMgr = liveDataMgr;
}

void ApplicationController::setTripGroupManager(TripGroupManager *tripGroupMgr)
{
    m_tripGroupMgr = tripGroupMgr;
}

void ApplicationController::setPassManager(PassManager *passMgr)
{
    m_passMgr = passMgr;
}

void ApplicationController::setHealthCertificateManager(HealthCertificateManager *healthCertMgr)
{
    m_healthCertMgr = healthCertMgr;
}

#ifdef Q_OS_ANDROID
void ApplicationController::importFromIntent(const KAndroidExtras::Intent &intent)
{
    using namespace KAndroidExtras;
    const auto action = intent.getAction();

    // main entry point, nothing to do
    if (action == Intent::ACTION_MAIN) {
        return;
    }

    // opening a URL, can be something to import or a shortcut path
    if (action == Intent::ACTION_VIEW) {
        const QUrl url = intent.getData();
        if (url.scheme() == QLatin1String("page")) {
            qCDebug(Log) << url;
            requestOpenPage(url.path().mid(1));
        } else {
            importFromUrl(intent.getData());
        }
        return;
    }

    // shared data, e.g. from email applications like FairMail
    if (action == Intent::ACTION_SEND || action == Intent::ACTION_SEND_MULTIPLE) {
        const QString type = intent.getType();
        const auto subject = intent.getStringExtra(Intent::EXTRA_SUBJECT);
        const auto from = intent.getStringArrayExtra(Intent::EXTRA_EMAIL);
        const auto text = intent.getStringExtra(Intent::EXTRA_TEXT);
        qCInfo(Log) << action << type << subject << from << text;
        const QStringList attachments = ItineraryActivity().attachmentsForIntent(intent);
        qCInfo(Log) << attachments;

        KMime::Message msg;
        msg.subject()->fromUnicodeString(subject, "utf-8");
        for (const auto &f : from) {
            KMime::Types::Mailbox mb;
            mb.fromUnicodeString(f);
            msg.from()->addAddress(mb);
        }

        if (attachments.empty()) {
            msg.contentType()->setMimeType(type.toUtf8());
            msg.setBody(text.toUtf8());
        } else {
            auto body = new KMime::Content;
            body->contentType()->setMimeType(type.toUtf8());
            body->setBody(text.toUtf8());
            msg.addContent(body);
            for (const auto &a : attachments) {
                QUrl attUrl(a);
                auto att = new KMime::Content;
                att->contentType()->setMimeType(ContentResolver::mimeType(attUrl).toUtf8());
                att->contentTransferEncoding()->setEncoding(KMime::Headers::CEbase64);
                att->contentTransferEncoding()->setDecoded(true);
                att->contentType()->setName(attUrl.fileName(), "utf-8");
                QFile f(a);
                if (!f.open(QFile::ReadOnly)) {
                    qCWarning(Log) << "Failed to open attachement:" << a << f.errorString();
                    continue;
                }
                att->setBody(f.readAll());
                msg.addContent(att);
            }
        }

        msg.assemble();
        qDebug().noquote() << msg.encodedContent();
        importMimeMessage(&msg);
        return;
    }

    qCInfo(Log) << "Unhandled intent action:" << action;
}
#endif

void ApplicationController::importFromClipboard()
{
    const auto md = QGuiApplication::clipboard()->mimeData();
    if (md->hasUrls()) {
        const auto urls = md->urls();
        for (const auto &url : urls) {
            importFromUrl(url);
        }
    }

    else if (md->hasText()) {
        const auto content = md->data(QLatin1String("text/plain"));

        // URL copied as plain text
        if (content.startsWith("https://") && content.size() < 256) {
            const QUrl url(QString::fromUtf8(content));
            if (url.isValid()) {
                importFromUrl(url);
                return;
            }
        }
        importData(content);
    }

    else if (md->hasFormat(QLatin1String("application/octet-stream"))) {
        importData(md->data(QLatin1String("application/octet-stream")));
    }
}

void ApplicationController::importFromUrl(const QUrl &url)
{
    if (!url.isValid()) {
        return;
    }

    qCDebug(Log) << url;
    if (FileHelper::isLocalFile(url)) {
        importLocalFile(url);
        return;
    }

    if (url.scheme().startsWith(QLatin1String("http"))) {
        if (!m_nam ) {
            m_nam = new QNetworkAccessManager(this);
        }
        auto reqUrl(url);
        reqUrl.setScheme(QLatin1String("https"));
        QNetworkRequest req(reqUrl);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        auto reply = m_nam->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() != QNetworkReply::NoError) {
                qCDebug(Log) << reply->url() << reply->errorString();
                Q_EMIT infoMessage(i18n("Download failed: %1", reply->errorString()));
                return;
            }
            importData(reply->readAll());
        });
        return;
    }

    qCDebug(Log) << "Unhandled URL type:" << url;
}

void ApplicationController::importLocalFile(const QUrl &url)
{
    qCDebug(Log) << url;
    if (url.isEmpty()) {
        return;
    }

    QFile f(FileHelper::toLocalFile(url));
    if (!f.open(QFile::ReadOnly)) {
        qCWarning(Log) << "Failed to open" << f.fileName() << f.errorString();
        Q_EMIT infoMessage(i18n("Import failed: %1", f.errorString()));
        return;
    }
    if (f.size() > 4000000 && !f.fileName().endsWith(QLatin1String(".itinerary"))) {
        qCWarning(Log) << "File too large, ignoring" << f.fileName() << f.size();
        Q_EMIT infoMessage(i18n("Import failed: File too large."));
        return;
    }

    // deal with things we can import more efficiently from a file directly
    const auto head = f.peek(4);
    if (FileHelper::hasZipHeader(head)) {
        if (url.fileName().endsWith(QLatin1String(".itinerary"), Qt::CaseInsensitive) && importBundle(url)) {
            return;
        }
    }

    importData(f.readAll(), f.fileName());
}

bool ApplicationController::importData(const QByteArray &data, const QString &fileName)
{
    qCDebug(Log);
    if (data.size() < 4) {
        return false;
    }

    if (FileHelper::hasZipHeader(data)) {
        if (importBundle(data)) {
            return true;
        }
    }

    bool success = false, healthCertImported = false;
    using namespace KItinerary;
    ExtractorEngine engine;
    // user opened the file, so we can be reasonably sure they assume it contains
    // relevant content, so try expensive extraction methods too
    engine.setHints(ExtractorEngine::ExtractFullPageRasterImages);
#if KITINERARY_VERSION >= QT_VERSION_CHECK(5, 22, 41)
    engine.setHints(engine.hints() | ExtractorEngine::ExtractGenericIcalEvents);
#endif
    engine.setContextDate(QDateTime(QDate::currentDate(), QTime(0, 0)));
    engine.setData(data, fileName);
    const auto extractorResult = JsonLdDocument::fromJson(engine.extract());
    const auto resIds = m_resMgr->importReservations(extractorResult);
    if (!resIds.isEmpty()) {
        // check if there is a document we want to attach here
        QMimeDatabase db;
        const auto mt = db.mimeTypeForFileNameAndData(fileName, data);
        if (mt.name() == QLatin1String("application/pdf")) { // TODO support more file types (however we certainly want to exclude pkpass and json here)
            DigitalDocument docInfo;
            docInfo.setName(fileName);
            docInfo.setEncodingFormat(mt.name());
            const auto docId = DocumentUtil::idForContent(data);
            m_docMgr->addDocument(docId, docInfo, data);

            for (const auto &resId : resIds) {
                auto res = m_resMgr->reservation(resId);
                if (DocumentUtil::addDocumentId(res, docId)) {
                    m_resMgr->updateReservation(resId, res);
                }
            }
        }

        // collect pkpass files from the document tree
        QScopedValueRollback importLocker(m_importLock, true);
        importNode(engine.rootDocumentNode());

        Q_EMIT infoMessage(i18np("One reservation imported.", "%1 reservations imported.", resIds.size()));
        success = true;
    }

    // look for health certificate barcodes instead
    // if we don't find anything, try to import as health certificate directly
    if (importHealthCertificateRecursive(engine.rootDocumentNode()) || m_healthCertMgr->importCertificate(data)) {
        Q_EMIT infoMessage(i18n("Health certificate imported."));
        healthCertImported = true;
    }

    // look for time-less passes/program memberships/etc
    if (m_passMgr->import(extractorResult) || (resIds.isEmpty() && !healthCertImported && importGenericPkPass(engine.rootDocumentNode()))) {
        Q_EMIT infoMessage(i18n("Pass imported."));
        success = true;
    }

    // check for things we can add reservations for manually
    if (!success && !extractorResult.isEmpty()) {
        ExtractorPostprocessor postProc;
        postProc.setValidationEnabled(false);
        postProc.process(extractorResult);
        const auto postProcssedResult = postProc.result();
        ExtractorValidator validator;
        validator.setAcceptedTypes<LodgingBusiness, FoodEstablishment, LocalBusiness>();
        validator.setAcceptOnlyCompleteElements(true);
        for (const auto &resFor : postProcssedResult) {
            if (!validator.isValidElement(resFor)) {
                continue;
            }
            if (JsonLd::isA<LodgingBusiness>(resFor)) {
                LodgingReservation res;
                res.setReservationFor(resFor);
                Q_EMIT editNewHotelReservation(res);
                success = true;
                break;
            } else if (JsonLd::isA<FoodEstablishment>(resFor) || JsonLd::isA<LocalBusiness>(resFor)) {
                // LocalBusiness is frequently used for restaurants in website annotations
                FoodEstablishmentReservation res;
                res.setReservationFor(resFor);
                Q_EMIT editNewRestaurantReservation(res);
                success = true;
                break;
            }
        }
    }

    // nothing found
    if (!success && !healthCertImported) {
        Q_EMIT infoMessage(i18n("Nothing imported."));
    }
    return success || healthCertImported;
}

bool ApplicationController::importText(const QString& text)
{
    return importData(text.toUtf8());
}

void ApplicationController::importNode(const KItinerary::ExtractorDocumentNode &node)
{
    if (node.mimeType() == QLatin1String("application/vnd.apple.pkpass")) {
        const auto pass = node.content<KPkPass::Pass*>();
        // ### could we pass along pass directly here to avoid the extra parsing roundtrip?
        m_pkPassMgr->importPassFromData(pass->rawData());
    }

    for (const auto &child : node.childNodes()) {
        importNode(child);
    }
}

bool ApplicationController::importGenericPkPass(const KItinerary::ExtractorDocumentNode &node)
{
    if (node.mimeType() == QLatin1String("application/vnd.apple.pkpass")) {
        const auto pass = node.content<KPkPass::Pass*>();
        if (pass->type() == KPkPass::Pass::Coupon || pass->type() == KPkPass::Pass::StoreCard) {
            // no support for displaying those yet
            return false;
        }

        GenericPkPass wrapper;
        wrapper.setName(pass->description());
        wrapper.setPkpassPassTypeIdentifier(pass->passTypeIdentifier());
        wrapper.setPkpassSerialNumber(pass->serialNumber());
        wrapper.setValidUntil(pass->expirationDate());

        QScopedValueRollback importLocker(m_importLock, true);
        m_pkPassMgr->importPassFromData(pass->rawData());
        m_passMgr->import(wrapper);

        return true;
    }

    bool res = false;
    for (const auto &child : node.childNodes()) {
        res |= importGenericPkPass(child);
    }
    return res;
}

bool ApplicationController::hasClipboardContent() const
{
    const auto md = QGuiApplication::clipboard()->mimeData();
    return md->hasText() || md->hasUrls() || md->hasFormat(QLatin1String("application/octet-stream"));
}


void ApplicationController::exportToFile(const QUrl &url)
{
    if (url.isEmpty()) {
        return;
    }

    qCDebug(Log) << url;
    File f(FileHelper::toLocalFile(url));
    if (!f.open(File::Write)) {
        qCWarning(Log) << f.errorString();
        Q_EMIT infoMessage(i18n("Export failed: %1", f.errorString()));
        return;
    }

    Exporter exporter(&f);
    exporter.exportReservations(m_resMgr);
    exporter.exportPasses(m_pkPassMgr);
    exporter.exportDocuments(m_docMgr);
    exporter.exportFavoriteLocations(m_favLocModel);
    exporter.exportTransfers(m_resMgr, m_transferMgr);
    exporter.exportPasses(m_passMgr);
    exporter.exportHealthCertificates(m_healthCertMgr);
    exporter.exportLiveData();
    exporter.exportSettings();
    Q_EMIT infoMessage(i18n("Export completed."));
}

void ApplicationController::exportTripToGpx(const QString &tripGroupId, const QUrl &url)
{
    if (url.isEmpty()) {
        return;
    }

    QFile f(FileHelper::toLocalFile(url));
    if (!f.open(QFile::WriteOnly)) {
        qCWarning(Log) << f.errorString() << f.fileName();
        Q_EMIT infoMessage(i18n("Export failed: %1", f.errorString()));
        return;
    }
    GpxExport exporter(&f);

    const auto tg = m_tripGroupMgr->tripGroup(tripGroupId);
    const auto batches = tg.elements();
    for (const auto &batchId : batches) {
        const auto res = m_resMgr->reservation(batchId);
        const auto transferBefore = m_transferMgr->transfer(batchId, Transfer::Before);
        const auto transferAfter = m_transferMgr->transfer(batchId, Transfer::After);
        exporter.writeReservation(res, m_liveDataMgr->journey(batchId), transferBefore, transferAfter);
    }
    Q_EMIT infoMessage(i18n("Export completed."));
}

bool ApplicationController::importBundle(const QUrl &url)
{
    KItinerary::File f(FileHelper::toLocalFile(url));
    if (!f.open(File::Read)) {
        qCWarning(Log) << "Failed to open bundle file:" << url << f.errorString();
        Q_EMIT infoMessage(i18n("Import failed: %1", f.errorString()));
        return false;
    }

    return importBundle(&f);
}

bool ApplicationController::importBundle(const QByteArray &data)
{
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QBuffer::ReadOnly);
    KItinerary::File f(&buffer);
    if (!f.open(File::Read)) {
        qCWarning(Log) << "Failed to open bundle data:" << f.errorString();
        Q_EMIT infoMessage(i18n("Import failed: %1", f.errorString()));
        return false;
    }

    return importBundle(&f);
}

bool ApplicationController::importBundle(KItinerary::File *file)
{
    Importer importer(file);
    int count = 0;
    {
        QSignalBlocker blocker(this); // suppress infoMessage()
        count += importer.importReservations(m_resMgr);
        count += importer.importPasses(m_pkPassMgr);
        count += importer.importDocuments(m_docMgr);
        count += importer.importFavoriteLocations(m_favLocModel);
        count += importer.importTransfers(m_resMgr, m_transferMgr);
        count += importer.importPasses(m_passMgr);
        count += importer.importHealthCertificates(m_healthCertMgr);
        count += importer.importLiveData(m_liveDataMgr);
        count += importer.importSettings();
    }

    if (count > 0) {
        Q_EMIT infoMessage(i18n("Import completed."));
    }
    return count > 0;
}

bool ApplicationController::importHealthCertificateRecursive(const ExtractorDocumentNode &node)
{
    if (node.childNodes().size() == 1 && (node.mimeType() == QLatin1String("internal/qimage") || node.mimeType() == QLatin1String("application/vnd.apple.pkpass"))) {
        const auto &child = node.childNodes()[0];
        if (child.isA<QString>()) {
            return m_healthCertMgr->importCertificate(child.content<QString>().toUtf8());
        }
        if (child.isA<QByteArray>()) {
            return m_healthCertMgr->importCertificate(child.content<QByteArray>());
        }
    }

    bool result = false;
    for (const auto &child : node.childNodes()) {
        if (importHealthCertificateRecursive(child)) { // no shortcut evaluation, more than one QR code per PDF is possible
            result = true;
        }
    }
    return result;
}

void ApplicationController::importPass(const QString &passId)
{
    if (m_importLock) {
        return;
    }

    const auto pass = m_pkPassMgr->pass(passId);
    KItinerary::ExtractorEngine engine;
    engine.setContent(QVariant::fromValue<KPkPass::Pass*>(pass), u"application/vnd.apple.pkpass");
    const auto resIds = m_resMgr->importReservations(JsonLdDocument::fromJson(engine.extract()));
    if (!resIds.isEmpty()) {
        Q_EMIT infoMessage(i18np("One reservation imported.", "%1 reservations imported.", resIds.size()));
    }
}

void ApplicationController::importMimeMessage(KMime::Message *msg)
{
    ExtractorEngine engine;
    engine.setContent(QVariant::fromValue<KMime::Content*>(msg), u"message/rfc822");
    const auto resIds = m_resMgr->importReservations(JsonLdDocument::fromJson(engine.extract()));
    if (!resIds.isEmpty()) {
        Q_EMIT infoMessage(i18np("One reservation imported.", "%1 reservations imported.", resIds.size()));
    }
}

void ApplicationController::addDocument(const QString &batchId, const QUrl &url)
{
    if (!url.isValid()) {
        return;
    }

    const auto docId = QUuid::createUuid().toString();
    auto res = m_resMgr->reservation(batchId);
    DocumentUtil::addDocumentId(res, docId);

    DigitalDocument docInfo;
#ifdef Q_OS_ANDROID
    docInfo.setEncodingFormat(KAndroidExtras::ContentResolver::mimeType(url));
    docInfo.setName(KAndroidExtras::ContentResolver::fileName(url));
#else
    QMimeDatabase db;
    docInfo.setEncodingFormat(db.mimeTypeForFile(FileHelper::toLocalFile(url)).name());
    docInfo.setName(url.fileName());
#endif

    m_docMgr->addDocument(docId, docInfo, FileHelper::toLocalFile(url));

    const auto resIds = m_resMgr->reservationsForBatch(batchId);
    for (const auto &resId : resIds) {
        m_resMgr->updateReservation(resId, res);
    }
}

void ApplicationController::removeDocument(const QString &batchId, const QString &docId)
{
    const auto resIds = m_resMgr->reservationsForBatch(batchId);
    for (const auto &resId : resIds) {
        auto res = m_resMgr->reservation(batchId);
        if (DocumentUtil::removeDocumentId(res, docId)) {
            m_resMgr->updateReservation(resId, res);
        }
    }
    m_docMgr->removeDocument(docId);
}

void ApplicationController::openDocument(const QUrl &url)
{
#ifdef Q_OS_ANDROID
    using namespace KAndroidExtras;
    auto uri = ItineraryActivity().openDocument(url.toLocalFile());

    Intent intent;
    intent.setData(uri);
    intent.setAction(Intent::ACTION_VIEW);
    intent.addFlags(Intent::FLAG_GRANT_READ_URI_PERMISSION);
    Activity::startActivity(intent, 0);
#else
    QDesktopServices::openUrl(url);
#endif
}

QString ApplicationController::applicationVersion() const
{
    return QString::fromUtf8(ITINERARY_DETAILED_VERSION_STRING);
}

QString ApplicationController::extractorCapabilities() const
{
    return ExtractorCapabilities::capabilitiesString();
}

QVariant ApplicationController::aboutData() const
{
    return QVariant::fromValue(KAboutData::applicationData());
}

QString ApplicationController::userAgent() const
{
    return QLatin1String("org.kde.itinerary/") + QCoreApplication::applicationVersion() + QLatin1String(" (kde-pim@kde.org)");
}

#include "moc_applicationcontroller.cpp"
