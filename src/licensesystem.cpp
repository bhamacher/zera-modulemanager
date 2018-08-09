#include "licensesystem.h"


#include <vcb_opensslsignaturehandler.h>
QT_BEGIN_NAMESPACE
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
QT_END_NAMESPACE

LicenseSystem::LicenseSystem(const QList<QUrl> &t_licenseURLs, QObject *t_parent) : VeinEvent::EventSystem(t_parent),
  m_licenseURLs(t_licenseURLs),
  m_certData(loadCertData())
{
  for(const QUrl &licenseUrl : m_licenseURLs)
  {
    ///@todo implement network requests for license files when required
    Q_ASSERT(licenseUrl.isLocalFile());

    const QHash<QString, QByteArray> licenseTable = getLicenseFilesFromPath(licenseUrl.toLocalFile());

    for(const QString &licenseFilePath : licenseTable.keys())
    {
      const QByteArray licenseFileData = licenseTable.value(licenseFilePath);
      bool isVerified = false;
      QByteArray licenseJsonData;
      licenseJsonData = VeinCryptoBridge::OpenSSLSignatureHandler::verifyCMSSignature(m_certData, licenseFileData, &isVerified);
      if(isVerified)
      {
        QJsonParseError parseError;
        QJsonDocument licenseDocument = QJsonDocument::fromJson(licenseJsonData, &parseError);
        if(parseError.error == QJsonParseError::NoError)
        {
          const QJsonObject rootObj = licenseDocument.object();
          Q_ASSERT(rootObj.contains("entityId"));
          Q_ASSERT(rootObj.contains("expires"));


          if(rootObj.value("expires").toString() == "never") ///@todo add expiry dates check
          {
            m_systemConfigurationTable.insert(rootObj.value("entityId").toInt(), rootObj.toVariantMap());

            if(rootObj.contains("uniqueModuleName"))
            {
              m_licensedModules.append(rootObj.value("uniqueModuleName").toString());
            }
            if(rootObj.contains("universalKey") && rootObj.value("universalKey").toInt(0) == 1)
            {
              m_universalLicenseFound = true;
            }
          }
          else
          {
            qWarning() << "License expired:" << licenseFilePath << "date:" << rootObj.value("expires").toString();
          }
        }
        else
        {
          qWarning() << "Error parsing license document:" << licenseFilePath << "\nParse error:" << parseError.errorString();
        }
      }
    }
  }
}

bool LicenseSystem::isModuleLicensed(const QString &t_uniqueModuleName) const
{
  return m_universalLicenseFound || m_licensedModules.contains(t_uniqueModuleName);
}

QVariantMap LicenseSystem::systemLicenseConfiguration(int t_entityId) const
{
  return m_systemConfigurationTable.value(t_entityId);
}

QByteArray LicenseSystem::loadCertData() const
{
  QByteArray retVal;
  QFile certFile(":/license_cert.pem"); //do not use paths from the regular filesystem, with a custom license_cert licenses could be forged
  Q_ASSERT(certFile.exists());

  certFile.open(QFile::ReadOnly);
  retVal = certFile.readAll();
  certFile.close();

  return retVal;
}

QByteArray LicenseSystem::loadLicenseFile(const QString &t_filePath) const
{
  QByteArray retVal;
  QFile tmpFile(t_filePath);
  QFileInfo tmpFileInfo(tmpFile);

  if(tmpFile.exists())
  {
    //mimetype is text/plain so use file extension do distinguish licenses from text files
    if(tmpFileInfo.completeSuffix().toLower() == "p7m") //format: pkcs7 s/mime (Content-Type: multipart/signed; protocol="application/x-pkcs7-signature";)
    {
      tmpFile.open(QFile::ReadOnly);
      retVal = tmpFile.readAll();
      tmpFile.close();
    }
    else
    {
      qDebug() << "Ignored regular file:" << t_filePath << "(no .p7m extension)";
    }
  }
  return retVal;
}

QHash<QString, QByteArray> LicenseSystem::getLicenseFilesFromPath(const QString &t_path) const
{
  QFileInfo fInfo(t_path);
  QHash<QString, QByteArray> retVal;

  if(fInfo.exists())
  {
    if(fInfo.isDir())
    {
      //recursive, only lists files
      QDirIterator tmpDirIterator(t_path, QDir::Files, QDirIterator::Subdirectories);
      while(tmpDirIterator.hasNext())
      {
        const QString nextFile = tmpDirIterator.next();
        const QByteArray licData = loadLicenseFile(nextFile);
        if(licData.isNull() == false)
        {
          retVal.insert(nextFile, licData);
        }
      }
    }
    else if(fInfo.isFile())
    {
      const QByteArray licData = loadLicenseFile(t_path);
      if(licData.isNull() == false)
      {
        retVal.insert(t_path, licData);
      }
    }
  }
  else
  {
    qWarning() << "Could not load license info from path:" << t_path;
  }

  return retVal;
}

bool LicenseSystem::processEvent(QEvent *t_event)
{
  return false;
}


