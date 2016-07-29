#ifndef MODULEMANAGERSETTINGS_H
#define MODULEMANAGERSETTINGS_H

#include <ve_eventsystem.h>

#include <QJsonDocument>

namespace VeinEvent {
  class StorageSystem;
}

class ModuleManagerController : public VeinEvent::EventSystem
{
  Q_OBJECT
public:
  explicit ModuleManagerController(QObject *t_parent = 0);
  int getEntityId() const;
  VeinEvent::StorageSystem *getStorageSystem() const;
  void setStorage(VeinEvent::StorageSystem *t_storageSystem);

  // EventSystem interface
public:
  bool processEvent(QEvent *t_event) override;

signals:
  void sigChangeSession(QVariant newSessionPath);

public slots:
  void initializeEntities(QString t_sessionPath);
  void initOnce();

private:

  void handleNotificationMessage(QJsonObject t_message);

  const int m_entityId = 0;
  const QString m_entityName = "_System";
  const QString m_entitynNameComponentName = "EntityName";
  const QString m_entitiesComponentName = "Entities";
  const QString m_sessionComponentName = "Session";
  const QString m_notificationMessagesComponentName = "Error_Messages";

  VeinEvent::StorageSystem *m_storageSystem = 0;
  QJsonDocument m_notificationMessages;
  QString m_currentSession;
  bool m_initDone=false;
  bool m_sessionReady=false;
};

#endif // MODULEMANAGERSETTINGS_H
