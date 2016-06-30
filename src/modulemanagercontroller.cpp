#include "modulemanagercontroller.h"

#include <ve_eventdata.h>
#include <ve_commandevent.h>
#include <ve_storagesystem.h>
#include <vcmp_componentdata.h>
#include <vcmp_entitydata.h>
#include <vcmp_introspectiondata.h>

ModuleManagerController::ModuleManagerController(QObject *t_parent) : VeinEvent::EventSystem(t_parent)
{

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
    bool validated=false;
    VeinEvent::CommandEvent *cEvent = 0;
    cEvent = static_cast<VeinEvent::CommandEvent *>(t_event);
    if(cEvent != 0 && cEvent->eventSubtype() != VeinEvent::CommandEvent::EventSubtype::NOTIFICATION) //we do not need to process notifications
    {
      if(cEvent->eventData()->type() == VeinComponent::IntrospectionData::dataType()) //introspection requests are always valid
      {
        validated = true;
      }
      else if(cEvent->eventData()->type() == VeinComponent::EntityData::dataType()) //validate subscription requests
      {
        VeinComponent::EntityData *eData=0;
        eData = static_cast<VeinComponent::EntityData *>(cEvent->eventData());
        Q_ASSERT(eData!=0);
        if(eData->eventCommand()==VeinComponent::EntityData::Command::ECMD_SUBSCRIBE
           || eData->eventCommand()==VeinComponent::EntityData::Command::ECMD_UNSUBSCRIBE) /// @todo maybe add roles/views later
        {
          validated = true;
        }
      }
      else if(cEvent->eventData()->type() == VeinComponent::ComponentData::dataType())
      {
        VeinComponent::ComponentData *cData=0;
        cData = static_cast<VeinComponent::ComponentData *>(cEvent->eventData());
        Q_ASSERT(cData!=0);

        //validate fetch requests
        if(cData->eventCommand()==VeinComponent::ComponentData::Command::CCMD_FETCH) /// @todo maybe add roles/views later
        {
          validated = true;
        }
        else if(cData->eventCommand()==VeinComponent::ComponentData::Command::CCMD_SET //validate set event for _System.Session
           && cData->entityId() == m_entityId
           && cData->componentName() == m_sessionComponentName)
        {
          emit sigChangeSession(cData->newValue());
          validated = true;
        }
      }
    }

    if(validated == true)
    {
      retVal = true;
      cEvent->setEventSubtype(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION);
      cEvent->eventData()->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL); //the validated answer is authored from the system that runs the validator (aka. this system)
      cEvent->eventData()->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL); //inform all users (may or may not result in network messages)
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
    systemData->setEntityId(m_entityId);

    VeinEvent::CommandEvent *systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, systemData);

    emit sigSendEvent(systemEvent);
    systemEvent=0;
    systemData=0;

    VeinComponent::ComponentData *introspectionData=0;

    introspectionData = new VeinComponent::ComponentData();
    introspectionData->setEntityId(m_entityId);
    introspectionData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
    introspectionData->setComponentName(m_entitynNameComponentName);
    introspectionData->setNewValue(m_entityName);

    systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, introspectionData);
    emit sigSendEvent(systemEvent);
    systemEvent = 0;

    introspectionData = new VeinComponent::ComponentData();
    introspectionData->setEntityId(m_entityId);
    introspectionData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
    introspectionData->setComponentName(m_entitiesComponentName);
    qDebug() << "ENTITIES" << m_storageSystem->getEntityList() << QVariant::fromValue<QList<int> >(m_storageSystem->getEntityList()).value<QList<int> >();
    introspectionData->setNewValue(QVariant::fromValue<QList<int> >(m_storageSystem->getEntityList()));

    systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, introspectionData);
    emit sigSendEvent(systemEvent);

    introspectionData = new VeinComponent::ComponentData();
    introspectionData->setEntityId(m_entityId);
    introspectionData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
    introspectionData->setComponentName(m_sessionComponentName);
    introspectionData->setNewValue(QVariant("0_default-session.json"));

    systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, introspectionData);
    emit sigSendEvent(systemEvent);
  }
  else
  {
    qCritical() << "[ModuleManagerController] StorageSystem required to call initializeEntities";
  }
}

