#include "modulemanagercontroller.h"

#include <ve_eventdata.h>
#include <ve_commandevent.h>
#include <ve_storagesystem.h>
#include <vcmp_componentdata.h>
#include <vcmp_entitydata.h>

ModuleManagerController::ModuleManagerController(QObject *t_parent) : VeinEvent::EventSystem(t_parent)
{

}

void ModuleManagerController::setEntityId(int t_entityId)
{
  m_entityId=t_entityId;
}

int ModuleManagerController::getEntityId() const
{
  return m_entityId;
}

VeinEvent::StorageSystem *ModuleManagerController::getStorageSystem() const
{
  return m_storageSystem;
}

void ModuleManagerController::setStorage(VeinEvent::StorageSystem *t_storageSystem)
{
  m_storageSystem = t_storageSystem;
}

bool ModuleManagerController::processEvent(QEvent *t_event)
{
  bool retVal = false;


  if(t_event->type() == VeinEvent::CommandEvent::eventType())
  {
    VeinEvent::CommandEvent *cEvent = 0;
    cEvent = static_cast<VeinEvent::CommandEvent *>(t_event);
    if(cEvent != 0 && cEvent->eventData()->entityId()==m_entityId)
    {
      switch(cEvent->eventSubtype())
      {
        case VeinEvent::CommandEvent::EventSubtype::TRANSACTION:
        {
          retVal = true;
          cEvent->setEventSubtype(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION);
          cEvent->eventData()->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL); //the validated answer is authored from the system that runs the validator (aka. this system)
          cEvent->eventData()->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL); //inform all users (may or may not result in network messages)
          break;
        }
        default:
          break;
      }
    }
  }
  return retVal;
}





void ModuleManagerController::initializeEntities()
{
  if(m_storageSystem!=0)
  {
    VeinComponent::EntityData *systemData = new VeinComponent::EntityData();
    systemData->setCommand(VeinComponent::EntityData::Command::ECMD_ADD);
    systemData->setEntityId(0);

    VeinEvent::CommandEvent *systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::TRANSACTION, systemData);

    emit sigSendEvent(systemEvent);
    systemEvent=0;
    systemData=0;

    VeinComponent::ComponentData *introspectionData=0;

    introspectionData = new VeinComponent::ComponentData();
    introspectionData->setEntityId(0);
    introspectionData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
    introspectionData->setComponentName("EntityName");
    introspectionData->setNewValue("_System");

    systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::TRANSACTION, introspectionData);
    emit sigSendEvent(systemEvent);
    systemEvent = 0;

    introspectionData = new VeinComponent::ComponentData();
    introspectionData->setEntityId(0);
    introspectionData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
    introspectionData->setComponentName("Entities");
    qDebug() << "ENTITIES" << m_storageSystem->getEntityList() << QVariant::fromValue<QList<int> >(m_storageSystem->getEntityList()).value<QList<int> >();
    introspectionData->setNewValue(QVariant::fromValue<QList<int> >(m_storageSystem->getEntityList()));

    systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::TRANSACTION, introspectionData);
    emit sigSendEvent(systemEvent);
  }
  else
  {
    qCritical() << "[ModuleManagerController] StorageSystem required to call initializeEntities";
  }
}

