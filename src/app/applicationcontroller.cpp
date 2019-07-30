/*
    Copyright (C) 2018 Volker Krause <vkrause@kde.org>

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

#include "applicationcontroller.h"
#include "documentmanager.h"
#include "logging.h"
#include "pkpassmanager.h"
#include "reservationmanager.h"

#include <KItinerary/CreativeWork>
#include <KItinerary/File>
#include <KItinerary/JsonLdDocument>

#include <KLocalizedString>

#include <QBuffer>
#include <QClipboard>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeData>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QUuid>
#include <QUrl>
#include <QUrlQuery>

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
#else
#include <QFileDialog>
#endif

#include <memory>

using namespace KItinerary;

#ifdef Q_OS_ANDROID

// ### obsolete with Qt 5.13
class ActivityResultReceiver : public QAndroidActivityResultReceiver
{
public:
    explicit inline ActivityResultReceiver(const std::function<void (int, int, const QAndroidJniObject&)> &callback)
        : m_callback(callback)
    {}
    inline void handleActivityResult(int receiverRequestCode, int resultCode, const QAndroidJniObject &intent) override
    {
        m_callback(receiverRequestCode, resultCode, intent);
        delete this;
    }
private:
    std::function<void(int, int, const QAndroidJniObject&)> m_callback;
};

#define PERMISSION_CALENDAR QStringLiteral("android.permission.READ_CALENDAR")

static void importReservation(JNIEnv *env, jobject that, jstring data)
{
    Q_UNUSED(that);
    const char *str = env->GetStringUTFChars(data, nullptr);
    ApplicationController::instance()->importData(str);
    env->ReleaseStringUTFChars(data, str);
}

static void importDavDroidJson(JNIEnv *env, jobject that, jstring data)
{
    Q_UNUSED(that);
    const char *str = env->GetStringUTFChars(data, nullptr);
    const auto doc = QJsonDocument::fromJson(str);
    env->ReleaseStringUTFChars(data, str);

    const auto array = doc.array();
    if (array.size() < 2 || array.at(0).toString() != QLatin1String("X-KDE-KITINERARY-RESERVATION")) {
        return;
    }

    ApplicationController::instance()->importData(array.at(1).toString().toUtf8());
}

static void importFromIntent(JNIEnv *env, jobject that, jobject data)
{
    Q_UNUSED(that);
    Q_UNUSED(env)
    ApplicationController::instance()->importFromIntent(data);
}

static const JNINativeMethod methods[] = {
    {"importReservation", "(Ljava/lang/String;)V", (void*)importReservation},
    {"importFromIntent", "(Landroid/content/Intent;)V", (void*)importFromIntent},
    {"importDavDroidJson", "(Ljava/lang/String;)V", (void*)importDavDroidJson}
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
    jclass cls = env->FindClass("org/kde/itinerary/Activity");
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

void ApplicationController::setReservationManager(ReservationManager* resMgr)
{
    m_resMgr = resMgr;
}

void ApplicationController::setPkPassManager(PkPassManager* pkPassMgr)
{
    m_pkPassMgr = pkPassMgr;
}

void ApplicationController::setDocumentManager(DocumentManager* docMgr)
{
    m_docMgr = docMgr;
}

#ifdef Q_OS_ANDROID
void ApplicationController::importFromIntent(const QAndroidJniObject &intent)
{
    if (!intent.isValid()) {
        return;
    }

    const auto uri = intent.callObjectMethod("getData", "()Landroid/net/Uri;");
    if (!uri.isValid()) {
        return;
    }

    const auto uriStr = uri.callObjectMethod("toString", "()Ljava/lang/String;");
    importFromUrl(QUrl(uriStr.toString()));
}
#endif

void ApplicationController::showImportFileDialog()
{
#ifdef Q_OS_ANDROID
    const auto ACTION_OPEN_DOCUMENT = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent", "ACTION_OPEN_DOCUMENT");
    QAndroidJniObject intent("android/content/Intent", "(Ljava/lang/String;)V", ACTION_OPEN_DOCUMENT.object());
    const auto CATEGORY_OPENABLE = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent", "CATEGORY_OPENABLE");
    intent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", CATEGORY_OPENABLE.object());
    intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString(QStringLiteral("*/*")).object());
    QtAndroid::startActivity(intent, 0, new ActivityResultReceiver([this](int, int, const QAndroidJniObject &intent) { importFromIntent(intent); }));
