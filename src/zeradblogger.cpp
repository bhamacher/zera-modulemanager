#include "zeradblogger.h"
#include "diriteratorworker.h"

#include <vcmp_componentdata.h>
#include <vcmp_remoteproceduredata.h>
#include <vcmp_errordata.h>
#include <ve_commandevent.h>

#include <QStorageInfo>
#include <QtConcurrent>
#include <QDir>
#include <QDirIterator>
#include <QThread>

#include <functional>

class ZeraDBLoggerPrivate
{

    enum RPCResultCodes {
        RPC_CANCELED = -64,
        RPC_EINVAL = -EINVAL, //invalid parameters
        RPC_SUCCESS = 0
    };

    ZeraDBLoggerPrivate(ZeraDBLogger *t_qPtr) :
        m_qPtr(t_qPtr),
        m_remoteProcedures{ VF_RPC_BIND(listStorages, std::bind(&ZeraDBLoggerPrivate::listStorages, this, std::placeholders::_1, std::placeholders::_2)),
                            VF_RPC_BIND(findDBFile, std::bind(&ZeraDBLoggerPrivate::findDBFile, this, std::placeholders::_1, std::placeholders::_2)) }
    {

    }

    void initEntity()
    {
        VeinComponent::ComponentData *recordNameData = new VeinComponent::ComponentData();
        recordNameData->setEntityId(m_qPtr->entityId());
        recordNameData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
        recordNameData->setComponentName(s_recordNameEntityName);
        recordNameData->setNewValue(QString());
        recordNameData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        recordNameData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

        emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, recordNameData));

        const QList<QString> tmpRemoteProcedureList = m_remoteProcedures.keys();
        for(const QString &tmpRemoteProcedureName : qAsConst(tmpRemoteProcedureList))
        {
            VeinComponent::RemoteProcedureData *rpcData = new VeinComponent::RemoteProcedureData();
            rpcData->setEntityId(m_qPtr->entityId());
            rpcData->setCommand(VeinComponent::RemoteProcedureData::Command::RPCMD_REGISTER);
            rpcData->setProcedureName(tmpRemoteProcedureName);
            rpcData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            rpcData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, rpcData));
        }
    }

    /**
   * @brief
   * @param t_dbFilePath
   * @return
   * @note policy was to not store the database on the OS partition due to sdcard / flash wear, but removable storage filesystem formats do not allow group permission based access (FAT32/NTFS)
   */
    bool checkStorageLocation(const QString &t_dbFilePath) const
    {
        bool retVal = false;
        const auto storages = QStorageInfo::mountedVolumes();
        for(const auto &storDevice : storages)
        {
            if(storDevice.fileSystemType().contains("tmpfs") == false
                    //&& storDevice.isRoot() == false
                    && t_dbFilePath.contains(storDevice.rootPath()))
            {
                retVal = true;
                break;
            }
        }

        return retVal;
    }

    bool processCommandEvent(VeinEvent::CommandEvent *t_cEvent)
    {
        bool retVal = false;
        if(t_cEvent->eventData()->type() == VeinComponent::ComponentData::dataType())
        {
            VeinComponent::ComponentData *componentData=nullptr;
            componentData = static_cast<VeinComponent::ComponentData *>(t_cEvent->eventData());
            Q_ASSERT(componentData != nullptr);
            if(componentData->componentName() == s_recordNameEntityName)
            {
                if(componentData->eventOrigin() == VeinComponent::ComponentData::EventOrigin::EO_FOREIGN ///@todo is the check for EO_FOREIGN really required?
                        && componentData->eventCommand() == VeinComponent::ComponentData::Command::CCMD_SET
                        && t_cEvent->eventSubtype() == VeinEvent::CommandEvent::EventSubtype::TRANSACTION)
                {
                    VeinComponent::ComponentData *recordNameData = new VeinComponent::ComponentData();
                    recordNameData->setEntityId(m_qPtr->entityId());
                    recordNameData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                    recordNameData->setComponentName(s_recordNameEntityName);
                    recordNameData->setNewValue(componentData->newValue());
                    recordNameData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                    recordNameData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

                    emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, recordNameData));

                    retVal = true;
                    t_cEvent->accept();
                }
            }
        }
        else if(t_cEvent->eventData()->type() == VeinComponent::RemoteProcedureData::dataType())
        {
            VeinComponent::RemoteProcedureData *rpcData=nullptr;
            rpcData = static_cast<VeinComponent::RemoteProcedureData *>(t_cEvent->eventData());
            Q_ASSERT(rpcData != nullptr);
            if(rpcData->command() == VeinComponent::RemoteProcedureData::Command::RPCMD_CALL)
            {
                if(m_remoteProcedures.contains(rpcData->procedureName()))
                {
                    retVal = true;
                    const QUuid callId = rpcData->invokationData().value(VeinComponent::RemoteProcedureData::s_callIdString).toUuid();
                    Q_ASSERT(callId.isNull() == false);
                    Q_ASSERT(m_pendingRpcHash.contains(callId) == false);
                    m_pendingRpcHash.insert(callId, t_cEvent->peerId());
                    m_remoteProcedures.value(rpcData->procedureName())(callId, rpcData->invokationData()); //rpc invokation
                    t_cEvent->accept();
                }
                else //unknown procedure
                {
                    retVal = true;
                    qWarning() << "No remote procedure with entityId:" << m_qPtr->entityId() << "name:" << rpcData->procedureName();
                    VF_ASSERT(false, QStringC(QString("No remote procedure with entityId: %1 name: %2").arg(m_qPtr->entityId()).arg(rpcData->procedureName())));
                    VeinComponent::ErrorData *eData = new VeinComponent::ErrorData();
                    eData->setEntityId(m_qPtr->entityId());
                    eData->setErrorDescription(QString("No remote procedure with name: %1").arg(rpcData->procedureName()));
                    eData->setOriginalData(rpcData);
                    eData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                    eData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
                    VeinEvent::CommandEvent *errorEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, eData);
                    errorEvent->setPeerId(t_cEvent->peerId());
                    t_cEvent->accept();
                    emit m_qPtr->sigSendEvent(errorEvent);
                }
            }
        }

        return retVal;
    }

    //rpc
    VF_RPC(listStorages, "listStorages()", "returns ZeraDBLogger::storgeList: A lists of available storages for writing the database")
    void listStorages(const QUuid &t_callId, const QVariantMap &t_parameters)
    {
        QVariantMap retVal = t_parameters;
        const QFileInfo defaultPath("/home/operator/logger");

        const auto storages = QStorageInfo::mountedVolumes();
        QStringList storageList;

        for(const auto &storDevice : storages)
        {
            if(storDevice.isRoot() == false && storDevice.fileSystemType().contains("tmpfs") == false)
            {
                storageList.append(storDevice.rootPath());
            }
        }
        //add default path
        if(defaultPath.exists() && defaultPath.isWritable())
        {
            storageList.append(defaultPath.absoluteFilePath());
        }
        retVal.insert(s_listStoragesReturnValueName, storageList);
        retVal.insert(VeinComponent::RemoteProcedureData::s_resultCodeString, 0); //success
        m_qPtr->rpcFinished(t_callId, s_listStoragesProcedureName, retVal);
    }

    VF_RPC(findDBFile, "findDBFile(QString searchPath, QStringList searchPatternList)", "returns ZeraDBLogger::searchResultEntry A streamed list of available database files on the currently selected storage")
    void findDBFile(const QUuid &t_callId, const QVariantMap &t_rpcParameters)
    {
        QSet<QString> requiredParamKeys = { "searchPath", "searchPatternList" };
        const QVariantMap searchParameters = t_rpcParameters.value(VeinComponent::RemoteProcedureData::s_parameterString).toMap();
        requiredParamKeys.subtract(searchParameters.keys().toSet());

        if(requiredParamKeys.isEmpty())
        {
            const QString searchPath = searchParameters.value("searchPath").toString();
            const QStringList searchPatternList = searchParameters.value("searchPatternList").toStringList();
            DirIteratorWorker *worker = new DirIteratorWorker(searchPath, searchPatternList, QDir::Files, QDirIterator::Subdirectories);
            QThread *thread = new QThread(m_qPtr);
            //implementation specific filter function that tests the database files for the required schema
            DirIteratorWorker::FilterFunction validationFunction = m_validationDB->getDatabaseValidationFunction();
            worker->setFilterFunction(validationFunction);
            worker->moveToThread(thread);

            //executed in thread context
            QObject::connect(worker, &DirIteratorWorker::sigPartialResultReady, worker, [=](QString t_partialResult){
                QVariantMap tempData = t_rpcParameters;
                tempData.insert(s_findDbReturnValueName, t_partialResult);
                //would be so much easier if QDirIterator would just work with QtConcurrent::filtered
                QMetaObject::invokeMethod(m_qPtr, "rpcProgress", Qt::QueuedConnection,
                                          Q_ARG(QUuid, t_callId),
                                          Q_ARG(QString, s_findDBFileProcedureName),
                                          Q_ARG(QVariantMap, tempData));
            });
            //executed in thread context
            QObject::connect(worker, &DirIteratorWorker::sigFinished, worker, [=](){
                QVariantMap tempData = t_rpcParameters;
                tempData.insert(VeinComponent::RemoteProcedureData::s_resultCodeString, 0); //success
                //would be so much easier if QDirIterator would just work with QtConcurrent::filtered
                QMetaObject::invokeMethod(m_qPtr, "rpcFinished", Qt::QueuedConnection,
                                          Q_ARG(QUuid, t_callId),
                                          Q_ARG(QString, s_findDBFileProcedureName),
                                          Q_ARG(QVariantMap, tempData));
                worker->deleteLater();
            });
            //executed in thread context
            QObject::connect(worker, &DirIteratorWorker::sigInterrupted, worker, [=](){
                QVariantMap tempData = t_rpcParameters;
                tempData.insert(VeinComponent::RemoteProcedureData::s_resultCodeString, EINTR); //interrupted
                //would be so much easier if QDirIterator would just work with QtConcurrent::filtered
                QMetaObject::invokeMethod(m_qPtr, "rpcFinished", Qt::QueuedConnection,
                                          Q_ARG(QUuid, t_callId),
                                          Q_ARG(QString, s_findDBFileProcedureName),
                                          Q_ARG(QVariantMap, tempData));
                worker->deleteLater();
            });
            //executed in thread context
            QObject::connect(thread, &QThread::started, worker, &DirIteratorWorker::startSearch, Qt::QueuedConnection);
            //executed in thread context
            QObject::connect(thread, &QThread::finished, worker, &DirIteratorWorker::deleteLater, Qt::QueuedConnection);
            QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);

            thread->start();
        }
        else
        {
            QVariantMap retVal = t_rpcParameters; //copy parameters and other data, the client could attach tracking
            retVal.insert(VeinComponent::RemoteProcedureData::s_resultCodeString, RPCResultCodes::RPC_EINVAL);
            retVal.insert(VeinComponent::RemoteProcedureData::s_errorMessageString, QString("Missing required parameters: [%1]").arg(requiredParamKeys.toList().join(',')));
            m_qPtr->rpcFinished(t_callId, s_findDBFileProcedureName, retVal);
        }
    }

    ZeraDBLogger *m_qPtr=nullptr;
    /**
   * @brief call id, peer id
   */
    QHash<QUuid, QUuid> m_pendingRpcHash;
    //need an instance to call VeinLogger::AbstractLoggerDB::isValidDatabase
    VeinLogger::AbstractLoggerDB *m_validationDB;
    //functions need an instance so no static variable
    const VeinEvent::RoutedRemoteProcedureAtlas m_remoteProcedures;
    static constexpr QLatin1String s_listStoragesReturnValueName = modman_util::to_latin1("ZeraDBLogger::storageList");
    static constexpr QLatin1String s_findDbReturnValueName = modman_util::to_latin1("ZeraDBLogger::searchResultEntry");
    static constexpr QLatin1String s_recordNameEntityName = modman_util::to_latin1("recordName");


    friend class ZeraDBLogger;
};

