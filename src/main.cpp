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

    if (true)
    {
        //QString filename = "/etc/zera/modules/rangemodule.xml";
        QString filename = "/home/peter/C++/zera-classes/zera-modules/rangemodule/src/rangemodule.xml";
        QByteArray xmlConfigdata;
        QFile xmlConfigfile(filename);

        xmlConfigfile.open(QIODevice::Unbuffered | QIODevice::ReadOnly);
        xmlConfigdata = xmlConfigfile.readAll();
        xmlConfigfile.close();
        modMan->startModule("rangemodule",xmlConfigdata);
    }

    if (true)
    {
        //QString filename = "/etc/zera/modules/rmsmodule.xml";
        QString filename2 = "/home/peter/C++/zera-classes/zera-modules/rmsmodule/src/rmsmodule.xml";
        QByteArray xmlConfigdata2;
        QFile xmlConfigfile2(filename2);

        xmlConfigfile2.open(QIODevice::Unbuffered | QIODevice::ReadOnly);
        xmlConfigdata2 = xmlConfigfile2.readAll();
        xmlConfigfile2.close();
        modMan->startModule("rmsmodule",xmlConfigdata2);
    }

    if (true)
    {
        //QString filename = "/etc/zera/modules/dftmodule.xml";
        QString filename3 = "/home/peter/C++/zera-classes/zera-modules/dftmodule/src/dftmodule.xml";
        QByteArray xmlConfigdata3;
        QFile xmlConfigfile3(filename3);

        xmlConfigfile3.open(QIODevice::Unbuffered | QIODevice::ReadOnly);
        xmlConfigdata3 = xmlConfigfile3.readAll();
        xmlConfigfile3.close();
        modMan->startModule("dftmodule",xmlConfigdata3);
    }


//  }

  //Q_ASSERT(modulesFound);

  vTC->startService(12000);
  return a.exec();
}
