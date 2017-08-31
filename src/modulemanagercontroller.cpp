#include "modulemanagercontroller.h"

#include <ve_eventdata.h>
#include <ve_commandevent.h>
#include <ve_storagesystem.h>
#include <vcmp_componentdata.h>
#include <vcmp_entitydata.h>
#include <vcmp_introspectiondata.h>

#include <QJsonArray>
#include <QDateTime>

ModuleManagerController::ModuleManagerController(QObject *t_parent) :
  VeinEvent::EventSystem(t_parent)
{
}

int ModuleManagerController::getEntityId() const
{
  return s_entityId;
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
        if(eData->eventCommand() == VeinComponent::EntityData::Command::ECMD_SUBSCRIBE
           || eData->eventCommand() == VeinComponent::EntityData::Command::ECMD_UNSUBSCRIBE) /// @todo maybe add roles/views later
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
        if(cData->eventCommand() == VeinComponent::ComponentData::Command::CCMD_FETCH) /// @todo maybe add roles/views later
        {
          validated = true;
        }
        else if(cData->eventCommand() == VeinComponent::ComponentData::Command::CCMD_SET && //validate set event for _System.Session
                cData->entityId() == s_entityId)
        {
          if(cData->componentName() == ModuleManagerController::s_sessionComponentName)
          {
            //m_currentSession=cData->newValue().toString();
            if(m_sessionReady == true)
            {
              emit sigChangeSession(cData->newValue());
              m_sessionReady = false;
            }
            if(cData->eventOrigin() != VeinEvent::EventData::EventOrigin::EO_LOCAL)
            {
              t_event->accept();
            }
          }
          else if(cData->componentName() == ModuleManagerController::s_notificationMessagesComponentName)
          {
            handleNotificationMessage(cData->newValue().toJsonObject());
            t_event->accept();
          }
          else if(cData->componentName() == ModuleManagerController::s_loggedComponentsComponentName)
          {
            VeinComponent::ComponentData *loggedComponentsData = new VeinComponent::ComponentData();
            loggedComponentsData->setEntityId(ModuleManagerController::s_entityId);
            loggedComponentsData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            loggedComponentsData->setComponentName(ModuleManagerController::s_loggedComponentsComponentName);
            loggedComponentsData->setNewValue(cData->newValue());
            loggedComponentsData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            loggedComponentsData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, loggedComponentsData));
          }
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





void ModuleManagerController::initializeEntities(QString t_sessionPath)
{
  if(m_storageSystem!=0)
  {
    m_sessionReady=true;
    m_currentSession=t_sessionPath;

    VeinComponent::ComponentData *initData=0;
    VeinEvent::CommandEvent *initEvent = 0;


    initData = new VeinComponent::ComponentData();
    initData->setEntityId(s_entityId);
    initData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
    initData->setComponentName(ModuleManagerController::s_entitiesComponentName);
    initData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
    initData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
    qDebug() << "ENTITIES" << m_storageSystem->getEntityList() << QVariant::fromValue<QList<int> >(m_storageSystem->getEntityList()).value<QList<int> >();
    initData->setNewValue(QVariant::fromValue<QList<int> >(m_storageSystem->getEntityList()));

    initEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, initData);
    emit sigSendEvent(initEvent);
    initEvent=0;


    initData = new VeinComponent::ComponentData();
    initData->setEntityId(s_entityId);
    initData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
    initData->setComponentName(ModuleManagerController::s_sessionComponentName);
    initData->setNewValue(QVariant(m_currentSession));
    initData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
    initData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
    initEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, initData);
    emit sigSendEvent(initEvent);
  }
  else
  {
    qCritical() << "[ModuleManagerController] StorageSystem required to call initializeEntities";
  }
}

void ModuleManagerController::initOnce()
{
  if(m_initDone == false)
  {
    VeinComponent::EntityData *systemData = new VeinComponent::EntityData();
    systemData->setCommand(VeinComponent::EntityData::Command::ECMD_ADD);
    systemData->setEntityId(s_entityId);

    emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, systemData));

    QHash<QString, QVariant> componentData;
    componentData.insert(ModuleManagerController::s_entityNameComponentName, ModuleManagerController::s_entityName);
    componentData.insert(ModuleManagerController::s_entitiesComponentName, QVariant());
    componentData.insert(ModuleManagerController::s_sessionComponentName, QVariant(m_currentSession));
    componentData.insert(ModuleManagerController::s_notificationMessagesComponentName, QVariant(m_notificationMessages.toJson()));
    componentData.insert(ModuleManagerController::s_loggedComponentsComponentName, QVariantMap());

    VeinComponent::ComponentData *introspectionData=0;
    for(const QString &compName : componentData.keys())
    {
      introspectionData = new VeinComponent::ComponentData();
      introspectionData->setEntityId(s_entityId);
      introspectionData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
      introspectionData->setComponentName(compName);
      introspectionData->setNewValue(componentData.value(compName));
      introspectionData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
      introspectionData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
      emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, introspectionData));
    }


    m_initDone = true;
  }
}

void ModuleManagerController::handleNotificationMessage(QJsonObject t_message)
{
  Q_ASSERT(t_message.isEmpty() == false);
  VeinComponent::ComponentData *notificationMessagesData = new VeinComponent::ComponentData();
  VeinEvent::CommandEvent *emDataEvent = 0;
  notificationMessagesData->setEntityId(s_entityId);
  notificationMessagesData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
  notificationMessagesData->setComponentName(s_notificationMessagesComponentName);
  notificationMessagesData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
  notificationMessagesData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

  QJsonArray tmpArray = m_notificationMessages.array();
  tmpArray.append(t_message);
  m_notificationMessages.setArray(tmpArray);
  notificationMessagesData->setNewValue(m_notificationMessages.toJson());

  emDataEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, notificationMessagesData);
  emit sigSendEvent(emDataEvent);
}

