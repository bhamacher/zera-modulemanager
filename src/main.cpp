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

    if (true)
    {
        //QString filename = "/etc/zera/modules/power1module.xml";
        QString filename4 = "/home/peter/C++/zera-classes/zera-modules/power1module/src/power1module.xml";
        QByteArray xmlConfigdata4;
        QFile xmlConfigfile4(filename4);

        xmlConfigfile4.open(QIODevice::Unbuffered | QIODevice::ReadOnly);
        xmlConfigdata4 = xmlConfigfile4.readAll();
        xmlConfigfile4.close();
        modMan->startModule("power1module",xmlConfigdata4);
    }

    if (true)
    {
        //QString filename = "/etc/zera/modules/power1module.xml";
        QString filename5 = "/home/peter/C++/zera-classes/zera-modules/power1module/src/power1module2.xml";
        QByteArray xmlConfigdata5;
        QFile xmlConfigfile5(filename5);

        xmlConfigfile5.open(QIODevice::Unbuffered | QIODevice::ReadOnly);
        xmlConfigdata5 = xmlConfigfile5.readAll();
        xmlConfigfile5.close();
        modMan->startModule("power1module",xmlConfigdata5);
    }

    if (true)
    {
        //QString filename = "/etc/zera/modules/power1module.xml";
        QString filename6 = "/home/peter/C++/zera-classes/zera-modules/power1module/src/power1module3.xml";
        QByteArray xmlConfigdata6;
        QFile xmlConfigfile6(filename6);

        xmlConfigfile6.open(QIODevice::Unbuffered | QIODevice::ReadOnly);
        xmlConfigdata6 = xmlConfigfile6.readAll();
        xmlConfigfile6.close();
        modMan->startModule("power1module",xmlConfigdata6);
    }

    if (true)
    {
        //QString filename = "/etc/zera/modules/thdnmodule.xml";
        QString filename7 = "/home/peter/C++/zera-classes/zera-modules/thdnmodule/src/thdnmodule.xml";
        QByteArray xmlConfigdata7;
        QFile xmlConfigfile7(filename7);

        xmlConfigfile7.open(QIODevice::Unbuffered | QIODevice::ReadOnly);
        xmlConfigdata7 = xmlConfigfile7.readAll();
        xmlConfigfile7.close();
        modMan->startModule("thdnmodule",xmlConfigdata7);
    }

    if (true)
    {
        //QString filename = "/etc/zera/modules/oscimodule.xml";
        QString filename8 = "/home/peter/C++/zera-classes/zera-modules/oscimodule/src/oscimodule.xml";
        QByteArray xmlConfigdata8;
        QFile xmlConfigfile8(filename8);

        xmlConfigfile8.open(QIODevice::Unbuffered | QIODevice::ReadOnly);
        xmlConfigdata8 = xmlConfigfile8.readAll();
        xmlConfigfile8.close();
        modMan->startModule("oscimodule",xmlConfigdata8);
    }


//  }

  //Q_ASSERT(modulesFound);

  vTC->startService(12000);
  return a.exec();
}
