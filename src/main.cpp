#include "modulemanager.h"

#include <QCoreApplication>
#include <veintcpcontroller.h>

#include <QDebug>

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  VeinTcpController *vTC = new VeinTcpController(&a);
  ZeraModules::ModuleManager *modMan = ZeraModules::ModuleManager::getInstance();

  bool modulesFound;

  modMan->setHub(vTC->getLocalHub());

  modulesFound = modMan->loadModules();

  if(!modulesFound)
  {
    qDebug() << "[Zera-Module-Manager] No modules found";
  }

  for(int i=0; i<1000000; i++)
  {
    modMan->startModule("DummyModule","");
  }

  //Q_ASSERT(modulesFound);

  return a.exec();
}
