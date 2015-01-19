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
  ModuleManager::ModuleManager(QObject *qobjParent) :
    QObject(qobjParent),
    proxyInstance(Zera::Proxy::cProxy::getInstance())
  {
  }

  ModuleManager::~ModuleManager()
  {
    foreach(VirtualModule *toDelete, moduleList)
    {
      delete toDelete;
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
    moduleDir = "/work/qt_projects/Zera/build-zera-classes-Desktop_Qt_5_2_1_GCC_64bit-Debug/modules/";
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
      modManPeer = localHub->peerAdd("ModuleManager");
      sessionSwitchEntity = modManPeer->dataAdd("SessionFile");
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
    VeinPeer *tmpPeer=0;
    int moduleCount = 0;
    MeasurementModuleFactory *tmpFactory=0;

    tmpFactory=factoryTable.value(uniqueModuleName);
    if(tmpFactory)
    {
      moduleCount = tmpFactory->listModules().count();
      if(moduleCount<0) //in case of an empty list
      {
        moduleCount=0;
      }
      tmpPeer=localHub->peerAdd(QString("%1%2").arg(uniqueModuleName).arg(moduleCount));
      qDebug()<<"Creating module instance:"<<tmpPeer->getName(); //<< "with config" << xmlConfigData;
      VirtualModule *tmpModule = factoryTable.value(uniqueModuleName)->createModule(proxyInstance,tmpPeer,this);
      if(tmpModule)
      {
        if(!xmlConfigData.isNull())
        {
          tmpModule->setConfiguration(xmlConfigData);
        }
        tmpModule->startModule();
        moduleList.append(tmpModule);
      }
    }
  }

  void ModuleManager::stopModules()
  {
    foreach(VirtualModule *toStop, moduleList)
    {
      toStop->stopModule();
      toStop->deleteLater();
    }
    moduleList.clear();
  }

  void ModuleManager::onChangeSession(QVariant newSessionPath)
  {
    stopModules();
    emit sigSessionSwitched(newSessionPath.toString());
  }
}
