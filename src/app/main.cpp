/*
    SPDX-FileCopyrightText: 2018 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "config-itinerary.h"
#include "itinerary_version.h"
#include "logging.h"

#include "applicationcontroller.h"
#include "calendarimportmodel.h"
#include "countrysubdivisionmodel.h"
#include "clipboard.h"
#include "developmentmodecontroller.h"
#include "documentmanager.h"
#include "documentsmodel.h"
#include "factory.h"
#include "favoritelocationmodel.h"
#include "genericpkpass.h"
#include "healthcertificatemanager.h"
#include "journeysectionmodel.h"
#include "kdeconnect.h"
#include "livedatamanager.h"
#include "localizer.h"
#include "locationinformation.h"
#include "mapdownloadmanager.h"
#include "matrixcontroller.h"
#include "navigationcontroller.h"
#include "notificationconfigcontroller.h"
#include "onlineticketimporter.h"
#include "passmanager.h"
#include "pkpassmanager.h"
#include "permissionmanager.h"
#include "pkpassimageprovider.h"
#include "publictransport.h"
#include "reservationmanager.h"
#include "settings.h"
#include "statisticsmodel.h"
#include "statisticstimerangemodel.h"
#include "tickettokenmodel.h"
#include "timelinedelegatecontroller.h"
#include "timelinemodel.h"
#include "timelinesectiondelegatecontroller.h"
#include "transferdelegatecontroller.h"
#include "transfermanager.h"
#include "tripgroupinfoprovider.h"
#include "tripgroupmanager.h"
#include "tripgroupproxymodel.h"
#include "unitconversion.h"
#include "util.h"
#include "weatherforecastmodel.h"

#if HAVE_MATRIX
#include "matrix/matrixroomsmodel.h"
#include "matrix/matrixbeacon.h"
#endif

#include <weatherforecastmanager.h>

#include <calendarlistmodel.h>
#include <calendarpluginloader.h>

#include <KItinerary/CountryDb>
#include <KItinerary/JsonLdDocument>
#include <KItinerary/Ticket>
#include <kitinerary_version.h>
#if KITINERARY_VERSION >= QT_VERSION_CHECK(5, 24, 41)
#include <KItinerary/PriceUtil>
#endif

#include <KPkPass/Field>
#include <KPkPass/Barcode>
#include <KPkPass/BoardingPass>

#ifndef Q_OS_ANDROID
#include <KDBusService>
#include <KWindowSystem>
#endif

#include <KLocalizedContext>
#include <KLocalizedString>

#include <KAboutData>
#if HAVE_KCRASH
#include <KCrash>
#endif
#include <KContacts/AddressFormat>

#include <QQuickStyle>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#ifdef Q_OS_ANDROID
#include <kandroidextras/activity.h>
#include <kandroidextras/intent.h>
#else
#include <QApplication>
#endif

#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QNetworkAccessManager>
#include <QStandardPaths>
#include <QWindow>

#if !HAVE_MATRIX
class MatrixBeaconStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant connection MEMBER m_connection)
    QVariant m_connection;
};
#endif

#if KITINERARY_VERSION <QT_VERSION_CHECK(5, 24, 41)
class DummyPriceUtil
{
    Q_GADGET
 public:
    Q_INVOKABLE static bool hasPrice(const QVariant&) { return false; }
    Q_INVOKABLE static bool canHavePrice(const QVariant&) { return false; }
    Q_INVOKABLE static double price(const QVariant&) { return 0.0; }
    Q_INVOKABLE static QString currency(const QVariant&) { return QString(); }
    Q_INVOKABLE static void setPrice(QVariant&, double, const QString&) {}
    Q_INVOKABLE static int decimalCount(QStringView) { return 2; }
};
#endif

void registerKContactsTypes()
{
    // ### this should move into a real QML plugin for KContacts
    qmlRegisterUncreatableMetaObject(KContacts::staticMetaObject, "org.kde.contacts", 1, 0, "KContacts", {});
    qmlRegisterSingletonType("org.kde.contacts", 1, 0, "AddressFormatRepository", [](QQmlEngine *, QJSEngine *jsEngine) -> QJSValue {
        return jsEngine->toScriptValue(KContacts::AddressFormatRepository());
    });
    qRegisterMetaType<KContacts::AddressFormat>();
    qRegisterMetaType<KContacts::AddressFormatElement>();
    qRegisterMetaType<KContacts::AddressFormatRepository>();
    qRegisterMetaType<KContacts::AddressFormatPreference>();
    qRegisterMetaType<KContacts::AddressFormatScriptPreference>();
}

void registerKCalendarCoreTypes()
{
    // ### this should move into a QML plugin of KCalendarCore
    qmlRegisterUncreatableMetaObject(KCalendarCore::staticMetaObject, "internal.org.kde.kcalendarcore", 1, 0, "KCalendarCore", {});
    qmlRegisterType<KCalendarCoreExtras::CalendarListModel>("internal.org.kde.kcalendarcore", 1, 0, "CalendarListModel");
    qmlRegisterSingletonType("internal.org.kde.kcalendarcore", 1, 0, "CalendarPluginLoader", [](QQmlEngine *, QJSEngine *jsEngine) -> QJSValue {
        return jsEngine->toScriptValue(KCalendarCoreExtras::CalendarPluginLoader());
    });
}

void registerKPkPassTypes()
{
    qmlRegisterUncreatableType<KPkPass::Barcode>("org.kde.pkpass", 1, 0, "Barcode", {});
    qmlRegisterUncreatableType<KPkPass::Field>("org.kde.pkpass", 1, 0, "Field", {});
    qmlRegisterUncreatableType<KPkPass::Pass>("org.kde.pkpass", 1, 0, "Pass", {});
    qmlRegisterUncreatableType<KPkPass::BoardingPass>("org.kde.pkpass", 1, 0, "BoardingPass", {});
}

void registerKItineraryTypes()
{
    qRegisterMetaType<KItinerary::KnowledgeDb::DrivingSide>();
    qmlRegisterUncreatableType<KItinerary::Ticket>("org.kde.kitinerary", 1, 0, "Ticket", {});
    qmlRegisterUncreatableMetaObject(KItinerary::KnowledgeDb::staticMetaObject, "org.kde.kitinerary", 1, 0, "KnowledgeDb", {});
    qmlRegisterSingletonType("org.kde.itinerary", 1, 0, "PriceUtil", [](QQmlEngine*, QJSEngine *engine) -> QJSValue {
#if KITINERARY_VERSION >= QT_VERSION_CHECK(5, 24, 41)
        return engine->toScriptValue(KItinerary::PriceUtil());
#else
        return engine->toScriptValue(DummyPriceUtil());
#endif
    });
}

void registerApplicationTypes()
{
    qRegisterMetaType<ReservationManager*>();
    qRegisterMetaType<Transfer::Alignment>();
    qRegisterMetaType<TripGroupManager*>();
    qRegisterMetaType<WeatherForecast>();
    qRegisterMetaType<Permission::Permission>();
    qRegisterMetaType<HealthCertificateManager*>();

    qmlRegisterUncreatableType<LocationInformation>("org.kde.itinerary", 1, 0, "LocationInformation", {});
    qmlRegisterUncreatableType<StatisticsItem>("org.kde.itinerary", 1, 0, "StatisticsItem", {});
    qmlRegisterUncreatableType<TimelineElement>("org.kde.itinerary", 1, 0, "TimelineElement", {});
    qmlRegisterUncreatableType<TimelineModel>("org.kde.itinerary", 1, 0, "TimelineModel", {});
    qmlRegisterUncreatableType<Transfer>("org.kde.itinerary", 1, 0, "Transfer", {});

    qmlRegisterUncreatableMetaObject(Permission::staticMetaObject, "org.kde.itinerary", 1, 0, "Permission", {});
#if HAVE_MATRIX
    qmlRegisterUncreatableMetaObject(MatrixRoomsModel::staticMetaObject, "org.kde.itinerary", 1, 0, "MatrixRoomsModel", {});
#endif

    qmlRegisterType<CalendarImportModel>("org.kde.itinerary", 1, 0, "CalendarImportModel");
    qmlRegisterType<CountrySubdivisionModel>("org.kde.itinerary", 1, 0, "CountrySubdivisionModel");
    qmlRegisterType<DocumentsModel>("org.kde.itinerary", 1, 0, "DocumentsModel");
    qmlRegisterType<JourneySectionModel>("org.kde.itinerary", 1, 0, "JourneySectionModel");
    qmlRegisterType<KDEConnectDeviceModel>("org.kde.itinerary", 1, 0, "KDEConnectDeviceModel");
    qmlRegisterType<StatisticsModel>("org.kde.itinerary", 1, 0, "StatisticsModel");
    qmlRegisterType<StatisticsTimeRangeModel>("org.kde.itinerary", 1, 0, "StatisticsTimeRangeModel");
    qmlRegisterType<TicketTokenModel>("org.kde.itinerary", 1, 0, "TicketTokenModel");
    qmlRegisterType<TimelineDelegateController>("org.kde.itinerary", 1, 0, "TimelineDelegateController");
    qmlRegisterType<TimelineSectionDelegateController>("org.kde.itinerary", 1, 0, "TimelineSectionDelegateController");
    qmlRegisterType<TransferDelegateController>("org.kde.itinerary", 1, 0, "TransferDelegateController");
    qmlRegisterType<WeatherForecastModel>("org.kde.itinerary", 1, 0, "WeatherForecastModel");
#if HAVE_MATRIX
    qmlRegisterType<MatrixBeacon>("org.kde.itinerary", 1, 0, "MatrixBeacon");
#else
    qmlRegisterType<MatrixBeaconStub>("org.kde.itinerary", 1, 0, "MatrixBeacon");
#endif
    qmlRegisterType<OnlineTicketImporter>("org.kde.itinerary", 1, 0, "OnlineTicketImporter");

    qmlRegisterSingletonType("org.kde.itinerary", 1, 0, "About", [](QQmlEngine *engine, QJSEngine *) -> QJSValue {
        return engine->toScriptValue(KAboutData::applicationData());
    });

    qmlRegisterSingletonType<Clipboard>("org.kde.itinerary", 1, 0, "Clipboard", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return new Clipboard;
    });
}

// for registering QML singletons only
static ReservationManager *s_reservationManager = nullptr;
static DocumentManager *s_documentManager = nullptr;
static FavoriteLocationModel *s_favoriteLocationModel = nullptr;
static PkPassManager *s_pkPassManager = nullptr;
static Settings *s_settings = nullptr;
static TransferManager *s_tranferManager = nullptr;
static TripGroupManager *s_tripGroupManager = nullptr;
static LiveDataManager *s_liveDataMnager = nullptr;
static WeatherForecastManager *s_weatherForecastManager = nullptr;
static TimelineModel *s_timelineModel = nullptr;
static TripGroupInfoProvider s_tripGroupInfoProvider;
static TripGroupProxyModel *s_tripGroupProxyModel = nullptr;
static MapDownloadManager *s_mapDownloadManager = nullptr;
static PassManager *s_passManager = nullptr;
static MatrixController *s_matrixController = nullptr;

#define REGISTER_SINGLETON_INSTANCE(Class, Instance) \
    qmlRegisterSingletonInstance<Class>("org.kde.itinerary", 1, 0, #Class, Instance);

#define REGISTER_SINGLETON_GADGET_INSTANCE(Class, Instance) \
    qmlRegisterSingletonType("org.kde.itinerary", 1, 0, #Class, [](QQmlEngine *engine, QJSEngine*) -> QJSValue { \
        return engine->toScriptValue(Instance); \
    });

#define REGISTER_SINGLETON_GADGET_FACTORY(Class) \
    qmlRegisterSingletonType("org.kde.itinerary", 1, 0, #Class, [](QQmlEngine*, QJSEngine *engine) -> QJSValue { \
        return engine->toScriptValue(Class()); \
    });

void registerApplicationSingletons()
{
    REGISTER_SINGLETON_INSTANCE(ApplicationController, ApplicationController::instance())
    REGISTER_SINGLETON_INSTANCE(ReservationManager, s_reservationManager)
    REGISTER_SINGLETON_INSTANCE(DocumentManager, s_documentManager)
    REGISTER_SINGLETON_INSTANCE(FavoriteLocationModel, s_favoriteLocationModel)
    REGISTER_SINGLETON_INSTANCE(PkPassManager, s_pkPassManager)
    REGISTER_SINGLETON_INSTANCE(Settings, s_settings)
    REGISTER_SINGLETON_INSTANCE(TransferManager, s_tranferManager)
    REGISTER_SINGLETON_INSTANCE(TripGroupManager, s_tripGroupManager)
    REGISTER_SINGLETON_INSTANCE(LiveDataManager, s_liveDataMnager)
    REGISTER_SINGLETON_INSTANCE(WeatherForecastManager, s_weatherForecastManager)
    REGISTER_SINGLETON_INSTANCE(TimelineModel, s_timelineModel)
    REGISTER_SINGLETON_INSTANCE(TripGroupProxyModel, s_tripGroupProxyModel)
    REGISTER_SINGLETON_INSTANCE(MapDownloadManager, s_mapDownloadManager)
    REGISTER_SINGLETON_INSTANCE(PassManager, s_passManager)
    REGISTER_SINGLETON_INSTANCE(MatrixController, s_matrixController);

    REGISTER_SINGLETON_GADGET_INSTANCE(TripGroupInfoProvider, s_tripGroupInfoProvider)

    REGISTER_SINGLETON_GADGET_FACTORY(DevelopmentModeController)
    REGISTER_SINGLETON_GADGET_FACTORY(Factory)
    REGISTER_SINGLETON_GADGET_FACTORY(Localizer)
    REGISTER_SINGLETON_GADGET_FACTORY(NavigationController)
    REGISTER_SINGLETON_GADGET_FACTORY(NotificationConfigController)
    REGISTER_SINGLETON_GADGET_FACTORY(PermissionManager)
    REGISTER_SINGLETON_GADGET_FACTORY(PublicTransport)
    REGISTER_SINGLETON_GADGET_FACTORY(UnitConversion)
    REGISTER_SINGLETON_GADGET_FACTORY(Util)
}

#undef REGISTER_SINGLETON_INSTANCE
#undef REGISTER_SINGLETON_GADGET_INSTANCE
#undef REGISTER_SINGLETON_GADGET_FACTORY

static QNetworkAccessManager *namFactory()
{
    static QNetworkAccessManager *s_nam = nullptr;
    if (!s_nam) {
        s_nam = new QNetworkAccessManager(QCoreApplication::instance());
        s_nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
        s_nam->setStrictTransportSecurityEnabled(true);
        s_nam->enableStrictTransportSecurityStore(true, QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1String("/hsts/"));
    }
    return s_nam;
}

void handleCommandLineArguments(ApplicationController *appController, const QStringList &args, bool isTemporary, const QString &page)
{
    for (const auto &file : args) {
        const auto localUrl = QUrl::fromLocalFile(file);
        if (QFile::exists(localUrl.toLocalFile())) {
            appController->importFromUrl(localUrl);
            if (isTemporary) {
                QFile::remove(localUrl.toLocalFile());
            }
        } else {
            appController->importFromUrl(QUrl::fromUserInput(file));
        }
    }

    if (!page.isEmpty()) {
        appController->requestOpenPage(page);
    }
}

#ifdef Q_OS_ANDROID
Q_DECL_EXPORT
#endif
int main(int argc, char **argv)
{
    QCoreApplication::setApplicationName(QStringLiteral("itinerary"));
    QCoreApplication::setOrganizationName(QStringLiteral("KDE"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationVersion(QStringLiteral(ITINERARY_VERSION_STRING));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
#ifdef Q_OS_ANDROID
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("Material"));
#else
    QIcon::setFallbackThemeName(QStringLiteral("breeze"));
    QApplication app(argc, argv); // for native file dialogs

    // Default to org.kde.desktop style unless the user forces another style
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
#endif
    QGuiApplication::setApplicationDisplayName(i18n("KDE Itinerary"));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("org.kde.itinerary")));

    auto aboutData = KAboutData::applicationData();
    aboutData.setProductName("KDE Itinerary/general"); // Bugzilla product/component name
    aboutData.setLicense(KAboutLicense::LGPL_V2, KAboutLicense::OrLaterVersions);
    aboutData.setHomepage(QStringLiteral("https://apps.kde.org/itinerary"));
    aboutData.setShortDescription(i18n("Digital Travel Assistant"));
    aboutData.setCopyrightStatement(i18n("Copyright © 2018-%1 The KDE Community", QStringLiteral("2022")));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.itinerary"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    QCommandLineOption isTemporaryOpt(QStringLiteral("tempfile"), QStringLiteral("Input file is a temporary file and will be deleted after importing."));
    parser.addOption(isTemporaryOpt);
    QCommandLineOption pageOpt(QStringLiteral("page"), i18n("Open Itinerary on the given page"), QStringLiteral("page"));
    parser.addOption(pageOpt);
    QCommandLineOption selfTestOpt(QStringLiteral("self-test"), QStringLiteral("internal, for automated testing"));
    parser.addOption(selfTestOpt);
    aboutData.setupCommandLine(&parser);
    parser.addPositionalArgument(QStringLiteral("file"), i18n("Files or URLs to import."));
    parser.process(app);
    aboutData.processCommandLine(&parser);
#if HAVE_KCRASH
    KCrash::initialize();
#endif

#ifndef Q_OS_ANDROID
    KDBusService service(KDBusService::Unique);
#endif

    Settings settings;
    s_settings = &settings;

    PkPassManager pkPassMgr;
    pkPassMgr.setNetworkAccessManagerFactory(namFactory);
    s_pkPassManager = &pkPassMgr;

    ReservationManager resMgr;
    s_reservationManager = &resMgr;

    DocumentManager docMgr;
    s_documentManager = &docMgr;

    FavoriteLocationModel favLocModel;
    s_favoriteLocationModel = &favLocModel;

    TripGroupManager tripGroupMgr;
    tripGroupMgr.setReservationManager(&resMgr);
    s_tripGroupManager = &tripGroupMgr;

    LiveDataManager liveDataMgr;
    liveDataMgr.setPkPassManager(&pkPassMgr);
    liveDataMgr.setReservationManager(&resMgr);
    liveDataMgr.setPollingEnabled(settings.queryLiveData());
    liveDataMgr.setShowNotificationsOnLockScreen(settings.showNotificationOnLockScreen());
    QObject::connect(&settings, &Settings::queryLiveDataChanged, &liveDataMgr, &LiveDataManager::setPollingEnabled);
    QObject::connect(&settings, &Settings::showNotificationOnLockScreenChanged, &liveDataMgr, &LiveDataManager::setShowNotificationsOnLockScreen);
    s_liveDataMnager = &liveDataMgr;

    WeatherForecastManager weatherForecastMgr;
    weatherForecastMgr.setAllowNetworkAccess(settings.weatherForecastEnabled());
    QObject::connect(&settings, &Settings::weatherForecastEnabledChanged, &weatherForecastMgr, &WeatherForecastManager::setAllowNetworkAccess);
    s_weatherForecastManager = &weatherForecastMgr;

    TransferManager transferManager;
    transferManager.setReservationManager(&resMgr);
    transferManager.setTripGroupManager(&tripGroupMgr);
    transferManager.setFavoriteLocationModel(&favLocModel);
    transferManager.setLiveDataManager(&liveDataMgr);
    transferManager.setAutoAddTransfers(settings.autoAddTransfers());
    transferManager.setAutoFillTransfers(settings.autoFillTransfers());
    QObject::connect(&settings, &Settings::autoAddTransfersChanged, &transferManager, &TransferManager::setAutoAddTransfers);
    QObject::connect(&settings, &Settings::autoFillTransfersChanged, &transferManager, &TransferManager::setAutoFillTransfers);
    s_tranferManager = &transferManager;

    tripGroupMgr.setTransferManager(&transferManager);

    TimelineModel timelineModel;
    timelineModel.setHomeCountryIsoCode(settings.homeCountryIsoCode());
    timelineModel.setReservationManager(&resMgr);
    timelineModel.setWeatherForecastManager(&weatherForecastMgr);
    timelineModel.setTransferManager(&transferManager);
    timelineModel.setTripGroupManager(&tripGroupMgr);
    QObject::connect(&settings, &Settings::homeCountryIsoCodeChanged, &timelineModel, &TimelineModel::setHomeCountryIsoCode);
    s_timelineModel = &timelineModel;

    TripGroupProxyModel tripGroupProxy;
    tripGroupProxy.setSourceModel(&timelineModel);
    s_tripGroupProxyModel = &tripGroupProxy;

    s_tripGroupInfoProvider.setReservationManager(&resMgr);
    s_tripGroupInfoProvider.setWeatherForecastManager(&weatherForecastMgr);

    MapDownloadManager mapDownloadMgr;
    mapDownloadMgr.setReservationManager(&resMgr);
    mapDownloadMgr.setAutomaticDownloadEnabled(s_settings->preloadMapData());
    QObject::connect(s_settings, &Settings::preloadMapDataChanged, &mapDownloadMgr, &MapDownloadManager::setAutomaticDownloadEnabled);
    s_mapDownloadManager = &mapDownloadMgr;

    KItinerary::JsonLdDocument::registerType<GenericPkPass>();
    PassManager passMgr;
    s_passManager = &passMgr;

    MatrixController matrixController;
    s_matrixController = &matrixController;

    ApplicationController appController;
    appController.setNetworkAccessManagerFactory(namFactory);
    appController.setReservationManager(&resMgr);
    appController.setPkPassManager(&pkPassMgr);
    appController.setDocumentManager(&docMgr);
    appController.setFavoriteLocationModel(&favLocModel);
    appController.setTransferManager(&transferManager);
    appController.setLiveDataManager(&liveDataMgr);
    appController.setTripGroupManager(&tripGroupMgr);
    appController.setPassManager(&passMgr);
#ifndef Q_OS_ANDROID
    QObject::connect(&service, &KDBusService::activateRequested, [&](const QStringList &args, const QString &workingDir) {
        qCDebug(Log) << "remote activation" << args << workingDir;
        if (!args.isEmpty()) {
            QDir::setCurrent(workingDir);
            parser.parse(args);
            handleCommandLineArguments(&appController, parser.positionalArguments(), parser.isSet(isTemporaryOpt), parser.value(pageOpt));
        }
        if (!QGuiApplication::allWindows().isEmpty()) {
            QWindow *window = QGuiApplication::allWindows().at(0);
            KWindowSystem::updateStartupId(window);
            KWindowSystem::activateWindow(window);
        }
    });
#endif

    OnlineTicketImporter::setNetworkAccessManagerFactory(namFactory);

    registerKContactsTypes();
    registerKCalendarCoreTypes();
    registerKPkPassTypes();
    registerKItineraryTypes();
    registerApplicationTypes();
    registerApplicationSingletons();

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("org.kde.pkpass"), new PkPassImageProvider(&pkPassMgr));
    auto l10nContext = new KLocalizedContext(&engine);
    l10nContext->setTranslationDomain(QStringLiteral(TRANSLATION_DOMAIN));
    engine.rootContext()->setContextObject(l10nContext);
    engine.load(QStringLiteral("qrc:/main.qml"));

    // Exit on QML load error.
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    handleCommandLineArguments(&appController, parser.positionalArguments(), parser.isSet(isTemporaryOpt), parser.value(pageOpt));

#ifdef Q_OS_ANDROID
    using namespace KAndroidExtras;
    appController.importFromIntent(Activity::getIntent());
#endif

    if (parser.isSet(selfTestOpt)) {
        QTimer::singleShot(std::chrono::milliseconds(250), &app, &QCoreApplication::quit);
    }

    return app.exec();
}

#include "main.moc"
