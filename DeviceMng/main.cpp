#include "DeviceMng.h"
#include "MsgMng.h"
#include <signal.h>
#include "libdefdebuglog.h"
static bool sysLogBefore(QString msg, char const* filename, int line)
{
    Q_UNUSED(msg);
    Q_UNUSED(filename);
    Q_UNUSED(line);
    QFile file("/opt/bin/DeviceMng.Debug");
    //return file.exists();

    return true;
}
void SignalFunc(int var)
{
    sysLogQE() "<DeviceMng signal exit:" << var << " >";
    DeviceMng* pDeviceMng = DeviceMng::GetDeviceMng();
    MsgMng*    pMsgMng    = MsgMng::GetMsgMng();

    DELETE(pMsgMng);
    DELETE(pDeviceMng);
    sysLogQE() << "DeviceMng signal exit finish! main return";
    exit(0);
}

int main()
{
    Liblog::getInstance(Log_log4cplus);
    Liblog::getInstance()->setMinLogLevel(LEVEL_ALL_LOG);
    Liblog::getInstance()->setLogBefore(sysLogBefore);
    setbuf(stdout, NULL);
    DeviceMng* pDeviceMng = DeviceMng::GetDeviceMng();
    MsgMng*    pMsgMng    = MsgMng::GetMsgMng();

    sysLogI("-----------DeviceMng program start-------------!");
    pDeviceMng->loadCfgFile(CFGXML_FILE_PATH);
    pDeviceMng->loadShareParam(CFGINI_FILE_PATH);
    sysLogI("DeviceMng load param finish!");
    if (!pDeviceMng->CheckParamValidity())
    {
        sysLogQE() << "DeviceMng param check fail! main return";
        return 0;
    }

    if (!pMsgMng->Init(pDeviceMng->GetDeviceMngKey(), pDeviceMng->GetDeviceMngKey() + 1,
            pDeviceMng->GetDeviceMngKey() + 2, pDeviceMng->GetDeviceMngKey() + 3, pDeviceMng->GetDeviceMngResKey()))
    {
        sysLogQE() << "DeviceMng self msg init fail! main return";
        return 0;
    }

    if (!pDeviceMng->SetupDriver())
    {
        sysLogQE() << "DeviceMng driver init fail! main return";
        return 0;
    }
    signal(SIGINT, SignalFunc);
    signal(SIGTERM, SignalFunc);

    pDeviceMng->InitFinishFlag = true;
    // int testcycle              = 0;

    while (1)
    {
        sleep(1);
        // sysLogQD() << "DeviceMng cycle:" << testcycle++;
        pDeviceMng->SendHeartToDriver();
        sleep(2);
        pDeviceMng->DriverHeartMng();
    }

    return 1;
}
