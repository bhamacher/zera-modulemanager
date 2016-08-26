#ifndef JSONSESSIONLOADER_H
#define JSONSESSIONLOADER_H

#include <QObject>

class JsonSessionLoader : public QObject
{
  Q_OBJECT
public:
  explicit JsonSessionLoader(QObject *t_Parent = 0);

  const QString &getSessionFilePath() const;

signals:
  void sigLoadModule(QString t_uniqueModuleName, QString t_xmlPath, QByteArray t_xmlData, int t_moduleId);

public slots:
  void loadSession(QString t_filePath);


private:
  QString m_currentSessionFile;
};

#endif // JSONSESSIONLOADER_H
