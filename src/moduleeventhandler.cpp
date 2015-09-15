#include "moduleeventhandler.h"

#include <QEvent>

ModuleEventHandler::ModuleEventHandler(QObject *t_parent) : VeinEvent::EventHandler(t_parent)
{

}

void ModuleEventHandler::addSystem(VeinEvent::EventSystem *t_eventSystem)
{
  if(m_moduleSystems.contains(t_eventSystem) == false)
  {
    m_moduleSystems.append(t_eventSystem);
    t_eventSystem->attach(this);
  }
}

void ModuleEventHandler::clearSystems()
{
  m_moduleSystems.clear();
}

void ModuleEventHandler::customEvent(QEvent *t_event)
{
  /** @todo maybe event processing can be accelerated with QtConcurrent? */

  for(int i=0; i < m_moduleSystems.count() && t_event->isAccepted()==false; ++i)
  {
    m_moduleSystems.at(i)->processEvent(t_event);
  }

  VeinEvent::EventHandler::customEvent(t_event);
}

