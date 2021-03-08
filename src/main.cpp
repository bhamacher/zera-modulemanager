#include <moduleeventhandler.h>

#include "customerdatasystem.h"


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
#include <QUrl>


#include <veinmanager.h>
#include <modulemanager.h>
#include <licensesystem.h>


/**
 * @brief main
 * @todo remove  VeinLogger::QmlLogger::setContentSetPaths, when handling is moved to vl_databaselogger.
 */
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

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

    ZeraModules::ModuleManager* modManager = new ZeraModules::ModuleManager(QString(),MODMAN_CONFIG_FILE,MODMAN_SESSION_PATH,MODMAN_MODULE_PATH);
    modManager->setEventHandler(evHandler);


    // setup vein modules
    VeinManager *mmController = new VeinManager(&a);

    VeinScript::ScriptSystem *scriptSystem = new VeinScript::ScriptSystem(&a);

    VeinLogger::DatabaseLogger *dataLoggerSystem = new VeinLogger::DatabaseLogger(new VeinLogger::DataSource(static_cast<VeinStorage::VeinHash*>(mmController->getStorageSystem())),sqliteFactory); //takes ownership of DataSource
    dataLoggerSystem->setContentSetPath(QString(MODMAN_CONTENTSET_PATH).append("ZeraContext.json"),QString(MODMAN_CONTENTSET_PATH).append("CustomerContext.json"));

    CustomerDataSystem *customerDataSystem = nullptr;

    vfExport::vf_export *exportModule=new vfExport::vf_export();

    vfFiles::vf_files *filesModule = new vfFiles::vf_files();

    LicenseSystem* licSys = new LicenseSystem({QUrl("file:///home/operator/license-keys/")},QUrl("file:///opt/zera/conf/serialnumber"));
    modManager->setLicenseSystem(licSys);

/*
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
*/

    QList<VeinEvent::EventSystem*> subSystems;
    //do not reorder
    subSystems.append(mmController);
    subSystems.append(scriptSystem);
    subSystems.append(modManager->entity());
    evHandler->setSubsystems(subSystems);
    modManager->loadLicensedModule("CustomerData", customerDataSystem);
    modManager->loadLicensedModule("_LoggingSystem", exportModule->getVeinEntity());
    modManager->loadLicensedModule("_LoggingSystem", dataLoggerSystem);


    //conditional systems

        QObject::connect(customerDataSystem, &CustomerDataSystem::sigAttached, [&]() {
                customerDataSystem->initializeEntity();
        });


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

    bool modulesFound;

    qRegisterMetaTypeStreamOperators<QList<int> >("QList<int>");
    qRegisterMetaTypeStreamOperators<QList<float> >("QList<float>");
    qRegisterMetaTypeStreamOperators<QList<double> >("QList<double>");
    qRegisterMetaTypeStreamOperators<QList<QString> >("QList<QString>");
    qRegisterMetaTypeStreamOperators<QVector<QString> >("QVector<QString>");
    qRegisterMetaTypeStreamOperators<QList<QVariantMap> >("QList<QVariantMap>");


    if(!modulesFound)
    {
        qCritical() << "[Zera-Module-Manager] No modules found";
        a.quit();
    }
    else
    {
        mmController->startServer(12000);
    }

    return a.exec();
}
