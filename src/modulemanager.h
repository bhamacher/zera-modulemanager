#ifndef MODULELOADER_H
#define MODULELOADER_H

#include <QObject>
#include <virtualmodule.h>
#include <QVariant>
#include <QHash>
#include <QQueue>


class MeasurementModuleFactory;
class VeinHub;
class VeinPeer;
class VeinEntity;

namespace Zera
{
  namespace Proxy
  {
    class cProxy;
  }
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
    //???void reconfigureModule(VirtualModule *module, QByteArray xmlConfigData);
    //???void restartModule(VirtualModule *module, QByteArray xmlConfigData=QByteArray())
    //???void stopModule(VirtualModule *module);
    void setHub(VeinHub *vHub);
    VeinHub *getHub();

  signals:
    void sigSessionSwitched(QString newSessionPath);

  public slots:
    void startModule(QString uniqueModuleName, QByteArray xmlConfigData);
    void stopModules();

  private slots:
    void onChangeSession(QVariant newSessionPath);
    void onModuleDelete();
    void onModuleStartNext();
    void onModuleError(const QString &error);
    void checkModuleList();

  private:
    void onDeletionFinished();

    QHash<QString, MeasurementModuleFactory*> factoryTable;
    QList<ModuleData *> moduleList;
    QQueue<ModuleData *> deferedStartList;
    Zera::Proxy::cProxy *proxyInstance;

    VeinHub *localHub;
    VeinPeer *modManPeer;
    VeinEntity *sessionSwitchEntity;
    VeinEntity *sessionReadyEntity;

    bool moduleStartLock;
  };
}

#endif // MODULELOADER_H