#else
    const auto url = QFileDialog::getOpenFileUrl(nullptr, i18n("Import Reservation"),
        QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
        i18n("All Files (*.*);;PkPass files (*.pkpass);;PDF files (*.pdf);;KDE Itinerary files (*.itinerary)"));
    if (url.isValid()) {
        importFromUrl(url);
    }
#endif
}

void ApplicationController::importFromClipboard()
{
    if (QGuiApplication::clipboard()->mimeData()->hasUrls()) {
        const auto urls = QGuiApplication::clipboard()->mimeData()->urls();
        for (const auto &url : urls)
            importFromUrl(url);
        return;
    }

    if (QGuiApplication::clipboard()->mimeData()->hasText()) {
        const auto content = QGuiApplication::clipboard()->mimeData()->data(QLatin1String("text/plain"));
        importData(content);
    }
}

void ApplicationController::importFromUrl(const QUrl &url)
{
    qCDebug(Log) << url;
    if (url.isLocalFile() || url.scheme() == QLatin1String("content")) {
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

    QFile f(url.isLocalFile() ? url.toLocalFile() : url.toString());
    if (!f.open(QFile::ReadOnly)) {
        qCWarning(Log) << "Failed to open" << f.fileName() << f.errorString();
        return;
    }
    if (f.size() > 4000000) {
        qCWarning(Log) << "File too large, ignoring" << f.fileName() << f.size();
        return;
    }

    const auto head = f.peek(4);
    if (url.fileName().endsWith(QLatin1String(".pkpass"), Qt::CaseInsensitive)) {
        m_pkPassMgr->importPass(url);
    } else if (url.fileName().endsWith(QLatin1String(".itinerary"), Qt::CaseInsensitive)) {
        importBundle(url);
    } else if (strncmp(head.constData(), "PK\x03\x04", 4) == 0) {
        if (m_pkPassMgr->importPass(url).isEmpty()) {
            importBundle(url);
        }
    } else {
        m_resMgr->importReservation(f.readAll(), f.fileName());
    }
}

void ApplicationController::importData(const QByteArray &data)
{
    qCDebug(Log);
    if (data.size() < 4) {
        return;
    }
    if (strncmp(data.constData(), "PK\x03\x04", 4) == 0) {
        if (m_pkPassMgr->importPassFromData(data).isEmpty()) {
            importBundle(data);
        }
    } else {
        m_resMgr->importReservation(data);
    }
}

void ApplicationController::checkCalendar()
{
#ifdef Q_OS_ANDROID

    if (QtAndroid::checkPermission(PERMISSION_CALENDAR) == QtAndroid::PermissionResult::Granted) {
        const auto activity = QtAndroid::androidActivity();
        if (activity.isValid()) {
            activity.callMethod<void>("checkCalendar");
        }
    } else {
        QtAndroid::requestPermissions({PERMISSION_CALENDAR}, [this] (const QtAndroid::PermissionResultMap &result){
            if (result[PERMISSION_CALENDAR] == QtAndroid::PermissionResult::Granted) {
                checkCalendar();
            }
        });
    }
#endif
}

bool ApplicationController::hasClipboardContent() const
{
    return QGuiApplication::clipboard()->mimeData()->hasText() || QGuiApplication::clipboard()->mimeData()->hasUrls();
}

void ApplicationController::exportData()
{
    qCDebug(Log);
#ifdef Q_OS_ANDROID
    const auto ACTION_OPEN_DOCUMENT = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent", "ACTION_CREATE_DOCUMENT");
    QAndroidJniObject intent("android/content/Intent", "(Ljava/lang/String;)V", ACTION_OPEN_DOCUMENT.object());
    const auto CATEGORY_OPENABLE = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Intent", "CATEGORY_OPENABLE");
    intent.callObjectMethod("addCategory", "(Ljava/lang/String;)Landroid/content/Intent;", CATEGORY_OPENABLE.object());
    intent.callObjectMethod("setType", "(Ljava/lang/String;)Landroid/content/Intent;", QAndroidJniObject::fromString(QStringLiteral("*/*")).object());
    QtAndroid::startActivity(intent, 0, new ActivityResultReceiver([this](int, int, const QAndroidJniObject &intent) { exportToIntent(intent); }));
#else
    const auto filePath = QFileDialog::getSaveFileName(nullptr, i18n("Export Itinerary Data"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        i18n("KDE Itinerary files (*.itinerary)"));
    exportToFile(filePath);
#endif
}

#ifdef Q_OS_ANDROID
void ApplicationController::exportToIntent(const QAndroidJniObject &intent)
{
    if (!intent.isValid()) {
        return;
    }

    const auto uri = intent.callObjectMethod("getData", "()Landroid/net/Uri;");
    if (!uri.isValid()) {
        return;
    }

    const auto uriStr = uri.callObjectMethod("toString", "()Ljava/lang/String;");
    exportToFile(uriStr.toString());
}
#endif

void ApplicationController::exportToFile(const QString &filePath)
{
    qCDebug(Log) << filePath;

    File f(filePath);
    if (!f.open(File::Write)) {
        qCWarning(Log) << f.errorString();
        // TODO show error in UI
        return;
    }

    // export reservation data
    for (const auto &batchId : m_resMgr->batches()) {
        const auto resIds = m_resMgr->reservationsForBatch(batchId);
        for (const auto &resId : resIds) {
            f.addReservation(resId, m_resMgr->reservation(resId));
        }
    }

    // export passes
    const auto passIds = m_pkPassMgr->passes();
    for (const auto &passId : passIds) {
        f.addPass(passId, m_pkPassMgr->rawData(passId));
    }

    // export documents
    const auto docIds = m_docMgr->documents();
    for (const auto &docId : docIds) {
        const auto fileName = m_docMgr->documentFilePath(docId);
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly)) {
            qCWarning(Log) << "failed to open" << fileName << "for exporting" << file.errorString();
            continue;
        }
        f.addDocument(docId, m_docMgr->documentInfo(docId), file.readAll());
    }

    // TODO export settings
}

