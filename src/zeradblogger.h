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
  ~ZeraDBLogger();
signals:

  // DatabaseLogger interface
public slots:
  bool openDatabase(const QString &t_filePath) override;
  bool processEvent(QEvent *t_event) override;

private:
  ZeraDBLoggerPrivate *m_dPtr=nullptr;
};

#endif // ZERADBLOGGER_H
