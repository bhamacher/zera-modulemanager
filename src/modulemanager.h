#ifndef MODULELOADER_H
#define MODULELOADER_H

#include <QObject>
#include <virtualmodule.h>
#include <QVariant>
#include <QHash>
#include <QQueue>
#include <QTimer>


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
class MeasurementModuleFactory;
class LicenseSystem;

namespace ZeraModules
{
  class ModuleData;

  class ModuleManager : public QObject
  {
    Q_OBJECT
  public:
    explicit ModuleManager(const QStringList &t_sessionList, QObject *t_parent = nullptr);
    ~ModuleManager() override;
    bool loadModules();
    void loadScripts(VeinScript::ScriptSystem *t_scriptSystem);
    void setStorage(VeinEvent::StorageSystem *t_storage);
    void setLicenseSystem(LicenseSystem *t_licenseSystem);
    void setEventHandler(ModuleEventHandler *t_eventHandler);

  signals:
    void sigSessionSwitched(const QString &t_newSessionPath);
    void sigModulesLoaded(const QString &t_sessionPath, const QStringList &t_sessionsAvailable);

  public slots:
    void startModule(const QString &t_uniqueName, const QString &t_xmlConfigPath, const QByteArray &t_xmlConfigData, int t_moduleId);
    void stopModules();
    void changeSessionFile(const QString &t_newSessionPath);

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
    QQueue<ModuleData *> m_deferredStartList;
    Zera::Proxy::cProxy * m_proxyInstance;

    VeinEvent::StorageSystem *m_storage=nullptr;
    ModuleEventHandler *m_eventHandler=nullptr;
    LicenseSystem *m_licenseSystem=nullptr;

    QString m_sessionPath;
    QList<QString> m_sessionsAvailable;

    bool m_moduleStartLock;
  };
}

#endif // MODULELOADER_H
