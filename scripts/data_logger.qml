import QtQuick 2.0
import VeinEntity 1.0
import VeinLogger 1.0

VeinLogger {
  initializeValues: true;
  recordName: loggerEntity.recordName;
  transactionName: loggerEntity.transactionName;


  readonly property QtObject systemEntity: VeinEntity.getEntity("_System");
  readonly property QtObject loggerEntity: VeinEntity.getEntity("_LoggingSystem");
  readonly property string sysSession: systemEntity.Session
  onSysSessionChanged: {
    session = systemEntity.Session;
    loggerEntity.availableContentSets = readSession();
  }

  readonly property string sysContext: loggerEntity.currentContext;
  onSysContextChanged: {
      context=loggerEntity.currentContext;
      var comps = readContext();
      clearLoggerEntries();
      systemEntity.LoggedComponents = comps;
  }

  readonly property bool scriptRunning: loggingEnabled
  onScriptRunningChanged: {
    if(scriptRunning === true)
    {
      console.log("starting logging at", new Date().toLocaleTimeString());
      startLogging();
    }
    else
    {
      stopLogging();
      console.log("stopped logging at", new Date().toLocaleTimeString());
    }
  }

  readonly property var loggedValues: systemEntity.LoggedComponents
  onLoggedValuesChanged: {
    clearLoggerEntries();
    for(var it in loggedValues)
    {
      var entData = loggedValues[it];
      for(var i=0; i<entData.length; ++i)
      {
        addLoggerEntry(it, entData[i]);
      }
    }
  }  
}
