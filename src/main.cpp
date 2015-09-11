#include "modulemanager.h"
#include "jsonsessionloader.h"

#include <QCoreApplication>
#include <QFile>
#include <QByteArray>
#include <QUuid>

#include <ve_eventhandler.h>
#include <ve_validator.h>
#include <ve_scriptengine.h>
#include <vn_networksystem.h>
#include <vn_tcpsystem.h>
#include <vn_introspectionsystem.h>
#include <vs_veinhash.h>
#include <vscr_componentengine.h>

#include <vcmp_entitydata.h>
#include <vcmp_componentdata.h>
#include <ve_commandevent.h>

#include <QDebug>
#include <QFile>

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
                                                QString("%1.debug=false").arg(VEIN_STORAGE_HASH_VERBOSE().categoryName());


  QLoggingCategory::setFilterRules(loggingFilters.join("\n"));

  VeinEvent::EventHandler *evHandler = new VeinEvent::EventHandler();

  VeinEvent::Validator *validator = new VeinEvent::Validator(&a);
//  VeinScript::ComponentEngine *scrSystem = new VeinScript::ComponentEngine(&a);
  VeinStorage::VeinHash *storSystem = new VeinStorage::VeinHash(&a);
  VeinNet::IntrospectionSystem *introspectionSystem = new VeinNet::IntrospectionSystem(&a);
  VeinNet::NetworkSystem *netSystem = new VeinNet::NetworkSystem(&a);
  VeinNet::TcpSystem *tcpSystem = new VeinNet::TcpSystem(&a);


  ZeraModules::ModuleManager *modMan = new ZeraModules::ModuleManager(&a);
  JsonSessionLoader *sessionLoader = new JsonSessionLoader(&a);

  netSystem->setOperationMode(VeinNet::NetworkSystem::VNOM_SUBCRIPTION);

  QList<VeinEvent::EventSystem*> subSystems;

  subSystems.append(validator);
//  subSystems.append(scrSystem);
  subSystems.append(storSystem);
  subSystems.append(introspectionSystem);
  subSystems.append(netSystem);
  subSystems.append(tcpSystem);


  evHandler->setSubsystems(subSystems);

  introspectionSystem->setStorage(storSystem);
  modMan->setStorage(storSystem);
  modMan->setEventHandler(evHandler);

  QObject::connect(sessionLoader, &JsonSessionLoader::sigLoadModule, modMan, &ZeraModules::ModuleManager::startModule);
  QObject::connect(modMan, &ZeraModules::ModuleManager::sigSessionSwitched, sessionLoader, &JsonSessionLoader::loadSession);

  bool modulesFound;

  qRegisterMetaTypeStreamOperators<QList<double> >("QList<double>");
  qRegisterMetaTypeStreamOperators<QList<QString> >("QList<QString>");

  modulesFound = modMan->loadModules();

  if(!modulesFound)
  {
    qDebug() << "[Zera-Module-Manager] No modules found";
    a.quit();
  }
  else
  {
    modMan->loadDefaultSession();
    tcpSystem->startServer(12000);
  }

  QObject::connect(modMan, &ZeraModules::ModuleManager::sigModulesLoaded, [&]() {
    VeinComponent::EntityData *systemData = new VeinComponent::EntityData();
    systemData->setCommand(VeinComponent::EntityData::Command::ECMD_ADD);
    systemData->setEntityId(0);

    VeinEvent::CommandEvent *systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::TRANSACTION, systemData);

    QCoreApplication::postEvent(evHandler, systemEvent);
    systemEvent=0;
    systemData=0;

    VeinComponent::ComponentData *introspectionData=0;

    introspectionData = new VeinComponent::ComponentData();
    introspectionData->setEntityId(0);
    introspectionData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
    introspectionData->setComponentName("EntityName");
    introspectionData->setNewValue("_System");

    systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::TRANSACTION, introspectionData);
    QCoreApplication::postEvent(evHandler, systemEvent);
    systemEvent = 0;

    introspectionData = new VeinComponent::ComponentData();
    introspectionData->setEntityId(0);
    introspectionData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
    introspectionData->setComponentName("Entities");
    qDebug() << "ENTITIES" << storSystem->getEntityList() << QVariant::fromValue<QList<int> >(storSystem->getEntityList()).value<QList<int> >();
    introspectionData->setNewValue(QVariant::fromValue<QList<int> >(storSystem->getEntityList()));

    systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::TRANSACTION, introspectionData);
    QCoreApplication::postEvent(evHandler, systemEvent);

    systemEvent = 0;
  });


  return a.exec();
}
