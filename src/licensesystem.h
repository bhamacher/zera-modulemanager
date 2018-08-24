#ifndef LICENSESYSTEM_H
#define LICENSESYSTEM_H

#include <ve_eventsystem.h>

#include <QMap>
#include <QHash>
#include <QUrl>

class LicenseSystem : public VeinEvent::EventSystem
{
  Q_OBJECT
public:
  LicenseSystem(const QSet<QUrl> &t_licenseURLs, QObject *t_parent = nullptr);

  bool isModuleLicensed(const QString &t_uniqueModuleName) const;
  QVariantMap systemLicenseConfiguration(int t_entityId) const;

private:
  QByteArray loadCertData() const;
  QByteArray loadLicenseFile(const QString &t_filePath) const;
  QHash<QString, QByteArray> getLicenseFilesFromPath(const QString &t_path) const;

  QString getDeviceSerial() const;

  //allows multiple paths to load licenses from (e.g. file:///etc/zera/licenses, file:///home/$USER/licenses and http://$LICENSE_SERVER:8080/licenses)
  const QSet<QUrl> m_licenseURLs;

  //modules currently don't support configurable licensing
  QList<QString> m_licensedModules;

  //use QVariantMap for support of QML type conversion
  QHash<int, QVariantMap> m_systemConfigurationTable;

  //pubkey
  QByteArray m_certData;

  bool m_universalLicenseFound=false;

  // EventSystem interface
public:
  bool processEvent(QEvent *t_event) override;
};

#endif // LICENSESYSTEM_H
