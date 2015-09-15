#ifndef MODULEMANAGERSETTINGS_H
#define MODULEMANAGERSETTINGS_H

#include <ve_eventsystem.h>

namespace VeinEvent {
  class StorageSystem;
}

class ModuleManagerController : public VeinEvent::EventSystem
{
  Q_OBJECT
public:
  explicit ModuleManagerController(QObject *t_parent = 0);
  void setEntityId(int t_entityId);
  int getEntityId() const;
  VeinEvent::StorageSystem *getStorageSystem() const;
  void setStorage(VeinEvent::StorageSystem *t_storageSystem);

  // EventSystem interface
public:
  bool processEvent(QEvent *t_event) override;

signals:

public slots:
  void initializeEntities();

private:
  int m_entityId=0;
  VeinEvent::StorageSystem *m_storageSystem=0;

};

#endif // MODULEMANAGERSETTINGS_H
