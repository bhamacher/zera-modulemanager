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
}

class ModuleEventHandler;

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
    void setEventHandler(ModuleEventHandler *t_eventHandler);

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
    QHash<QString, MeasurementModuleFactory*> factoryTable;
    QList<ModuleData *> moduleList;
    QQueue<ModuleData *> deferedStartList;
    Zera::Proxy::cProxy *proxyInstance;

    VeinEvent::StorageSystem *m_storage=0;
    ModuleEventHandler *m_eventHandler=0;

    QList<QString> m_sessionsAvailable;

    bool moduleStartLock;
  };
}

#endif // MODULELOADER_H
