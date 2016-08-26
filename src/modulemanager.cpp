#include "modulemanager.h"

#include "moduleeventhandler.h"
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
    ModuleData(VirtualModule *t_ref, const QString &t_name, const QString &t_confPath, const QByteArray &t_confData, int t_moduleId) : _reference(t_ref), m_uniqueName(t_name), m_configPath(t_confPath), m_configData(t_confData), m_moduleId(t_moduleId) {}

    static ModuleData *findByReference(QList<ModuleData*> t_list, VirtualModule *t_ref)
    {
      ModuleData *retVal = 0;
      foreach(ModuleData *tmpData, t_list)
      {
        if(tmpData->_reference == t_ref)
        {
          retVal = tmpData;
          break;
        }
      }
      return retVal;
    }

    VirtualModule *_reference;
    const QString m_uniqueName;
    const QString m_configPath;
    QByteArray m_configData;
    int m_moduleId;
  };


  ModuleManager::ModuleManager(QObject *t_parent) :
    QObject(t_parent),
    m_proxyInstance(Zera::Proxy::cProxy::getInstance()),
    m_configBackupTimer(new QTimer(this)),
    m_moduleStartLock(false)
 {
    QStringList sessionExtension("*.json");
    QDir directory(SESSION_PATH);
    m_sessionsAvailable = directory.entryList(sessionExtension);
    qDebug() << "sessions available:" << m_sessionsAvailable;

    m_configBackupTimer->setInterval(60*1000); //1 minute interval
    connect(m_configBackupTimer, &QTimer::timeout, this, &ModuleManager::onSaveModuleConfig);
  }

  ModuleManager::~ModuleManager()
  {
    foreach(ModuleData *toDelete, m_moduleList)
    {
      saveModuleConfig(toDelete);
      delete toDelete->_reference;
    }
    m_proxyInstance->deleteLater();
  }

  bool ModuleManager::isModuleLicensed(VirtualModule *t_module) const
  {
    Q_UNUSED(t_module)
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
        m_factoryTable.insert(module->getFactoryName(), module);
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

  void ModuleManager::setEventHandler(ModuleEventHandler *t_eventHandler)
  {
    m_eventHandler = t_eventHandler;
  }

  void ModuleManager::startModule(const QString & t_uniqueModuleName, const QString & t_xmlConfigPath, const QByteArray &t_xmlConfigData, int t_moduleId)
  {
    // do not allow starting until all modules are shut down
    if(m_moduleStartLock == false)
    {
      //VeinPeer *tmpPeer=0;
      //int moduleCount = 0;
      MeasurementModuleFactory *tmpFactory=0;

      tmpFactory=m_factoryTable.value(t_uniqueModuleName);
      if(tmpFactory)
      {
        //qDebug()<<"Creating module instance:"<<tmpPeer->getName(); //<< "with config" << xmlConfigData;
        VirtualModule *tmpModule = tmpFactory->createModule(m_proxyInstance, t_moduleId, m_storage, this);
        if(tmpModule)
        {
          connect(tmpModule, &VirtualModule::addEventSystem, this, &ModuleManager::onModuleEventSystemAdded);
          if(!t_xmlConfigData.isNull())
          {
            tmpModule->setConfiguration(t_xmlConfigData);
          }
          connect(tmpModule, SIGNAL(moduleDeactivated()), this, SLOT(onModuleDelete()));
          connect(tmpModule, &VirtualModule::moduleActivated, this, &ModuleManager::onModuleStartNext);
          connect(tmpModule, &VirtualModule::moduleError, this, &ModuleManager::onModuleError);
          m_moduleStartLock = true;
          tmpModule->startModule();
          m_moduleList.append(new ModuleData(tmpModule, t_uniqueModuleName, t_xmlConfigPath, QByteArray(), t_moduleId));
        }
      }
    }
    else
    {
      m_deferedStartList.enqueue(new ModuleData(0, t_uniqueModuleName, t_xmlConfigPath, t_xmlConfigData, t_moduleId));
    }
  }

  void ModuleManager::stopModules()
  {
    if(m_moduleList.isEmpty() == false)
    {
      m_moduleStartLock = true;
      onSaveModuleConfig();
      m_configBackupTimer->stop();

      for(int i = m_moduleList.length()-1; i>=0; i--)
      {
        VirtualModule *toStop = m_moduleList.at(i)->_reference;
        QString tmpModuleName = m_moduleList.at(i)->m_uniqueName;
        //toStop->stopModule();
        if(m_factoryTable.contains(tmpModuleName))
        {
          qDebug() << "Destroying module:" << tmpModuleName;
          m_factoryTable.value(tmpModuleName)->destroyModule(toStop);
        }
      }
    }
  }

  void ModuleManager::onChangeSession(QVariant t_newSessionPath)
  {
    if(m_sessionPath != t_newSessionPath.toString())
    {
      m_sessionPath = t_newSessionPath.toString();
      QString finalSessionPath = QString("%1").arg(SESSION_PATH) + m_sessionPath;
      if(m_moduleStartLock==false && QFile::exists(finalSessionPath)) // do not mess up with state machines
      {
        m_eventHandler->clearSystems();
        stopModules();
        emit sigSessionSwitched(finalSessionPath);
      }
    }
  }

  void ModuleManager::onModuleDelete()
  {
    VirtualModule *toDelete = qobject_cast<VirtualModule*>(QObject::sender());
    if(toDelete)
    {
      ModuleData *tmpData = ModuleData::findByReference(m_moduleList, toDelete);
      if(tmpData)
      {
        m_moduleList.removeAll(tmpData);
        qDebug() << "Deleted module:" << tmpData->m_uniqueName;
        connect(toDelete, &VirtualModule::destroyed, this, &ModuleManager::checkModuleList);
        toDelete->deleteLater();
        delete tmpData;
      }
      else
      {
        qWarning() << "Could not find data for VirtualModule" << toDelete;
      }
    }
    checkModuleList();
  }

  void ModuleManager::onModuleStartNext()
  {
    m_moduleStartLock = false;
    if(m_deferedStartList.length()>0)
    {
      ModuleData *tmpData = m_deferedStartList.dequeue();
      qDebug() << "###Defered module start for"<< tmpData->m_uniqueName;
      startModule(tmpData->m_uniqueName, tmpData->m_configPath, tmpData->m_configData, tmpData->m_moduleId);
      delete tmpData;
    }
    else
    {
      emit sigModulesLoaded(m_sessionPath);
      m_configBackupTimer->start();
    }
  }

  void ModuleManager::onModuleError(const QString &t_error)
  {
    qWarning() << "Module error:" << t_error;
  }

  void ModuleManager::onSaveModuleConfig()
  {
    foreach(ModuleData *toBackup, m_moduleList)
    {
      saveModuleConfig(toBackup);
    }
  }


  void ModuleManager::checkModuleList()
  {
    if(m_moduleList.isEmpty())
    {
      //start modules that were unable to start while shutting down
      onModuleStartNext();
    }
  }

  void ModuleManager::onModuleEventSystemAdded(VeinEvent::EventSystem *t_eventSystem)
  {
    m_eventHandler->addSystem(t_eventSystem);
  }

  void ModuleManager::saveModuleConfig(ModuleData *t_moduleData)
  {
    QByteArray configData = t_moduleData->_reference->getConfiguration();

    if(configData.isEmpty() == false)
    {
      QFile f;

      f.setFileName(QString("%1").arg(t_moduleData->m_configPath));
      f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered);
      if(f.isOpen() && f.isWritable())
      {
        f.write(configData);
        f.close();
      }
    }
    else
    {
      qWarning() << "Configuration could not be retrieved from module:" << t_moduleData->m_uniqueName;
    }
  }
}