void ApplicationController::importBundle(const QUrl &url)
{
    KItinerary::File f(url.isLocalFile() ? url.toLocalFile() : url.toString());
    if (!f.open(File::Read)) {
        // TODO show error in the ui
        qCWarning(Log) << "Failed to open bundle file:" << url << f.errorString();
        return;
    }

    importBundle(&f);
}

void ApplicationController::importBundle(const QByteArray &data)
{
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QBuffer::ReadOnly);
    KItinerary::File f(&buffer);
    if (!f.open(File::Read)) {
        // TODO show error in the ui
        qCWarning(Log) << "Failed to open bundle data:" << f.errorString();
        return;
    }

    importBundle(&f);
}

void ApplicationController::importBundle(KItinerary::File *file)
{
    const auto resIds = file->reservations();
    for (const auto &resId : resIds) {
        m_resMgr->addReservation(file->reservation(resId));
    }

    const auto passIds = file->passes();
    for (const auto &passId : passIds) {
        m_pkPassMgr->importPassFromData(file->passData(passId));
    }

    const auto docIds = file->documents();
    for (const auto &docId : docIds) {
        m_docMgr->addDocument(docId, file->documentInfo(docId), file->documentData(docId));
    }
}

void ApplicationController::addDocument(const QString &batchId)
{
    // TODO Android support
#ifndef Q_OS_ANDROID
    const auto url = QFileDialog::getOpenFileUrl(nullptr, i18n("Add Document"),
        QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)),
        i18n("All Files (*.*)"));
    if (url.isValid()) {
        const auto docId = QUuid::createUuid().toString();
        auto res = m_resMgr->reservation(batchId);
        auto docList = JsonLdDocument::readProperty(res, "subjectOf").toList();
        docList.push_back(docId);
        JsonLdDocument::writeProperty(res, "subjectOf", docList);

        DigitalDocument docInfo;
        docInfo.setName(url.fileName());

        QMimeDatabase db;
        docInfo.setEncodingFormat(db.mimeTypeForFile(url.isLocalFile() ? url.toLocalFile() : url.toString()).name());

        m_docMgr->addDocument(docId, docInfo, url.isLocalFile() ? url.toLocalFile() : url.toString());
        m_resMgr->updateReservation(batchId, res); // TODO attach to all elements in the batch?
    }
#endif
}

void ApplicationController::removeDocument(const QString &batchId, const QString &docId)
{
    auto res = m_resMgr->reservation(batchId);
    auto docList = JsonLdDocument::readProperty(res, "subjectOf").toList();
    docList.removeAll(docId);
    JsonLdDocument::writeProperty(res, "subjectOf", docList);

    m_resMgr->updateReservation(batchId, res); // TODO update all reservations in the batch
    m_docMgr->removeDocument(docId);
}
