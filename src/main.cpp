#include "modulemanager.h"
#include "jsonsessionloader.h"
#include "modulemanagercontroller.h"
#include "moduleeventhandler.h"

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

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  QStringList loggingFilters = QStringList() << QString("%1.debug=false").arg(VEIN_EVENT().categoryName()) <<
                                                QString("%1.debug=false").arg(VEIN_NET_VERBOSE().categoryName()) <<
                                                QString("%1.debug=false").arg(VEIN_NET_INTRO_VERBOSE().categoryName()) << //< Introspection logging is still enabled
                                                QString("%1.debug=false").arg(VEIN_NET_TCP_VERBOSE().categoryName()) <<
                                                QString("%1.debug=false").arg(VEIN_API_QML().categoryName()) <<
                                                QString("%1.debug=false").arg(VEIN_API_QML_VERBOSE().categoryName()) <<
                                                QString("%1.debug=false").arg(VEIN_STORAGE_HASH_VERBOSE().categoryName());

  const VeinLogger::DBFactory sqliteFactory = [](){
    return new VeinLogger::SQLiteDB();
  };

  QLoggingCategory::setFilterRules(loggingFilters.join("\n"));

  ModuleEventHandler *evHandler = new ModuleEventHandler(&a);

  ModuleManagerController *mmController = new ModuleManagerController(&a);
  VeinStorage::VeinHash *storSystem = new VeinStorage::VeinHash(&a);
  VeinNet::IntrospectionSystem *introspectionSystem = new VeinNet::IntrospectionSystem(storSystem);
  VeinNet::NetworkSystem *netSystem = new VeinNet::NetworkSystem(&a);
  VeinNet::TcpSystem *tcpSystem = new VeinNet::TcpSystem(&a);
  VeinScript::ScriptSystem *scriptSystem = new VeinScript::ScriptSystem(&a);
  VeinApiQml::VeinQml *qmlSystem = new VeinApiQml::VeinQml(&a);
  VeinLogger::DatabaseLogger *dataLoggerSystem = new VeinLogger::DatabaseLogger(new VeinLogger::DataSource(storSystem, &a), sqliteFactory, &a);

  VeinApiQml::VeinQml::setStaticInstance(qmlSystem);
  VeinLogger::QmlLogger::setStaticLogger(dataLoggerSystem);

  ZeraModules::ModuleManager *modMan = new ZeraModules::ModuleManager(&a);
  JsonSessionLoader *sessionLoader = new JsonSessionLoader(&a);

  netSystem->setOperationMode(VeinNet::NetworkSystem::VNOM_SUBSCRIPTION);
  QObject::connect(dataLoggerSystem, &VeinLogger::DatabaseLogger::sigDatabaseError, [dataLoggerSystem](const QString &t_error){
    QJsonObject jsonErrorObj;

    jsonErrorObj.insert("ModuleName", "DataLogger");
    jsonErrorObj.insert("Time", QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm:ss"));
    jsonErrorObj.insert("Error", t_error);

    VeinComponent::ComponentData *cData = new VeinComponent::ComponentData();
    cData->setEntityId(0);
    cData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
    cData->setEventTarget(VeinEvent::EventData::EventTarget::ET_LOCAL);
    cData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
    cData->setComponentName("Error_Messages");

    cData->setNewValue(jsonErrorObj);

    emit dataLoggerSystem->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::TRANSACTION, cData));
  });

  QList<VeinEvent::EventSystem*> subSystems;
  //do not reorder
  subSystems.append(mmController);
  subSystems.append(storSystem);
  subSystems.append(introspectionSystem);
  subSystems.append(netSystem);
  subSystems.append(tcpSystem);
  subSystems.append(qmlSystem);
  subSystems.append(scriptSystem);
  subSystems.append(dataLoggerSystem);

  evHandler->setSubsystems(subSystems);

  modMan->setStorage(storSystem);
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
    qDebug() << "[Zera-Module-Manager] No modules found";
    a.quit();
  }
  else
  {
    modMan->loadDefaultSession();
    mmController->initOnce();
    tcpSystem->startServer(12000);
  }
  QObject::connect(modMan, &ZeraModules::ModuleManager::sigModulesLoaded, mmController, &ModuleManagerController::initializeEntities);
  QObject::connect(mmController, &ModuleManagerController::sigChangeSession, modMan, &ZeraModules::ModuleManager::onChangeSession);
  QObject::connect(mmController, &ModuleManagerController::sigModulesPausedChanged, modMan, &ZeraModules::ModuleManager::setModulesPaused);

  //0 = ModuleManagerController, 2 = VeinLogger::DatabaseLogger
  qmlSystem->setRequiredIds(QList<int>()<<0<<2);
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
