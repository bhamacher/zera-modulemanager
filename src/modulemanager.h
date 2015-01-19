#ifndef MODULELOADER_H
#define MODULELOADER_H

#include <QObject>
#include <virtualmodule.h>
#include <QVariant>
#include <QHash>


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

  class ModuleManager : public QObject
  {
    Q_OBJECT
  public:
    explicit ModuleManager(QObject *qobjParent = 0);
    ~ModuleManager();
    void configureModules();
    bool isModuleLicensed(VirtualModule *module);
    bool loadModules();
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


  private:
    QHash<QString, MeasurementModuleFactory*> factoryTable;
    QHash<VirtualModule *, QString> moduleList;
    Zera::Proxy::cProxy *proxyInstance;

    VeinHub *localHub;
    VeinPeer *modManPeer;
    VeinEntity *sessionSwitchEntity;
  };
}

#endif // MODULELOADER_H
