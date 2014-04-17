#include "modulemanager.h"

#include <QCoreApplication>
#include <veintcpcontroller.h>
#include <veinhub.h>
#include <QFile>
#include <QByteArray>


#include <QDebug>
#include <QFile>

int main(int argc, char *argv[])
{
  QFile xmlFile;
  QByteArray baXmlFile;

  QCoreApplication a(argc, argv);

  VeinTcpController *vTC = new VeinTcpController(&a);
  ZeraModules::ModuleManager *modMan = ZeraModules::ModuleManager::getInstance();

  VeinHub *localHub=vTC->getLocalHub();
  bool modulesFound;

  localHub->setUuid(QUuid::createUuid());
  modMan->setHub(localHub);


  modulesFound = modMan->loadModules();

  if(!modulesFound)
  {
    qDebug() << "[Zera-Module-Manager] No modules found";
  }


  //xmlFile.setFileName("/work/git-local/zera-classes/zera-modules/rangemodule/src/rangemodule.xml");
  //xmlFile.open(QFile::ReadOnly);
  //baXmlFile = xmlFile.readAll();



  // modMan->startModule("DummyModule","");
  // modMan->startModule("RangeModule", baXmlFile);

//  for(int i=0; i<1000000; i++)
//  {
    //QString filename = "/home/peter/C++/rangemodule/src/rangemodule.xml";
    // QString filename = "/home/peter/C++/zera-classes/zera-modules/rangemodule/src/rangemodule.xml";
    QString filename = "/etc/zera/modules/rangemodule.xml";

    QByteArray xmlConfigdata;
    QFile xmlConfigfile(filename);

    xmlConfigfile.open(QIODevice::Unbuffered | QIODevice::ReadOnly);
    xmlConfigdata = xmlConfigfile.readAll();
    xmlConfigfile.close();

    modMan->startModule("RangeModule",xmlConfigdata);
//  }

  //Q_ASSERT(modulesFound);

  vTC->startService(12000);
  return a.exec();
}
