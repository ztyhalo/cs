#include "DeviceMng.h"
#include "MsgMng.h"
#include <signal.h>
#include "libdefdebuglog.h"
#include "libcommon.h"

// #ifdef ARM
// #include "exception_handler.h"
// google_breakpad::ExceptionHandler* eh = nullptr;

// static bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
// {
//     Q_UNUSED(context);
//     qDebug("start breakcommend: %s\n");
//     QString str = "/opt/bin/breakpad/dump.sh DeviceMng " + QString(descriptor.path());
//     system(str.toStdString().c_str());
//     qDebug("stop breakcommend: %s\n", str.toStdString().c_str());
//     return succeeded;
// }
// #endif

static bool sysLogBefore(QString msg, char const* filename, int line)
{
    Q_UNUSED(msg);
    Q_UNUSED(filename);
    Q_UNUSED(line);
    QFile file("/opt/bin/DeviceMng.Debug");
    // return file.exists();

    return true;
}
void SignalFunc(int var)
{
    sysLogQE() "<DeviceMng signal exit:" << var << " >";
    DeviceMng* pDeviceMng = DeviceMng::GetDeviceMng();
    // MsgMng*    pMsgMng    = MsgMng::GetMsgMng();

    // DELETE(pMsgMng);
    sysLogQE() << "-----------------DELETE(pMsgMng);";
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
    // string path = "/opt/commonlib";
    // string pr_file = path + "/deviceMng.log";
    // PRINTF_CLASS::getInstance()->printf_class_init(path, pr_file);

    // #ifdef ARM
    //     system("mkdir -p /opt/236Logs/crashlog");
    //     google_breakpad::MinidumpDescriptor descriptor("/opt/236Logs/crashlog");
    //     eh = new google_breakpad::ExceptionHandler(descriptor, NULL, dumpCallback, NULL, true, -1);
    // #endif
    signal(SIGINT, SignalFunc);
    signal(SIGTERM, SignalFunc);
    DeviceMng* pDeviceMng = DeviceMng::GetDeviceMng();
    // MsgMng*    pMsgMng    = MsgMng::GetMsgMng();

    sysLogI("-----------DeviceMng program start-------------!");
    pDeviceMng->loadCfgFile(CFGXML_FILE_PATH);
    pDeviceMng->loadShareParam(CFGINI_FILE_PATH);
    sysLogI("DeviceMng load param finish!");
    if (!pDeviceMng->CheckParamValidity())
    {
        sysLogQE() << "DeviceMng param check fail! main return";
        return 0;
    }

    if (!pDeviceMng->m_pMngServ->init(pDeviceMng->GetDeviceMngKey(), pDeviceMng->GetDeviceMngKey() + 1,
            pDeviceMng->GetDeviceMngKey() + 2, pDeviceMng->GetDeviceMngKey() + 3, pDeviceMng->GetDeviceMngResKey()))
    {
        sysLogQE() << "DeviceMng self msg init fail! main return";
        DELETE(pDeviceMng);
        return 0;
    }

    if (!pDeviceMng->SetupDriver())
    {
        sysLogQE() << "DeviceMng driver init fail! main return";
        DELETE(pDeviceMng);
        return 0;
    }


    pDeviceMng->m_initOk = true;
    // int testcycle              = 0;
    while (1)
    {
        SLEEP(1);
        // sysLogQD() << "DeviceMng cycle:" << testcycle++;
        pDeviceMng->SendHeartToDriver();
        SLEEP(2);
        pDeviceMng->DriverHeartMng();
    }

    return 1;
}
