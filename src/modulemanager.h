#ifndef MODULELOADER_H
#define MODULELOADER_H

#include <QObject>
#include <virtualmodule.h>
#include <QVariant>
#include <QHash>
#include <QQueue>
#include <QTimer>


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

namespace VeinScript
{
  class ScriptSystem;
}

class ModuleEventHandler;

namespace ZeraModules
{
  class ModuleData;

  class ModuleManager : public QObject
  {
    Q_OBJECT
  public:
    explicit ModuleManager(QObject *t_parent = 0);
    ~ModuleManager();
    bool isModuleLicensed(VirtualModule *t_module) const;
    bool loadModules();
    void loadDefaultSession();
    void loadScripts(VeinScript::ScriptSystem *t_scriptSystem);
    void setStorage(VeinEvent::StorageSystem *t_storage);
    void setEventHandler(ModuleEventHandler *t_eventHandler);

  signals:
    void sigSessionSwitched(const QString &t_newSessionPath);
    void sigModulesLoaded(const QString &t_sessionPath);

  public slots:
    void startModule(const QString &t_uniqueName, const QString &t_xmlConfigPath, const QByteArray &t_xmlConfigData, int t_moduleId);
    void stopModules();
    void onChangeSession(QVariant t_newSessionPath);

    void setModulesPaused(bool t_paused);

  private slots:
    void onModuleDelete();
    void onModuleStartNext();
    void onModuleError(const QString &t_error);
    void checkModuleList();

    void onModuleEventSystemAdded(VeinEvent::EventSystem *t_eventSystem);

  private:
    void saveModuleConfig(ModuleData *t_moduleData);

    QHash<QString, MeasurementModuleFactory*> m_factoryTable;
    QList<ModuleData *> m_moduleList;
    QQueue<ModuleData *> m_deferedStartList;
    Zera::Proxy::cProxy * m_proxyInstance;

    VeinEvent::StorageSystem *m_storage=0;
    ModuleEventHandler *m_eventHandler=0;

    QString m_sessionPath;
    QList<QString> m_sessionsAvailable;

    bool m_moduleStartLock;
  };
}

#endif // MODULELOADER_H
