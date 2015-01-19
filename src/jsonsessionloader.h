#ifndef JSONSESSIONLOADER_H
#define JSONSESSIONLOADER_H

#include <QObject>

class JsonSessionLoader : public QObject
{
  Q_OBJECT
public:
  explicit JsonSessionLoader(QObject *qObjParent = 0);

  const QString &getSessionFilePath() const;

signals:
  void sigLoadModule(QString uniqueModuleName, QByteArray xmlData);

public slots:
  void loadSession(QString filePath);


private:
  QString m_currentSessionFile;
};

#endif // JSONSESSIONLOADER_H
