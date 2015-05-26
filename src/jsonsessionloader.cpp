#include "jsonsessionloader.h"

#include <QFile>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

JsonSessionLoader::JsonSessionLoader(QObject *qObjParent) :
  QObject(qObjParent)
{
}

const QString &JsonSessionLoader::getSessionFilePath() const
{
  return m_currentSessionFile;
}

void JsonSessionLoader::loadSession(QString filePath)
{
  QFile sesFile;
  sesFile.setFileName(filePath);

  if(sesFile.exists() && sesFile.open(QIODevice::Unbuffered | QIODevice::ReadOnly))
  {
    qDebug() << "loading session:" << filePath;
    QByteArray sesFileContent;
    QJsonDocument jsonSession;
    QJsonParseError jsonError;

    sesFileContent = sesFile.readAll();
    sesFile.close();

    jsonSession = QJsonDocument::fromJson(sesFileContent, &jsonError);
    if(jsonError.error == QJsonParseError::NoError)
    {
      if(jsonSession.isObject())
      {
        QJsonObject rootObj = jsonSession.object();
        QJsonValue tmpModules = rootObj.value("modules");
        if(tmpModules.isArray())
        {
          QJsonArray moduleArray = tmpModules.toArray();
          for(int i = 0; i < moduleArray.count(); i++)
          {
            QJsonValue tmpObject = moduleArray.at(i);
            if(tmpObject.isObject())
            {
              QJsonObject moduleObject = tmpObject.toObject();
              QString tmpNameString, tmpConfigString;
              QFile tmpXmlConfigFile;
              QByteArray xmlConfigData;

              tmpNameString = moduleObject.value("name").toString();
              tmpConfigString = moduleObject.value("configPath").toString();

              tmpXmlConfigFile.setFileName(tmpConfigString);
              if(tmpXmlConfigFile.exists() && tmpXmlConfigFile.open(QIODevice::Unbuffered | QIODevice::ReadOnly))
              {
                xmlConfigData = tmpXmlConfigFile.readAll();
                tmpXmlConfigFile.close();
                emit sigLoadModule(tmpNameString, xmlConfigData);
              }
              else
              {
                qDebug() << "Error opening config file for module:" << tmpNameString << "path:" << tmpConfigString;
              }
            }
          }
        }
        else
        {
          qDebug() << "Error parsing session file:" << filePath << "content mismatch";
        }
      }
    }
    else
    {
      qDebug() << "Error parsing session file:" << filePath << "error:" << jsonError.errorString();
    }
  }
  else
  {
    qDebug() << "Error opening session file:" << filePath;
  }
}
