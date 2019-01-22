#include "modulemanager.h"
#include "jsonsessionloader.h"
#include "modulemanagercontroller.h"
#include "moduleeventhandler.h"
#include "customerdatasystem.h"
#include "priorityarbitrationsystem.h"
#include "zeradblogger.h"
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
#include <vl_qmllogger.h>
#include <vl_sqlitedb.h>

#include <QDebug>

#include <QLoggingCategory>
#include <QStringList>
#include <QDataStream>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
}

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  const QJsonDocument defaultConfig = ZeraModulemanager::getDefaultConfig();
  if(defaultConfig.isEmpty() || defaultConfig.isObject() == false)
  {
    qCritical() << "Error loading config file from path:" << MODMAN_CONFIG_FILE;
    return -ENOENT;
  }
  const QString deviceName = defaultConfig.object().value("deviceName").toString();
  const bool customerdataSystemEnabled = defaultConfig.object().value(deviceName).toObject().value("customerdataSystemEnabled").toBool(false);
  const QVariant tmpAvailList = defaultConfig.object().value(deviceName).toObject().value("availableSessions").toArray().toVariantList();
  const QStringList availableSessionList = tmpAvailList.toStringList();

  QStringList loggingFilters = QStringList() << QString("%1.debug=false").arg(VEIN_EVENT().categoryName()) <<
                                                QString("%1.debug=false").arg(VEIN_NET_VERBOSE().categoryName()) <<
                                                QString("%1.debug=false").arg(VEIN_NET_INTRO_VERBOSE().categoryName()) << //< Introspection logging is still enabled
                                                QString("%1.debug=false").arg(VEIN_NET_TCP_VERBOSE().categoryName()) <<
                                                QString("%1.debug=false").arg(VEIN_API_QML().categoryName()) <<
                                                QString("%1.debug=false").arg(VEIN_API_QML_VERBOSE().categoryName()) <<
                                                //                                                QString("%1.debug=false").arg(VEIN_LOGGER().categoryName()) <<
                                                QString("%1.debug=false").arg(VEIN_STORAGE_HASH_VERBOSE().categoryName());

  const VeinLogger::DBFactory sqliteFactory = [](){
    return new VeinLogger::SQLiteDB();
  };

  QLoggingCategory::setFilterRules(loggingFilters.join("\n"));

  ModuleEventHandler *evHandler = new ModuleEventHandler(&a);

#ifdef DEVICE_ARBITRATION
  //priority based arbitration
  PriorityArbitrationSystem *arbitrationSystem = new PriorityArbitrationSystem(&a);
  evHandler->setArbitrationSystem(arbitrationSystem);
#endif
  ModuleManagerController *mmController = new ModuleManagerController(&a);
  VeinNet::IntrospectionSystem *introspectionSystem = new VeinNet::IntrospectionSystem(&a);
  VeinStorage::VeinHash *storSystem = new VeinStorage::VeinHash(&a);
  VeinNet::NetworkSystem *netSystem = new VeinNet::NetworkSystem(&a);
  VeinNet::TcpSystem *tcpSystem = new VeinNet::TcpSystem(&a);
  VeinScript::ScriptSystem *scriptSystem = new VeinScript::ScriptSystem(&a);
  VeinApiQml::VeinQml *qmlSystem = new VeinApiQml::VeinQml(&a);
  ZeraDBLogger *dataLoggerSystem = new ZeraDBLogger(new VeinLogger::DataSource(storSystem, &a), sqliteFactory, &a); //takes ownership of DataSource
  CustomerDataSystem *customerDataSystem = nullptr;
  LicenseSystem *licenseSystem = new LicenseSystem({QUrl("file:///home/operator/license-keys")}, &a);

  VeinApiQml::VeinQml::setStaticInstance(qmlSystem);
  VeinLogger::QmlLogger::setStaticLogger(dataLoggerSystem);

  ZeraModules::ModuleManager *modMan = new ZeraModules::ModuleManager(availableSessionList, &a);
  JsonSessionLoader *sessionLoader = new JsonSessionLoader(&a);

  netSystem->setOperationMode(VeinNet::NetworkSystem::VNOM_SUBSCRIPTION);
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
  if(customerdataSystemEnabled && licenseSystem->isSystemLicensed(CustomerDataSystem::s_entityName))
  {
    qDebug() << "CustomerDataSystem is enabled";
    customerDataSystem = new CustomerDataSystem(&a);
    QObject::connect(customerDataSystem, &CustomerDataSystem::sigCustomerDataError, errorReportFunction);
    subSystems.append(customerDataSystem);
  }
  subSystems.append(introspectionSystem);
  subSystems.append(storSystem);
  subSystems.append(netSystem);
  subSystems.append(tcpSystem);
  subSystems.append(qmlSystem);
  subSystems.append(scriptSystem);
  if(licenseSystem->isSystemLicensed(dataLoggerSystem->entityName()))
  {
    qDebug() << "DataLoggerSystem is enabled";
    subSystems.append(dataLoggerSystem);
  }
  subSystems.append(licenseSystem);

  evHandler->setSubsystems(subSystems);

  modMan->setStorage(storSystem);
  modMan->setLicenseSystem(licenseSystem);
  modMan->setEventHandler(evHandler);
  mmController->setStorage(storSystem);

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

  modulesFound = modMan->loadModules();

  if(!modulesFound)
  {
    qCritical() << "[Zera-Module-Manager] No modules found";
    a.quit();
  }
  else
  {
    const QString defaultSessionFile = defaultConfig.object().value(deviceName).toObject().value("defaultSession").toString();
    modMan->changeSessionFile(defaultSessionFile);
    mmController->initOnce();
    tcpSystem->startServer(12000);
  }
  QObject::connect(modMan, &ZeraModules::ModuleManager::sigModulesLoaded, mmController, &ModuleManagerController::initializeEntity);
  QObject::connect(mmController, &ModuleManagerController::sigChangeSession, modMan, &ZeraModules::ModuleManager::changeSessionFile);
  QObject::connect(mmController, &ModuleManagerController::sigModulesPausedChanged, modMan, &ZeraModules::ModuleManager::setModulesPaused);


  qmlSystem->entitySubscribeById(0); //0 = ModuleManagerController
  qmlSystem->entitySubscribeById(2); //2 = VeinLogger::DatabaseLogger
  bool initOnce = false;

  QObject::connect(qmlSystem, &VeinApiQml::VeinQml::sigStateChanged, [&](VeinApiQml::VeinQml::ConnectionState t_state){
    if(t_state == VeinApiQml::VeinQml::ConnectionState::VQ_LOADED && initOnce == false)
    {
      modMan->loadScripts(scriptSystem);
      initOnce = true;
    }
  });

  return a.exec();
}
