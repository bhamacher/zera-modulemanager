#include "modulemanager.h"
#include "jsonsessionloader.h"
#include "modulemanagercontroller.h"
#include "moduleeventhandler.h"
#include "customerdatasystem.h"
#include "priorityarbitrationsystem.h"
#include "licensesystem.h"

#include <QCoreApplication>

#include <ve_commandevent.h>
#include <vcmp_componentdata.h>
#include <vn_networksystem.h>
#include <vn_tcpsystem.h>
#include <vn_introspectionsystem.h>
#include <vs_veinhash.h>
#include <vsc_scriptsystem.h>
#include <veinqml.h>
#include <veinqmlwrapper.h>

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


#include <veinmanager.h>

namespace ZeraModulemanager
{
QJsonDocument getDefaultConfig()
{
    QJsonDocument retVal;
    QFile configFile(MODMAN_CONFIG_FILE);
    if(configFile.exists() && configFile.open(QFile::ReadOnly))
    {
        retVal = QJsonDocument::fromJson(configFile.readAll());
    }
    return retVal;
}
const QString getDevNameFromUBoot()
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
}
/**
 * @brief main
 * @todo remove  VeinLogger::QmlLogger::setContentSetPaths, when handling is moved to vl_databaselogger.
 */
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    const QJsonDocument defaultConfig = ZeraModulemanager::getDefaultConfig();
    if(defaultConfig.isEmpty() || defaultConfig.isObject() == false)
    {
        qCritical() << "Error loading config file from path:" << MODMAN_CONFIG_FILE;
        return -ENOENT;
    }
    const QString deviceNameUBoot = ZeraModulemanager::getDevNameFromUBoot();
    const QString deviceName = !deviceNameUBoot.isEmpty() ? deviceNameUBoot : defaultConfig.object().value("deviceName").toString();
    if(deviceName.isEmpty())
    {
        qCritical() << "No device name found in kernel cmdline or default config!";
        return -ENODEV;
    }
    qInfo() << "Loading session data for " << deviceName;
    const bool customerdataSystemEnabled = defaultConfig.object().value(deviceName).toObject().value("customerdataSystemEnabled").toBool(false);
    const QVariant tmpAvailList = defaultConfig.object().value(deviceName).toObject().value("availableSessions").toArray().toVariantList();
    const QStringList availableSessionList = tmpAvailList.toStringList();
    if(availableSessionList.isEmpty())
    {
        qCritical() << "No sessions found for device" << deviceName;
        return -ENODEV;
    }
    const QString defaultSessionFile = defaultConfig.object().value(deviceName).toObject().value("defaultSession").toString();
    if(defaultSessionFile.isEmpty())
    {
        qCritical() << "No default session found for device" << deviceName;
        return -ENODEV;
    }

    QStringList loggingFilters = QStringList() << QString("%1.debug=false").arg(VEIN_EVENT().categoryName()) <<
                                                  QString("%1.debug=false").arg(VEIN_NET_VERBOSE().categoryName()) <<
                                                  QString("%1.debug=false").arg(VEIN_NET_INTRO_VERBOSE().categoryName()) << //< Introspection logging is still enabled
                                                  QString("%1.debug=false").arg(VEIN_NET_TCP_VERBOSE().categoryName()) <<
                                                  QString("%1.debug=false").arg(VEIN_API_QML().categoryName()) <<
                                                  QString("%1.debug=false").arg(VEIN_API_QML_VERBOSE().categoryName()) <<
                                                  //                                                QString("%1.debug=false").arg(VEIN_LOGGER().categoryName()) <<
                                                  QString("%1.debug=false").arg(VEIN_STORAGE_HASH_VERBOSE().categoryName());
    QLoggingCategory::setFilterRules(loggingFilters.join("\n"));

    const VeinLogger::DBFactory sqliteFactory = [](){
        return new VeinLogger::SQLiteDB();
    };


    ModuleEventHandler *evHandler = new ModuleEventHandler(&a);

#ifdef DEVICE_ARBITRATION
    //priority based arbitration
    PriorityArbitrationSystem *arbitrationSystem = new PriorityArbitrationSystem(&a);
    evHandler->setArbitrationSystem(arbitrationSystem);
