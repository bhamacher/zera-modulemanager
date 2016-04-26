#ifndef MODULEEVENTHANDLER_H
#define MODULEEVENTHANDLER_H

#include <ve_eventhandler.h>
#include <ve_eventsystem.h>
#include <QList>

class ModuleEventHandler : public VeinEvent::EventHandler
{
  Q_OBJECT
public:
  explicit ModuleEventHandler(QObject *t_parent = 0);

signals:

public slots:
  void addSystem(VeinEvent::EventSystem *t_eventSystem);
  void clearSystems();

  // QObject interface
protected:
  void customEvent(QEvent *t_event) override;

private:
  QList<VeinEvent::EventSystem *> m_moduleSystems;
};

#endif // MODULEEVENTHANDLER_H