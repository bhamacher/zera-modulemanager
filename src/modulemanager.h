#ifndef MODULELOADER_H
#define MODULELOADER_H

#include <QObject>
#include <virtualmodule.h>
#include <QVariant>
#include <QHash>
#include <QQueue>


class MeasurementModuleFactory;

namespace Zera
{
  namespace Proxy
  {
    class cProxy;
  }
}

namespace VeinEvent
{
  class StorageSystem;
  class EventHandler;
}

namespace ZeraModules
{
  class ModuleData;

  class ModuleManager : public QObject
  {
    Q_OBJECT
  public:
    explicit ModuleManager(QObject *qobjParent = 0);
    ~ModuleManager();
    bool isModuleLicensed(VirtualModule *module);
    bool loadModules();
    void loadDefaultSession();
    void setStorage(VeinEvent::StorageSystem *t_storage);
    void setEventHandler(VeinEvent::EventHandler *t_eventHandler);

//    void setHub(VeinHub *vHub)
//    {
//      if(vHub)
//      {
//        localHub = vHub;
//        modManPeer = localHub->peerAdd("ModuleManager"); ///< @todo remove hardcoded
//        sessionSwitchEntity = modManPeer->dataAdd("SessionFile");
//        sessionReadyEntity = modManPeer->dataAdd("SessionReady");
//        sessionListEntity = modManPeer->dataAdd("SessionList");

//        sessionSwitchEntity->modifiersAdd(VeinEntity::MOD_NOECHO);

//        sessionReadyEntity->setValue(false, modManPeer);
//        sessionReadyEntity->modifiersAdd(VeinEntity::MOD_READONLY);
//        sessionReadyEntity->modifiersAdd(VeinEntity::MOD_NOECHO);

//        sessionListEntity->setValue(QVariant::fromValue<QList<QString> >(m_sessionsAvailable));
//        sessionListEntity->modifiersAdd(VeinEntity::MOD_CONST);

//        connect(sessionSwitchEntity, SIGNAL(sigValueChanged(QVariant)), this, SLOT(onChangeSession(QVariant)));
//      }
//    }
//    VeinHub *getHub()
//    {
//      return localHub;
//    }

  signals:
    void sigSessionSwitched(QString newSessionPath);
    void sigModulesLoaded();

  public slots:
    void startModule(QString uniqueModuleName, QByteArray xmlConfigData, int moduleId);
    void stopModules();

  private slots:
    void onChangeSession(QVariant newSessionPath);
    void onModuleDelete();
    void onModuleStartNext();
    void onModuleError(const QString &error);
    void checkModuleList();

    void onModuleEventSystemAdded(VeinEvent::EventSystem *t_eventSystem);

  private:
//    void onDeletionFinished()
//    {
//      foreach (VeinPeer *tmpPeer, localHub->listPeers()) {
//        if(tmpPeer != modManPeer)
//        {
//          localHub->peerRemove(tmpPeer); //will be deleted by the hub
//        }
//      }
//    }

    QHash<QString, MeasurementModuleFactory*> factoryTable;
    QList<ModuleData *> moduleList;
    QQueue<ModuleData *> deferedStartList;
    Zera::Proxy::cProxy *proxyInstance;

    VeinEvent::StorageSystem *m_storage=0;
    VeinEvent::EventHandler *m_eventHandler=0;

    QList<VeinEvent::EventSystem *> m_modules;

//    VeinHub *localHub;
//    VeinPeer *modManPeer;
//    VeinEntity *sessionSwitchEntity;
//    VeinEntity *sessionReadyEntity;
//    VeinEntity *sessionListEntity;

    QList<QString> m_sessionsAvailable;

    bool moduleStartLock;
  };
}

#endif // MODULELOADER_H
