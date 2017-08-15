import QtQuick 2.0
import VeinEntity 1.0
import VeinLogger 1.0

VeinLogger {
  recordName: "default";
  initializeValues: true;

  readonly property QtObject systemEntity: VeinEntity.getEntity("_System");
  readonly property string session: systemEntity.Session
  readonly property bool scriptRunning: loggingEnabled && session === "0_default-session.json"

  function initScript() {
    //Range module
    addLoggerEntry(1001, "ACT_Frequency");
    addLoggerEntry(1001, "PAR_Channel1Range");
    addLoggerEntry(1001, "PAR_Channel2Range");
    addLoggerEntry(1001, "PAR_Channel3Range");
    addLoggerEntry(1001, "PAR_Channel4Range");
    addLoggerEntry(1001, "PAR_Channel5Range");
    addLoggerEntry(1001, "PAR_Channel6Range");
    //RMS module
    addLoggerEntry(1003, "ACT_RMSPN1");
    addLoggerEntry(1003, "ACT_RMSPN2");
    addLoggerEntry(1003, "ACT_RMSPN3");
    addLoggerEntry(1003, "ACT_RMSPN4");
    addLoggerEntry(1003, "ACT_RMSPN5");
    addLoggerEntry(1003, "ACT_RMSPN6");
    addLoggerEntry(1003, "ACT_RMSPP1");
    addLoggerEntry(1003, "ACT_RMSPP2");
    addLoggerEntry(1003, "ACT_RMSPP3");
    //DFT module
    addLoggerEntry(1004, "ACT_DFTPN1");
    addLoggerEntry(1004, "ACT_DFTPN2");
    addLoggerEntry(1004, "ACT_DFTPN3");
    addLoggerEntry(1004, "ACT_DFTPN4");
    addLoggerEntry(1004, "ACT_DFTPN5");
    addLoggerEntry(1004, "ACT_DFTPN6");
    addLoggerEntry(1004, "ACT_DFTPP1");
    addLoggerEntry(1004, "ACT_DFTPP2");
    addLoggerEntry(1004, "ACT_DFTPP3");
    //Power1Module1
    addLoggerEntry(1006, "ACT_PQS1");
    addLoggerEntry(1006, "ACT_PQS2");
    addLoggerEntry(1006, "ACT_PQS3");
    addLoggerEntry(1006, "ACT_PQS4");
    addLoggerEntry(1006, "PAR_MeasuringMode");
    //Power1Module2
    addLoggerEntry(1007, "ACT_PQS1");
    addLoggerEntry(1007, "ACT_PQS2");
    addLoggerEntry(1007, "ACT_PQS3");
    addLoggerEntry(1007, "ACT_PQS4");
    addLoggerEntry(1007, "PAR_MeasuringMode");
    //Power1Module3
    addLoggerEntry(1008, "ACT_PQS1");
    addLoggerEntry(1008, "ACT_PQS2");
    addLoggerEntry(1008, "ACT_PQS3");
    addLoggerEntry(1008, "ACT_PQS4");
    addLoggerEntry(1008, "PAR_MeasuringMode");
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
