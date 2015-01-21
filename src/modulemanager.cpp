#include "modulemanager.h"

#include <veinhub.h>
#include <veinpeer.h>
#include <veinentity.h>

#include <proxy.h>

#include <QPluginLoader>
#include <abstractmodulefactory.h>
#include <QDir>
#include <QDebug>

#include <QState>

namespace ZeraModules
{
  class ModuleData {
  public:
    ModuleData(VirtualModule *pRef, QString pName, QByteArray pConfData) : reference(pRef), uniqueName(pName), configData(pConfData) {}

    static ModuleData *findByReference(QList<ModuleData*> list, VirtualModule *ref)
    {
      ModuleData *retVal = 0;
      for(int i = 0; i < list.length(); i++)
      {
        if(list.at(i)->reference == ref)
        {
          retVal = list.at(i);
          break;
        }
      }
      return retVal;
    }

    VirtualModule *reference;
    const QString uniqueName;
    QByteArray configData;
  };


  ModuleManager::ModuleManager(QObject *qobjParent) :
    QObject(qobjParent),
    proxyInstance(Zera::Proxy::cProxy::getInstance()),
    moduleStartLock(false)
  {
  }

  ModuleManager::~ModuleManager()
  {
    foreach(ModuleData *toDelete, moduleList)
    {
      delete toDelete->reference;
    }
    proxyInstance->deleteLater();
  }

  void ModuleManager::configureModules()
  {

  }

  bool ModuleManager::isModuleLicensed(VirtualModule *module)
  {
    Q_UNUSED(module)
    /**
     * @todo implement function later
     */
    return true;
  }

  bool ModuleManager::loadModules()
  {
    bool retVal = true;
    QDir moduleDir;
    qDebug() << "Loading modules";
    foreach (QObject *staticModule, QPluginLoader::staticInstances())
    {
      qDebug()<<staticModule;//doNothing();
    }

    /**
     * @todo remove hardcoded path
     */
    moduleDir = "/work/qt_projects/Zera/zera-classes/modules";
    //moduleDir = "/usr/lib/zera-modules";
    //moduleDir = "/home/peter/modules";

    foreach (QString fileName, moduleDir.entryList(QDir::Files))
    {
      qDebug() << "File is a library?" << QLibrary::isLibrary(moduleDir.absoluteFilePath(fileName));
      QPluginLoader loader(moduleDir.absoluteFilePath(fileName));
      MeasurementModuleFactory *module = qobject_cast<MeasurementModuleFactory *>(loader.instance());
      qDebug() << "Analyzing:" << loader.fileName() << "loaded:" << loader.isLoaded();
      if (module)
      {
        factoryTable.insert(module->getFactoryName(), module);
      }
      else
      {
        retVal=false;
        qDebug() << "Error string:\n" << loader.errorString();
      }
    }
    return retVal;
  }



  void ModuleManager::setHub(VeinHub *vHub)
  {
    if(vHub)
    {
      localHub = vHub;
      modManPeer = localHub->peerAdd("ModuleManager"); ///@todo remove hardcoded
      sessionSwitchEntity = modManPeer->dataAdd("SessionFile"); ///@todo remove hardcoded
      //sessionSwitchEntity->setValue("default-session.json");
      connect(sessionSwitchEntity, SIGNAL(sigValueChanged(QVariant)), this, SLOT(onChangeSession(QVariant)));
    }
  }

  VeinHub *ModuleManager::getHub()
  {
    return localHub;
  }

  void ModuleManager::startModule(QString uniqueModuleName, QByteArray xmlConfigData)
  {
    if(moduleStartLock == false)
    {
      VeinPeer *tmpPeer=0;
      int moduleCount = 0;
      MeasurementModuleFactory *tmpFactory=0;

      tmpFactory=factoryTable.value(uniqueModuleName);
      if(tmpFactory)
      {
        moduleCount = tmpFactory->listModules().size();
        tmpPeer=localHub->peerAdd(QString("%1%2").arg(uniqueModuleName).arg(moduleCount));
        qDebug()<<"Creating module instance:"<<tmpPeer->getName(); //<< "with config" << xmlConfigData;
        VirtualModule *tmpModule = factoryTable.value(uniqueModuleName)->createModule(proxyInstance,tmpPeer,this);
        if(tmpModule)
        {
          if(!xmlConfigData.isNull())
          {
            tmpModule->setConfiguration(xmlConfigData);
          }
          connect(tmpModule, &VirtualModule::moduleDeactivated, this, &ModuleManager::onModuleDelete);
          connect(tmpModule, &VirtualModule::moduleActivated, this, &ModuleManager::onModuleStartNext);
          connect(tmpModule, &VirtualModule::moduleError, this, &ModuleManager::onModuleError);
          moduleStartLock = true;
          tmpModule->startModule();
        }
      }
    }
    else
    {
      deferedStartList.enqueue(new ModuleData(0, uniqueModuleName, xmlConfigData));
    }
  }

  void ModuleManager::stopModules()
  {
    for(int i = moduleList.length(); --i>=0;)
    {
      VirtualModule *toStop = moduleList.at(i)->reference;
      QString tmpModuleName = moduleList.at(i)->uniqueName;
      toStop->stopModule();
      if(factoryTable.contains(tmpModuleName))
      {
        factoryTable.value(tmpModuleName)->destroyModule(toStop);
        qDebug() << "Deleted module:" << tmpModuleName;
      }
    }
  }

  void ModuleManager::onChangeSession(QVariant newSessionPath)
  {
    stopModules();
    emit sigSessionSwitched(newSessionPath.toString());
  }

  void ModuleManager::onModuleDelete()
  {
    VirtualModule *toDelete = qobject_cast<VirtualModule*>(QObject::sender());
    if(toDelete)
    {
      ModuleData *tmpData = ModuleData::findByReference(moduleList, toDelete);
      moduleList.removeAll(tmpData);
      delete toDelete;
      delete tmpData;
    }
    if(moduleList.isEmpty())
    {
      onDeletionFinished();
    }
  }

  void ModuleManager::onModuleStartNext()
  {
    moduleStartLock = false;
    if(deferedStartList.length()>0)
    {
      ModuleData *tmpData = deferedStartList.dequeue();
      qDebug() << "###Defered module start for"<< tmpData->uniqueName;
      startModule(tmpData->uniqueName, tmpData->configData);
      delete tmpData;
    }
  }

  void ModuleManager::onModuleError(const QString &error)
  {
    qWarning() << "Module error:" << error;
  }

  void ModuleManager::onDeletionFinished()
  {
    foreach (VeinPeer *tmpPeer, localHub->listPeers()) {
      if(tmpPeer != modManPeer)
      {
        localHub->peerRemove(tmpPeer); //will be deleted by the hub
      }
    }
  }
}
