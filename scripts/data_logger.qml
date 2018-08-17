import QtQuick 2.0
import VeinEntity 1.0
import VeinLogger 1.0

VeinLogger {
  initializeValues: true;
  recordName: loggerEntity.recordName;

  readonly property QtObject systemEntity: VeinEntity.getEntity("_System");
  readonly property QtObject loggerEntity: VeinEntity.getEntity("_LoggingSystem");
  readonly property string session: systemEntity.Session
  onSessionChanged: {
    if(session.indexOf("mt310s2")===-1)
    {
      initScript();
    }
    else
    {
      initScriptMTS2()
    }
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

  function initScript() {
    clearLoggerEntries();
    var logValues = {};
    //Range module 1020
    var rangeModuleEntries = [];
    rangeModuleEntries.push("ACT_Frequency");
    rangeModuleEntries.push("PAR_Channel1Range");
    rangeModuleEntries.push("PAR_Channel2Range");
    rangeModuleEntries.push("PAR_Channel3Range");
    rangeModuleEntries.push("PAR_Channel4Range");
    rangeModuleEntries.push("PAR_Channel5Range");
    rangeModuleEntries.push("PAR_Channel6Range");
    logValues["1020"] = rangeModuleEntries;

    //DFT module 1050
    var dftModuleEntries = [];
    dftModuleEntries.push("ACT_DFTPN1");
    dftModuleEntries.push("ACT_DFTPN2");
    dftModuleEntries.push("ACT_DFTPN3");
    dftModuleEntries.push("ACT_DFTPN4");
    dftModuleEntries.push("ACT_DFTPN5");
    dftModuleEntries.push("ACT_DFTPN6");
    dftModuleEntries.push("ACT_DFTPP1");
    dftModuleEntries.push("ACT_DFTPP2");
    dftModuleEntries.push("ACT_DFTPP3");
    logValues["1050"] = dftModuleEntries;

    if(session !== "com5003-ref-session.json")
    {
      //RMS module 1040
      var rmsModuleEntries = [];
      rmsModuleEntries.push("ACT_RMSPN1");
      rmsModuleEntries.push("ACT_RMSPN2");
      rmsModuleEntries.push("ACT_RMSPN3");
      rmsModuleEntries.push("ACT_RMSPN4");
      rmsModuleEntries.push("ACT_RMSPN5");
      rmsModuleEntries.push("ACT_RMSPN6");
      rmsModuleEntries.push("ACT_RMSPP1");
      rmsModuleEntries.push("ACT_RMSPP2");
      rmsModuleEntries.push("ACT_RMSPP3");
      logValues["1040"] = rmsModuleEntries;
      //Power1Module1 1070
      var p1m1Entries = [];
      p1m1Entries.push("ACT_PQS1");
      p1m1Entries.push("ACT_PQS2");
      p1m1Entries.push("ACT_PQS3");
      p1m1Entries.push("ACT_PQS4");
      p1m1Entries.push("PAR_MeasuringMode");
      logValues["1070"] = p1m1Entries;
      //Power1Module2 1071
      var p1m2Entries = [];
      p1m2Entries.push("ACT_PQS1");
      p1m2Entries.push("ACT_PQS2");
      p1m2Entries.push("ACT_PQS3");
      p1m2Entries.push("ACT_PQS4");
      p1m2Entries.push("PAR_MeasuringMode");
      logValues["1071"] = p1m2Entries;
      //Power1Module3 1072
      var p1m3Entries = [];
      p1m3Entries.push("ACT_PQS1");
      p1m3Entries.push("ACT_PQS2");
      p1m3Entries.push("ACT_PQS3");
      p1m3Entries.push("ACT_PQS4");
      p1m3Entries.push("PAR_MeasuringMode");
      logValues["1072"] = p1m3Entries;
    }
    systemEntity.LoggedComponents = logValues;
  }

  function initScriptMTS2() {
    clearLoggerEntries();
    systemEntity.LoggedComponents = {
      "200":[
        "PAR_CustomerCity",
        "PAR_CustomerComment",
        "PAR_CustomerCountry",
        "PAR_CustomerFirstName",
        "PAR_CustomerLastName",
        "PAR_CustomerNumber",
        "PAR_CustomerPostalCode",
        "PAR_CustomerStreet",
        "PAR_DatasetComment",
        "PAR_DatasetIdentifier",
        "PAR_LocationCity",
        "PAR_LocationComment",
        "PAR_LocationCountry",
        "PAR_LocationFirstName",
        "PAR_LocationLastName",
        "PAR_LocationNumber",
        "PAR_LocationPostalCode",
        "PAR_LocationStreet",
        "PAR_MeterComment",
        "PAR_MeterFactoryNumber",
        "PAR_MeterManufacturer",
        "PAR_MeterOwner",
        "PAR_PowerGridComment",
        "PAR_PowerGridOperator",
        "PAR_PowerGridSupplier"
      ],
      "1020":[
        "PAR_Channel1Range",
        "PAR_Channel2Range",
        "PAR_Channel3Range",
        "PAR_Channel4Range",
        "PAR_Channel5Range",
        "PAR_Channel6Range",
        "PAR_Channel7Range",
        "PAR_Channel8Range",
        "ACT_Frequency"
      ],
      "1040":[
        "ACT_RMSPN1",
        "ACT_RMSPN2",
        "ACT_RMSPN3",
        "ACT_RMSPN4",
        "ACT_RMSPN5",
        "ACT_RMSPN6",
        "ACT_RMSPN7",
        "ACT_RMSPN8",
        "ACT_RMSPP1",
        "ACT_RMSPP2",
        "ACT_RMSPP3"
      ],
      "1050":[
        "ACT_DFTPN1",
        "ACT_DFTPN2",
        "ACT_DFTPN3",
        "ACT_DFTPN4",
        "ACT_DFTPN5",
        "ACT_DFTPN6",
        "ACT_DFTPN7",
        "ACT_DFTPN8",
        "ACT_DFTPP1",
        "ACT_DFTPP2",
        "ACT_DFTPP3"
      ],
      "1060":[
        "ACT_FFT1",
        "ACT_FFT2",
        "ACT_FFT3",
        "ACT_FFT4",
        "ACT_FFT5",
        "ACT_FFT6",
        "ACT_FFT7",
        "ACT_FFT8"
      ],
      "1070":[
        "ACT_PQS1",
        "ACT_PQS2",
        "ACT_PQS3",
        "ACT_PQS4",
        "PAR_MeasuringMode"
      ],
      "1071":[
        "ACT_PQS1",
        "ACT_PQS2",
        "ACT_PQS3",
        "ACT_PQS4",
        "PAR_MeasuringMode"
      ],
      "1072":[
        "ACT_PQS1",
        "ACT_PQS2",
        "ACT_PQS3",
        "ACT_PQS4",
        "PAR_MeasuringMode"
      ],
      "1100":[
        "ACT_HPW1",
        "ACT_HPW2",
        "ACT_HPW3",
        "ACT_HPW4"
      ],
      "1110":[
        "ACT_THDN1",
        "ACT_THDN2",
        "ACT_THDN3",
        "ACT_THDN4",
        "ACT_THDN5",
        "ACT_THDN6",
        "ACT_THDN7",
        "ACT_THDN8"
      ],
      "1120":[
        "ACT_OSCI1",
        "ACT_OSCI2",
        "ACT_OSCI3",
        "ACT_OSCI4",
        "ACT_OSCI5",
        "ACT_OSCI6",
        "ACT_OSCI7",
        "ACT_OSCI8"
      ],
      "1130":[
        "ACT_Result",
        "ACT_Status",
        "PAR_DutConstant",
        "PAR_DUTConstUnit",
        "PAR_DutInput",
        "PAR_Energy",
        "PAR_Mode",
        "PAR_MRate",
        "PAR_RefConstant",
        "PAR_RefInput",
        "PAR_Target"
      ],
      "1140":[
        "ACT_Lambda1",
        "ACT_Lambda2",
        "ACT_Lambda3"
      ],
      "1150":[
        "INF_ReleaseNr",
        "INF_SerialNr"
      ],
      "1160":[
        "ACT_Burden1",
        "ACT_Burden2",
        "ACT_Burden3",
        "ACT_PFactor1",
        "ACT_PFactor2",
        "ACT_PFactor3",
        "ACT_Ratio1",
        "ACT_Ratio2",
        "ACT_Ratio3",
        "PAR_NominalBurden",
        "PAR_NominalRange",
        "PAR_NominalRangeFactor",
        "PAR_WCrosssection",
        "PAR_WireLength"
      ],
      "1161":[
        "ACT_Burden1",
        "ACT_Burden2",
        "ACT_Burden3",
        "ACT_PFactor1",
        "ACT_PFactor2",
        "ACT_PFactor3",
        "ACT_Ratio1",
        "ACT_Ratio2",
        "ACT_Ratio3",
        "PAR_NominalBurden",
        "PAR_NominalRange",
        "PAR_NominalRangeFactor",
        "PAR_WCrosssection",
        "PAR_WireLength"
      ],
      "1170":[
        "ACT_Angle1",
        "ACT_Error1",
        "ACT_Ratio1",
        "PAR_DutPrimary",
        "PAR_DutSecondary",
        "PAR_PrimClampPrim",
        "PAR_PrimClampSec",
        "PAR_SecClampPrim",
        "PAR_SecClampSec"
      ]
    };
  }
}
