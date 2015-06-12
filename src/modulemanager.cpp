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
      foreach(ModuleData *tmpData, list)
      {
        if(tmpData->reference == ref)
        {
          retVal = tmpData;
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
    QStringList sessionExtension("*.json");
    QDir directory(SESSION_PATH);
    m_sessionsAvailable = directory.entryList(sessionExtension);
    qDebug() << "sessions available:" << m_sessionsAvailable;
  }

  ModuleManager::~ModuleManager()
  {
    foreach(ModuleData *toDelete, moduleList)
    {
      delete toDelete->reference;
    }
    proxyInstance->deleteLater();
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

    moduleDir = MODMAN_MODULE_PATH;

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

  void ModuleManager::loadDefaultSession()
  {
    sessionSwitchEntity->setValue("0_default-session.json"); ///< @todo remove hardcoded and add code for lastsession
  }



  void ModuleManager::setHub(VeinHub *vHub)
  {
    if(vHub)
    {
      localHub = vHub;
      modManPeer = localHub->peerAdd("ModuleManager"); ///< @todo remove hardcoded
      sessionSwitchEntity = modManPeer->dataAdd("SessionFile");
      sessionReadyEntity = modManPeer->dataAdd("SessionReady");
      sessionListEntity = modManPeer->dataAdd("SessionList");

      sessionSwitchEntity->modifiersAdd(VeinEntity::MOD_NOECHO);

      sessionReadyEntity->setValue(false, modManPeer);
      sessionReadyEntity->modifiersAdd(VeinEntity::MOD_READONLY);
      sessionReadyEntity->modifiersAdd(VeinEntity::MOD_NOECHO);

      sessionListEntity->setValue(QVariant::fromValue<QList<QString> >(m_sessionsAvailable));
      sessionListEntity->modifiersAdd(VeinEntity::MOD_CONST);

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
        VirtualModule *tmpModule = tmpFactory->createModule(proxyInstance,tmpPeer,this);
        if(tmpModule)
        {
          if(!xmlConfigData.isNull())
          {
            tmpModule->setConfiguration(xmlConfigData);
          }
          connect(tmpModule, SIGNAL(moduleDeactivated()), this, SLOT(onModuleDelete()));
          connect(tmpModule, &VirtualModule::moduleActivated, this, &ModuleManager::onModuleStartNext);
          connect(tmpModule, &VirtualModule::moduleError, this, &ModuleManager::onModuleError);
          moduleStartLock = true;
          tmpModule->startModule();
          moduleList.append(new ModuleData(tmpModule, uniqueModuleName, QByteArray()));
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
    // do not allow starting until all modules are shut down
    if(moduleList.isEmpty() == false)
    {
      moduleStartLock = true;
      sessionReadyEntity->setValue(false, modManPeer);
      for(int i = moduleList.length()-1; i>=0; i--)
      {
        VirtualModule *toStop = moduleList.at(i)->reference;
        QString tmpModuleName = moduleList.at(i)->uniqueName;
        //toStop->stopModule();
        if(factoryTable.contains(tmpModuleName))
        {
          qDebug() << "Destroying module:" << tmpModuleName;
          factoryTable.value(tmpModuleName)->destroyModule(toStop);
        }
      }
    }
  }

  void ModuleManager::onChangeSession(QVariant newSessionPath)
  {
    QString finalSessionPath = QString("%1").arg(SESSION_PATH) + newSessionPath.toString();
    if(moduleStartLock==false && QFile::exists(finalSessionPath)) // do not mess up with state machines
    {
      stopModules();
      emit sigSessionSwitched(finalSessionPath);
    }
  }

  void ModuleManager::onModuleDelete()
  {
    VirtualModule *toDelete = qobject_cast<VirtualModule*>(QObject::sender());
    if(toDelete)
    {
      ModuleData *tmpData = ModuleData::findByReference(moduleList, toDelete);
      if(tmpData)
      {
        moduleList.removeAll(tmpData);
        qDebug() << "Deleted module:" << tmpData->uniqueName;
        connect(toDelete, &VirtualModule::destroyed, this, &ModuleManager::checkModuleList);
        toDelete->deleteLater();
        delete tmpData;
      }
      else
      {
        qWarning() << "Could not find data for VirtualModule" << toDelete;
      }
    }
    if(moduleList.isEmpty())
    {
      onDeletionFinished();

      //start modules that were unable to start while shutting down
      onModuleStartNext();
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
    else
    {
      sessionReadyEntity->setValue(true, modManPeer);
    }
  }

  void ModuleManager::onModuleError(const QString &error)
  {
    qWarning() << "Module error:" << error;
  }

  void ModuleManager::checkModuleList()
  {
    if(moduleList.isEmpty())
    {
      onDeletionFinished();

      //start modules that were unable to start while shutting down
      onModuleStartNext();
    }
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
