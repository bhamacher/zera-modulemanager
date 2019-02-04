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

  bool isSystemLicensed(const QString &t_uniqueModuleName) const;
  QVariantMap systemLicenseConfiguration(const QString &t_systemName) const;

  void setDeviceSerial(const QString &t_serialNumber);

private:
  QByteArray loadCertData() const;
  QByteArray loadLicenseFile(const QString &t_filePath) const;
  QHash<QString, QByteArray> getLicenseFilesFromPath(const QString &t_path) const;

  //allows multiple paths to load licenses from (e.g. file:///etc/zera/licenses, file:///home/$USER/licenses and http://$LICENSE_SERVER:8080/licenses/$SERIALNO)
  const QSet<QUrl> m_licenseURLs;

  //modules currently don't support configurable licensing
  QList<QString> m_licensedSystems;

  //use QVariantMap for support of QML type conversion
  QHash<QString, QVariantMap> m_systemConfigurationTable;

  //signer x509 certificate
  QByteArray m_certData;

  QString m_deviceSerial;

  bool m_universalLicenseFound=false;

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