//constexpr definition, see: https://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char
constexpr QLatin1String ZeraDBLoggerPrivate::s_listStoragesProcedureName; //from VF_RPC(listStorages...
constexpr QLatin1String ZeraDBLoggerPrivate::s_listStoragesProcedureDescription; //from VF_RPC(listStorages...
constexpr QLatin1String ZeraDBLoggerPrivate::s_listStoragesReturnValueName; //from VF_RPC(listStorages...
constexpr QLatin1String ZeraDBLoggerPrivate::s_findDBFileProcedureName; //from VF_RPC(findDBFile...
constexpr QLatin1String ZeraDBLoggerPrivate::s_findDBFileProcedureDescription; //from VF_RPC(findDBFile...
constexpr QLatin1String ZeraDBLoggerPrivate::s_findDbReturnValueName; //from VF_RPC(findDBFile...
constexpr QLatin1String ZeraDBLoggerPrivate::s_recordNameEntityName;

ZeraDBLogger::ZeraDBLogger(VeinLogger::DataSource *t_dataSource, VeinLogger::DBFactory t_factoryFunction, QObject *t_parent) :
    VeinLogger::DatabaseLogger(t_dataSource, t_factoryFunction, t_parent),
    m_dPtr(new ZeraDBLoggerPrivate(this))
{
    //create a database for validation
    m_dPtr->m_validationDB = t_factoryFunction(); //this class takes ownership
    //init rpc after attaching to the EventHandler
    connect(this, &ZeraDBLogger::sigAttached, [this](){ m_dPtr->initEntity(); });
}

