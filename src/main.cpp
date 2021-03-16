#include <moduleeventhandler.h>
#include <QCoreApplication>

#include <vs_veinhash.h>

#include <vl_databaselogger.h>
#include <vl_datasource.h>
#include <vl_sqlitedb.h>
#include <vf_export.h>
#include <vf_files.h>

#include <QDebug>

#include <QLoggingCategory>
#include <QStringList>
#include <QDataStream>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QRegExp>
#include <QUrl>


#include <veinmanager.h>
#include <modulemanager.h>
#include <licensesystem.h>
#include <customerdatasystem.h>
#include <QCommandLineParser>


QString getDevNameFromUBoot()
{
    QString strDeviceName;
    // Check for kernel cmdline param which u-boot should set
    QFile procFileCmdLine(QLatin1String("/proc/cmdline"));
    if(procFileCmdLine.open(QIODevice::ReadOnly))
    {
        QString cmdLine = procFileCmdLine.readAll();
        procFileCmdLine.close();
        // Extract 'zera_device=<device_name>'
        QRegExp regExp(QLatin1String("\\bzera_device=[^ ]*"));
        if(regExp.indexIn(cmdLine) != -1)
        {
            strDeviceName = regExp.cap(0);
            // The following should go in regex above but...
            strDeviceName.replace(QLatin1String("zera_device="), QLatin1String(""));
            strDeviceName.replace(QLatin1String("\n"), QLatin1String(""));
            qInfo() << "ZERA Device from kernel cmdline: " << strDeviceName;
        }
    }
    return strDeviceName;
}


/**
 * @brief main
 */
int main(int argc, char *argv[])
{


    QString deviceName;


    QCoreApplication a(argc, argv);

    const QCommandLineOption devName("d", "device name", "devName");
    QCommandLineParser parser;
    parser.addOption(devName);
    parser.process(a);

    if(parser.isSet(devName)){
        deviceName=parser.value(devName);
    }else{
        deviceName=getDevNameFromUBoot();
    }



    QStringList loggingFilters = QStringList() << QString("%1.debug=false").arg(VEIN_EVENT().categoryName()) <<
                                                  QString("%1.debug=false").arg(VEIN_NET_VERBOSE().categoryName()) <<
                                                  QString("%1.debug=false").arg(VEIN_NET_INTRO_VERBOSE().categoryName()) << //< Introspection logging is still enabled
                                                  QString("%1.debug=false").arg(VEIN_NET_TCP_VERBOSE().categoryName()) <<
                                                  //                                                QString("%1.debug=false").arg(VEIN_LOGGER().categoryName()) <<
                                                  QString("%1.debug=false").arg(VEIN_STORAGE_HASH_VERBOSE().categoryName());
    QLoggingCategory::setFilterRules(loggingFilters.join("\n"));

    const VeinLogger::DBFactory sqliteFactory = [](){
        return new VeinLogger::SQLiteDB();
    };


    ModuleEventHandler *evHandler = new ModuleEventHandler(&a);

    // setup vein modules
    VeinManager *mmController = new VeinManager(&a);

    ZeraModules::ModuleManager* modManager = new ZeraModules::ModuleManager(deviceName,MODMAN_CONFIG_FILE,MODMAN_SESSION_PATH,MODMAN_MODULE_PATH);
    modManager->setEventHandler(evHandler);
    modManager->setStorage(mmController->getStorageSystem());

    VeinLogger::DatabaseLogger *dataLoggerSystem = new VeinLogger::DatabaseLogger(new VeinLogger::DataSource(static_cast<VeinStorage::VeinHash*>(mmController->getStorageSystem())),sqliteFactory); //takes ownership of DataSource
    dataLoggerSystem->setContentSetPath(QString(MODMAN_CONTENTSET_PATH).append("ZeraContext.json"),QString(MODMAN_CONTENTSET_PATH).append("CustomerContext.json"));

    VfCustomerdata::CustomerDataSystem *customerDataSystem = new VfCustomerdata::CustomerDataSystem("/home/operator/customerdata/");

    vfExport::vf_export *exportModule=new vfExport::vf_export(&a,4);

    vfFiles::vf_files *filesModule = new vfFiles::vf_files(&a,5);

    LicenseSystem* licSys = new LicenseSystem({QUrl("file:///home/operator/license-keys/")},QUrl("file:///opt/zera/conf/serialnumber"));
    modManager->setLicenseSystem(licSys);

    QList<VeinEvent::EventSystem*> subSystems;
    subSystems.append(mmController);
    subSystems.append(modManager->entity());
    subSystems.append(filesModule->getVeinEntity());
    evHandler->setSubsystems(subSystems);
    modManager->loadLicensedModule("CustomerData", customerDataSystem->entity());
    modManager->loadLicensedModule("_LoggingSystem", exportModule->getVeinEntity());
    modManager->loadLicensedModule("_LoggingSystem", dataLoggerSystem);


    //conditional systems
    QObject::connect(filesModule->getVeinEntity(),&VeinEvent::EventSystem::sigAttached,[=](){
        filesModule->addMountToWatch(
                    QStringLiteral("AutoMountedPaths"),
                    QStringLiteral(MODMAN_AUTOMOUNT_PATH));
        filesModule->addDirToWatch(
                    QStringLiteral("AvailableCustomerData"),
                    QStringLiteral(MODMAN_CUSTOMERDATA_PATH),
                    QStringList({"*.json", "*/*.json"}),
                    QDir::NoDotAndDotDot | QDir::Files,
                    false);
        filesModule->addDefaultPathComponent(
                    QStringLiteral("LoggerLocalPath"),
                    QStringLiteral(MODMAN_LOGGER_LOCAL_PATH),
                    true);
        filesModule->addDefaultPathComponent(
                    QStringLiteral("CustomerDataLocalPath"),
                    QStringLiteral(MODMAN_CUSTOMERDATA_PATH),
                    true);
    });


    qRegisterMetaTypeStreamOperators<QList<int> >("QList<int>");
    qRegisterMetaTypeStreamOperators<QSet<int> >("QSet<int>");
    qRegisterMetaTypeStreamOperators<QList<float> >("QList<float>");
    qRegisterMetaTypeStreamOperators<QList<double> >("QList<double>");
    qRegisterMetaTypeStreamOperators<QList<QString> >("QList<QString>");
    qRegisterMetaTypeStreamOperators<QVector<QString> >("QVector<QString>");
    qRegisterMetaTypeStreamOperators<QList<QVariantMap> >("QList<QVariantMap>");
    mmController->startServer(12000);

    return a.exec();
}