#endif

    // setup vein modules
    VeinManager *mmController = new VeinManager(&a);
    VeinScript::ScriptSystem *scriptSystem = new VeinScript::ScriptSystem(&a);
    VeinLogger::DatabaseLogger *dataLoggerSystem = new VeinLogger::DatabaseLogger(new VeinLogger::DataSource(static_cast<VeinStorage::VeinHash*>(mmController->getStorageSystem())),sqliteFactory); //takes ownership of DataSource
    dataLoggerSystem->setContentSetPath(QString(MODMAN_CONTENTSET_PATH).append("ZeraContext.json"),QString(MODMAN_CONTENTSET_PATH).append("CustomerContext.json"));
    CustomerDataSystem *customerDataSystem = nullptr;
    LicenseSystem *licenseSystem = new LicenseSystem({QUrl("file:///home/operator/license-keys")}, &a);
    vfExport::vf_export *exportModule=new vfExport::vf_export();
    vfFiles::vf_files *filesModule = new vfFiles::vf_files();

    // set zera and customer contentSet path. Defined in CMakeLists.txt.

    ZeraModules::ModuleManager *modMan = new ZeraModules::ModuleManager(availableSessionList, &a);
    JsonSessionLoader *sessionLoader = new JsonSessionLoader(&a);


    auto errorReportFunction = [dataLoggerSystem](const QString &t_error){
        QJsonObject jsonErrorObj;

        jsonErrorObj.insert("ModuleName", "DataLogger");
        jsonErrorObj.insert("Time", QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm:ss"));
        jsonErrorObj.insert("Error", t_error);

        VeinComponent::ComponentData *cData = new VeinComponent::ComponentData();
        cData->setEntityId(0);
        cData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        cData->setEventTarget(VeinEvent::EventData::EventTarget::ET_LOCAL);
        cData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        cData->setComponentName(ModuleManagerController::s_notificationMessagesComponentName);

        cData->setNewValue(jsonErrorObj);

        emit dataLoggerSystem->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::TRANSACTION, cData));
    };
    QObject::connect(dataLoggerSystem, &VeinLogger::DatabaseLogger::sigDatabaseError, errorReportFunction);


    QList<VeinEvent::EventSystem*> subSystems;
    //do not reorder
    subSystems.append(mmController);
    subSystems.append(scriptSystem);
    subSystems.append(licenseSystem);


    evHandler->setSubsystems(subSystems);

    //conditional systems
    bool customerDataSystemInitialized = false;
    if(customerdataSystemEnabled)
    {
        QObject::connect(licenseSystem, &LicenseSystem::sigSerialNumberInitialized, [&]() {
            if(licenseSystem->isSystemLicensed(CustomerDataSystem::s_entityName) && !customerDataSystemInitialized)
            {
                customerDataSystemInitialized = true;
                customerDataSystem = new CustomerDataSystem(&a);
                QObject::connect(customerDataSystem, &CustomerDataSystem::sigCustomerDataError, errorReportFunction);
                qDebug() << "CustomerDataSystem is enabled";
                evHandler->addSubsystem(customerDataSystem);
                customerDataSystem->initializeEntity();
            }
        });
    }
    bool dataLoggerSystemInitialized = false;

    QObject::connect(filesModule->getVeinEntity(),&VeinEvent::EventSystem::sigAttached,[=](){
        filesModule->initOnce();
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



    QObject::connect(licenseSystem, &LicenseSystem::sigSerialNumberInitialized, [&](){
        if(licenseSystem->isSystemLicensed(dataLoggerSystem->entityName()))
        {
            if(!dataLoggerSystemInitialized)
            {
                dataLoggerSystemInitialized = true;
                qDebug() << "DataLoggerSystem is enabled";
                evHandler->addSubsystem(dataLoggerSystem);

                // the following code is not exactly depending on license system
                // but here event system is up and running

                // files entity
                evHandler->addSubsystem(filesModule->getVeinEntity());

                // exports entity
                evHandler->addSubsystem(exportModule->getVeinEntity());
                exportModule->initOnce();

                // subscribe those entitities our magic logger QML script
                // requires (see modMan->loadScripts above)
            }
        }
    });

    modMan->setLicenseSystem(licenseSystem);
    modMan->setEventHandler(evHandler);

    QObject::connect(sessionLoader, &JsonSessionLoader::sigLoadModule, modMan, &ZeraModules::ModuleManager::startModule);
    QObject::connect(modMan, &ZeraModules::ModuleManager::sigSessionSwitched, sessionLoader, &JsonSessionLoader::loadSession);
    QObject::connect(modMan, &ZeraModules::ModuleManager::sigSessionSwitched, [&dataLoggerSystem]() {
        //disable logging to prevent data logging between session switching
        dataLoggerSystem->setLoggingEnabled(false);
    });

    bool modulesFound;

    qRegisterMetaTypeStreamOperators<QList<int> >("QList<int>");
    qRegisterMetaTypeStreamOperators<QList<float> >("QList<float>");
    qRegisterMetaTypeStreamOperators<QList<double> >("QList<double>");
    qRegisterMetaTypeStreamOperators<QList<QString> >("QList<QString>");
    qRegisterMetaTypeStreamOperators<QVector<QString> >("QVector<QString>");
    qRegisterMetaTypeStreamOperators<QList<QVariantMap> >("QList<QVariantMap>");



    modulesFound = modMan->loadModules();

    if(!modulesFound)
    {
        qCritical() << "[Zera-Module-Manager] No modules found";
        a.quit();
    }
    else
    {
        modMan->changeSessionFile(defaultSessionFile);
        mmController->initOnce();
        mmController->startServer(12000);
    }

    return a.exec();
}
