#ifndef LICENSESYSTEM_H
#define LICENSESYSTEM_H

#include "modman_util.h"

#include <ve_eventsystem.h>

#include <QMap>
#include <QHash>
#include <QUrl>

class LicenseSystem : public VeinEvent::EventSystem
{
  Q_OBJECT
public:
  LicenseSystem(const QSet<QUrl> &t_licenseURLs, QObject *t_parent = nullptr);

  bool isSystemLicensed(const QString &t_uniqueModuleName);
  QVariantMap systemLicenseConfiguration(const QString &t_systemName) const;

  void setDeviceSerial(const QString &t_serialNumber);
  bool serialNumberIsInitialized() const;

signals:
  void sigSerialNumberInitialized();

private:
  QByteArray loadCertData() const;
  QByteArray loadLicenseFile(const QString &t_filePath) const;
  QHash<QString, QByteArray> getLicenseFilesFromPath(const QString &t_path) const;

  bool isValidLicenseExpiryDate(const QString t_dateString) const;
  bool isValidLicenseDeviceSerial(const QString t_deviceSerial) const;

  ///@todo allows multiple paths to load licenses from, but lacks implementations for them (e.g. http://$LICENSE_SERVER:8080/licenses/$SERIALNO)
  const QSet<QUrl> m_licenseURLs;

  //modules currently don't support configurable licensing
  QList<QString> m_licensedSystems;

  //store if a system fails the checks
  QList<QString> m_unlicensedSystems;

  //use QVariantMap for support of QML type conversion
  ///@note verified only means that the file and signature is valid, not that the license has correct expiry date and serial number
  QHash<QString, QVariantMap> m_verifiedLicenseDataTable;

  //signer x509 certificate
  QByteArray m_certData;

  QString m_deviceSerial;

  bool m_universalLicenseFound=false;
  bool m_serialNumberInitialized=false;

  static constexpr QLatin1String s_systemNameDescriptor = modman_util::to_latin1("uniqueSystemName");
  static constexpr QLatin1String s_expiresDescriptor = modman_util::to_latin1("expires");
  static constexpr QLatin1String s_expiresNeverDescriptor = modman_util::to_latin1("never");
  static constexpr QLatin1String s_deviceSerialDescriptor = modman_util::to_latin1("deviceSerial");
  static constexpr QLatin1String s_universalLicenseDescriptor = modman_util::to_latin1("universalLicense");
  static constexpr QLatin1String s_universalSerialDescriptor = modman_util::to_latin1("universalSerial");

  // EventSystem interface
public:
  bool processEvent(QEvent *t_event) override;
};

#endif // LICENSESYSTEM_H
