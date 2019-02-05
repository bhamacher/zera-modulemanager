#include "modulemanager.h"

#include "moduleeventhandler.h"
#include "licensesystem.h"
#include <proxy.h>

#include <ve_eventsystem.h>
#include <vsc_scriptsystem.h>

#include <QPluginLoader>
#include <abstractmodulefactory.h>
#include <QDir>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QDebug>


namespace ZeraModules
{
  class ModuleData {
  public:
    ModuleData(VirtualModule *t_ref, const QString &t_name, const QString &t_confPath, const QByteArray &t_confData, int t_moduleId) :
      m_reference(t_ref),
      m_uniqueName(t_name),
      m_configPath(t_confPath),
      m_configData(t_confData),
      m_moduleId(t_moduleId)
    {}

    ~ModuleData() {}

    static ModuleData *findByReference(QList<ModuleData*> t_list, VirtualModule *t_ref)
    {
      ModuleData *retVal = 0;
      foreach(ModuleData *tmpData, t_list)
      {
        if(tmpData->m_reference == t_ref)
        {
          retVal = tmpData;
          break;
        }
      }
      return retVal;
    }

    VirtualModule *m_reference;
    const QString m_uniqueName;
    const QString m_configPath;
    QByteArray m_configData;
    int m_moduleId;
  };


  ModuleManager::ModuleManager(const QStringList &t_sessionList, QObject *t_parent) :
    QObject(t_parent),
    m_proxyInstance(Zera::Proxy::cProxy::getInstance()),
    m_moduleStartLock(false)
  {
    QStringList sessionExtension("*.json");
    QDir directory(MODMAN_SESSION_PATH);
    QSet<QString> fileSet, expectedSet;
    fileSet = directory.entryList(sessionExtension).toSet();
    expectedSet = t_sessionList.toSet();
    if(fileSet.contains(expectedSet))
    {
      m_sessionsAvailable = t_sessionList;
    }
    else
    {
      QSet<QString> missingSessions = expectedSet;
      missingSessions.subtract(fileSet);
      qCritical() << "Missing session file(s)" << missingSessions;
    }
    qDebug() << "sessions available:" << m_sessionsAvailable;
  }