ZeraDBLogger::~ZeraDBLogger()
{
    delete m_dPtr->m_validationDB;
    delete m_dPtr;
}

bool ZeraDBLogger::openDatabase(const QString &t_filePath)
{
    bool retVal = false;
    if(m_dPtr->checkStorageLocation(t_filePath))
    {
        retVal = VeinLogger::DatabaseLogger::openDatabase(t_filePath);
    }
    return retVal;
}

bool ZeraDBLogger::processEvent(QEvent *t_event)
{
    bool retVal = false;
    if(t_event->type()==VeinEvent::CommandEvent::eventType())
    {
        VeinEvent::CommandEvent *cEvent = nullptr;
        VeinEvent::EventData *evData = nullptr;
        cEvent = static_cast<VeinEvent::CommandEvent *>(t_event);
        Q_ASSERT(cEvent != nullptr);

        evData = cEvent->eventData();
        Q_ASSERT(evData != nullptr);

        if(evData->entityId() == entityId())
        {
            retVal = m_dPtr->processCommandEvent(cEvent);
        }
    }

    ///@note if both implementations need to handle the same event types in the future then the OR will get in the way
    retVal = retVal || VeinLogger::DatabaseLogger::processEvent(t_event);
    return retVal;
}

void ZeraDBLogger::rpcFinished(QUuid t_callId, const QString &t_procedureName, const QVariantMap &t_data)
{
    Q_ASSERT(m_dPtr->m_pendingRpcHash.contains(t_callId));
    VeinComponent::RemoteProcedureData *resultData = new VeinComponent::RemoteProcedureData();
    resultData->setEntityId(entityId());
    resultData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
    resultData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
    resultData->setCommand(VeinComponent::RemoteProcedureData::Command::RPCMD_RESULT);
    resultData->setProcedureName(t_procedureName);
    resultData->setInvokationData(t_data);

    VeinEvent::CommandEvent *rpcResultEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, resultData);
    rpcResultEvent->setPeerId(m_dPtr->m_pendingRpcHash.value(t_callId));
    emit sigSendEvent(rpcResultEvent);
    m_dPtr->m_pendingRpcHash.remove(t_callId);
}

void ZeraDBLogger::rpcProgress(QUuid t_callId, const QString &t_procedureName, const QVariantMap &t_data)
{
    Q_ASSERT(m_dPtr->m_pendingRpcHash.contains(t_callId));
    VeinComponent::RemoteProcedureData *progressData = new VeinComponent::RemoteProcedureData();
    progressData->setEntityId(entityId());
    progressData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
    progressData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
    progressData->setCommand(VeinComponent::RemoteProcedureData::Command::RPCMD_PROGRESS);
    progressData->setProcedureName(t_procedureName);
    progressData->setInvokationData(t_data);

    VeinEvent::CommandEvent *rpcResultEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, progressData);
    rpcResultEvent->setPeerId(m_dPtr->m_pendingRpcHash.value(t_callId));
    emit sigSendEvent(rpcResultEvent);
}
