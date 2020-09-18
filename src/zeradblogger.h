#ifndef ZERADBLOGGER_H
#define ZERADBLOGGER_H

#include <vl_databaselogger.h>
#include <QHash>
#include <QUuid>

#include "modman_util.h"

namespace VeinLogger
{
class DataSource;
}

class ZeraDBLoggerPrivate;

/**
 * @brief Class to avoid adding policy to the VeinLogger::DatabaseLogger
 */
class ZeraDBLogger : public VeinLogger::DatabaseLogger
{
    Q_OBJECT
public:
    ZeraDBLogger(VeinLogger::DataSource *t_dataSource, VeinLogger::DBFactory t_factoryFunction, QObject *t_parent=nullptr);
    ~ZeraDBLogger() override;
signals:

    // DatabaseLogger interface
public slots:
    /**
   * @brief this only calls ZeraDBLoggerPrivate::checkStorageLocation() and then VeinLogger::DatabaseLogger::openDatabase()
   * @param t_filePath
   * @return
   */
    bool openDatabase(const QString &t_filePath) override;
    bool processEvent(QEvent *t_event) override;

private slots:
    void rpcFinished(QUuid t_callId, const QString &t_procedureName, const QVariantMap &t_data);
    void rpcProgress(QUuid t_callId, const QString &t_procedureName, const QVariantMap &t_data);

private:
    ZeraDBLoggerPrivate *m_dPtr=nullptr;
    friend class ZeraDBLoggerPrivate;
};

#endif // ZERADBLOGGER_H