  ModuleManager::~ModuleManager()
  {
    foreach(ModuleData *toDelete, m_moduleList)
    {
      m_factoryTable.value(toDelete->m_uniqueName)->destroyModule(toDelete->m_reference);
      delete toDelete;
    }
    m_moduleList.clear();
    m_proxyInstance->deleteLater();
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
      QPluginLoader loader(moduleDir.absoluteFilePath(fileName));
      MeasurementModuleFactory *module = qobject_cast<MeasurementModuleFactory *>(loader.instance());
      qDebug() << "Analyzing:" << loader.fileName() << "\nfile is a library?" << QLibrary::isLibrary(moduleDir.absoluteFilePath(fileName)) << "loaded:" << loader.isLoaded();
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

  void ModuleManager::loadScripts(VeinScript::ScriptSystem *t_scriptSystem)
  {
    //load builtin scripts
    const QDir virtualFiles = QDir(":/scripts");
    const QStringList scriptList = virtualFiles.entryList();
    for(const QString &scriptFilePath : scriptList)
    {
      const QString dataLocation = QString("%1/%2").arg(virtualFiles.path()).arg(scriptFilePath);
      qDebug() << "Loading script:" << dataLocation;
      if(t_scriptSystem->loadScriptFromFile(dataLocation) == false)
      {
        qWarning() << "Error loading script file:" << scriptFilePath;
      }
    }
  }

  void ModuleManager::setStorage(VeinEvent::StorageSystem *t_storage)
  {
    m_storage = t_storage;
  }

  void ModuleManager::setLicenseSystem(LicenseSystem *t_licenseSystem)
  {
    ///@todo move to constructor as the ModuleManager depends on the LicenseSystem
    Q_ASSERT(t_licenseSystem != nullptr);
    m_licenseSystem = t_licenseSystem;
    //start the next module as soon as the INF_SerialNr component is avaiable
    connect(m_licenseSystem, &LicenseSystem::sigSerialNumberInitialized, this, &ModuleManager::delayedModuleStartNext);
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
      MeasurementModuleFactory *tmpFactory=nullptr;

      tmpFactory=m_factoryTable.value(t_uniqueModuleName);
      if(tmpFactory && m_licenseSystem->isSystemLicensed(t_uniqueModuleName))
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
          connect(tmpModule, &VirtualModule::moduleActivated, this, [this](){
            m_moduleStartLock=false;
            delayedModuleStartNext();
          });
          connect(tmpModule, &VirtualModule::moduleError, this, &ModuleManager::onModuleError);

          m_moduleStartLock = true;
          tmpModule->startModule();
          ModuleData *moduleData = new ModuleData(tmpModule, t_uniqueModuleName, t_xmlConfigPath, QByteArray(), t_moduleId);
          connect(tmpModule, &VirtualModule::parameterChanged, [this, moduleData](){
            saveModuleConfig(moduleData);
          });
          m_moduleList.append(moduleData);
        }
      }
      else
      {
        qWarning() << "Skipping module:" << t_uniqueModuleName << "No license found!";
        onModuleStartNext();
      }
    }
    else
    {
      m_deferredStartList.enqueue(new ModuleData(0, t_uniqueModuleName, t_xmlConfigPath, t_xmlConfigData, t_moduleId));
    }
  }

  void ModuleManager::stopModules()
  {
    if(m_moduleList.isEmpty() == false)
    {
      m_moduleStartLock = true;

      for(int i = m_moduleList.length()-1; i>=0; i--)
      {
        VirtualModule *toStop = m_moduleList.at(i)->m_reference;
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

  void ModuleManager::changeSessionFile(const QString &t_newSessionPath)
  {
    if(m_sessionPath != t_newSessionPath)
    {
      const QString finalSessionPath = QString("%1%2").arg(MODMAN_SESSION_PATH).arg(t_newSessionPath);
      if(QFile::exists(finalSessionPath))
      {
        if(m_moduleStartLock == false) // do not mess up the state machines
        {
          m_sessionPath = t_newSessionPath;
          m_eventHandler->clearSystems();
          stopModules();
          emit sigSessionSwitched(finalSessionPath);
        }
        else
        {
          qWarning() << "Cannot switch sessions while session change already is in progress";
          Q_ASSERT(false);
        }
      }
      else
      {
        qWarning() << "Session file not found:" << finalSessionPath << "Search path:" << MODMAN_SESSION_PATH;
      }
    }
  }

  void ModuleManager::setModulesPaused(bool t_paused)
  {
    for(ModuleData *module : m_moduleList)
    {
      if(t_paused)
      {
        qDebug() << "pausing module:" << module->m_uniqueName;
        module->m_reference->stopModule();
      }
      else
      {
        qDebug() << "unpausing module:" << module->m_uniqueName;
        module->m_reference->startModule();
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

  void ModuleManager::delayedModuleStartNext()
  {
    if(m_licenseSystem->serialNumberIsInitialized() == true && m_deferredStartList.length()>0 && m_moduleStartLock == false)
    {
      onModuleStartNext();
    }
  }

  void ModuleManager::onModuleStartNext()
  {
    m_moduleStartLock = false;
    if(m_deferredStartList.length()>0)
    {
      ModuleData *tmpData = m_deferredStartList.dequeue();
      qDebug() << "###deferred module start for" << tmpData->m_uniqueName;
      startModule(tmpData->m_uniqueName, tmpData->m_configPath, tmpData->m_configData, tmpData->m_moduleId);
      delete tmpData;
    }
    else
    {
      emit sigModulesLoaded(m_sessionPath, m_sessionsAvailable);
    }
  }

  void ModuleManager::onModuleError(const QString &t_error)
  {
    qWarning() << "Module error:" << t_error;
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
    QByteArray configData = t_moduleData->m_reference->getConfiguration();

    if(configData.isEmpty() == false)
    {
      QSaveFile f; //if the application is closed while writing it will not end up with empty files due to truncate

      f.setFileName(t_moduleData->m_configPath);
      //qDebug() << "Storing configuration for module:" << t_moduleData->m_uniqueName << "in file:" << f.fileName();
      f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered);
      if(f.isOpen() && f.isWritable())
      {
        f.write(configData);
        f.commit(); //writes into the permanent file and closes the file descriptors
      }
      else
      {
        qWarning() << "Failed to write configuration file:" << f.fileName() << "error:" << f.errorString();
      }
    }
    else
    {
      qWarning() << "Configuration could not be retrieved from module:" << t_moduleData->m_uniqueName;
    }
  }
}
