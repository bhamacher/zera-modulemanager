#include "licensesystem.h"


#include <vcb_opensslsignaturehandler.h>
QT_BEGIN_NAMESPACE
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
QT_END_NAMESPACE

LicenseSystem::LicenseSystem(const QSet<QUrl> &t_licenseURLs, QObject *t_parent) : VeinEvent::EventSystem(t_parent),
  m_licenseURLs(t_licenseURLs),
  m_certData(loadCertData())
{
  VeinCryptoBridge::OpenSSLSignatureHandler sigHandler;
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
      licenseJsonData = sigHandler.verifyCMSSignature(m_certData, licenseFileData, &isVerified);
      if(isVerified)
      {
        QJsonParseError parseError;
        QJsonDocument licenseDocument = QJsonDocument::fromJson(licenseJsonData, &parseError);
        if(parseError.error == QJsonParseError::NoError)
        {
          const QJsonObject rootObj = licenseDocument.object();
          const bool isUniversalLicense = (rootObj.contains(s_universalLicenseDescriptor) && rootObj.value(s_universalLicenseDescriptor).toInt(0) == 1);
          Q_ASSERT(rootObj.contains(s_expiresDescriptor));
          Q_ASSERT(rootObj.contains(s_deviceSerialDescriptor));
          Q_ASSERT(isUniversalLicense || rootObj.contains(s_systemNameDescriptor));


          //if(rootObj.value(s_deviceSerialDescriptor).toString() == getDeviceSerial())
          //{

          const QString expiryMonth = rootObj.value(s_expiresDescriptor).toString();
          if(expiryMonth == s_expiresNeverDescriptor || QDateTime::fromString(expiryMonth, "yyyy/MM") <= QDateTime::currentDateTime().addMonths(1))
          {
            m_systemConfigurationTable.insert(rootObj.value(s_systemNameDescriptor).toString(), rootObj.toVariantMap());
            m_licensedSystems.append(rootObj.value(s_systemNameDescriptor).toString());
            if(isUniversalLicense)
            {
              m_universalLicenseFound = true;
              //break;
            }
          }
          else
          {
            qWarning() << "License expired:" << licenseFilePath << "\n" << "date:" << rootObj.value(s_expiresDescriptor).toString();
          }
          //}
          //else
          //{
          //  qWarning() << "License serial number is invalid:" << licenseFilePath << "\n" << "license serial:" <<  rootObj.value(s_deviceSerialDescriptor).toString() << "expected serial:" << getDeviceSerial();
          //}
        }
        else
        {
          qWarning() << "Error parsing license document:" << licenseFilePath << "\n" << "parse error:" << parseError.errorString();
        }
      }
    }
  }
}

bool LicenseSystem::isSystemLicensed(const QString &t_uniqueModuleName) const
{
  return m_universalLicenseFound || m_licensedSystems.contains(t_uniqueModuleName);
}

QVariantMap LicenseSystem::systemLicenseConfiguration(const QString &t_systemName) const
{
  return m_systemConfigurationTable.value(t_systemName);
}

QByteArray LicenseSystem::loadCertData() const
{
  QByteArray retVal;
  QFile certFile(":/license_cert.pem"); //do not use paths from the regular filesystem, with a replaced custom license_cert.pem licenses could be forged
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

//constexpr definition, see: https://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char
constexpr QLatin1String LicenseSystem::s_systemNameDescriptor;
constexpr QLatin1String LicenseSystem::s_expiresDescriptor;
constexpr QLatin1String LicenseSystem::s_expiresNeverDescriptor;
constexpr QLatin1String LicenseSystem::s_deviceSerialDescriptor;
constexpr QLatin1String LicenseSystem::s_universalLicenseDescriptor;

