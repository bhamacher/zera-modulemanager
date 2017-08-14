import QtQuick 2.0
import VeinEntity 1.0
import VeinLogger 1.0

VeinLogger {
  recordName: "NewRecord";

  readonly property QtObject systemEntity: VeinEntity.getEntity("_System");
  readonly property string session: systemEntity.Session
  readonly property bool scriptRunning: loggingEnabled && session === "0_default-session.json"

  function initScript() {
    addLoggerEntry(1004, "ACT_DFTPN1");
    addLoggerEntry(1004, "ACT_DFTPN2");
    addLoggerEntry(1004, "ACT_DFTPN3");
    addLoggerEntry(1004, "ACT_DFTPN4");
    addLoggerEntry(1004, "ACT_DFTPN5");
    addLoggerEntry(1004, "ACT_DFTPN6");
  }

  Component.onCompleted: initScript();

  onScriptRunningChanged: {
    if(scriptRunning === true)
    {
      console.log("starting logging at", new Date().toLocaleTimeString());
      startLogging();
    }
    else
    {
      console.log("stopped logging at", new Date().toLocaleTimeString());
      stopLogging();
    }
  }
}
