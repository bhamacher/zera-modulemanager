#include "modulemanager.h"

#include <proxy.h>

#include <ve_eventsystem.h>

#include <QPluginLoader>
#include <abstractmodulefactory.h>
#include <QDir>
#include <QDebug>

#include <QState>

namespace ZeraModules
{
  class ModuleData {
  public:
    ModuleData(VirtualModule *pRef, QString pName, QByteArray pConfData, int pModuleId) : reference(pRef), uniqueName(pName), configData(pConfData), moduleId(pModuleId) {}

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
    int moduleId;
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
    bool retVal = false;
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
        retVal=true;
        factoryTable.insert(module->getFactoryName(), module);
      }
      else
      {
        qDebug() << "Error string:\n" << loader.errorString();
      }
    }
    return retVal;
  }

  void ModuleManager::loadDefaultSession()
  {
    onChangeSession(QVariant("0_default-session.json"));
    //sessionSwitchEntity->setValue("0_default-session.json"); ///< @todo remove hardcoded and add code for lastsession
  }

  void ModuleManager::setStorage(VeinEvent::StorageSystem *t_storage)
  {
    m_storage = t_storage;
  }

  void ModuleManager::setEventHandler(VeinEvent::EventHandler *t_eventHandler)
  {
    m_eventHandler = t_eventHandler;
  }

  void ModuleManager::startModule(QString uniqueModuleName, QByteArray xmlConfigData, int moduleId)
  {
    if(moduleStartLock == false)
    {
      //VeinPeer *tmpPeer=0;
      //int moduleCount = 0;
      MeasurementModuleFactory *tmpFactory=0;

      tmpFactory=factoryTable.value(uniqueModuleName);
      if(tmpFactory)
      {
        //VeinEntity *tmpNameEntity=0;
        //moduleCount = tmpFactory->listModules().size();
        //tmpPeer=localHub->peerAdd(QString("%1%2").arg(uniqueModuleName).arg(moduleCount), moduleId);
        //tmpNameEntity = tmpPeer->dataAdd("EntityName");
        //tmpNameEntity->setValue(QString("%1%2").arg(uniqueModuleName).arg(moduleCount));

        //qDebug()<<"Creating module instance:"<<tmpPeer->getName(); //<< "with config" << xmlConfigData;
        VirtualModule *tmpModule = tmpFactory->createModule(proxyInstance, moduleId, m_storage, this);
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
          moduleList.append(new ModuleData(tmpModule, uniqueModuleName, QByteArray(), moduleId));
        }
      }
    }
    else
    {
      deferedStartList.enqueue(new ModuleData(0, uniqueModuleName, xmlConfigData, moduleId));
    }
  }

  void ModuleManager::stopModules()
  {
    // do not allow starting until all modules are shut down
    if(moduleList.isEmpty() == false)
    {
      moduleStartLock = true;
      //sessionReadyEntity->setValue(false, modManPeer);
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
      //onDeletionFinished();

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
      startModule(tmpData->uniqueName, tmpData->configData, tmpData->moduleId);
      delete tmpData;
    }
//    else
//    {
//      sessionReadyEntity->setValue(true, modManPeer);
//    }
  }

  void ModuleManager::onModuleError(const QString &error)
  {
    qWarning() << "Module error:" << error;
  }

  void ModuleManager::checkModuleList()
  {
    if(moduleList.isEmpty())
    {
      //onDeletionFinished();

      //start modules that were unable to start while shutting down
      onModuleStartNext();
    }
  }

  void ModuleManager::onModuleEventSystemAdded(VeinEvent::EventSystem *t_eventSystem)
  {
    t_eventSystem->attach(m_eventHandler);
    m_modules.append(t_eventSystem);
  }


}
