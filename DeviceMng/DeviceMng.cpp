#include "DeviceMng.h"
#include "MsgMng.h"
#include "libdefdebuglog.h"

DeviceMng::DeviceMng()
{
    SysMinKey   = 0;
    SysMaxKey   = 0;
    AppStartKey = SysMaxKey + 1;
    CfgList.clear();
    DriverTable.clear();
    AppUseKeyList.clear();
    InitFinishFlag = false;
}

DeviceMng::~DeviceMng()
{
    mDriverTable::iterator item;

    sysLogQD() << "DeviceMng**************~DeviceMng";
    for (item = DriverTable.begin(); item != DriverTable.end(); ++item)
    {
        // item.value()->pmsg->delete_object();
        delete item.value();
        item.value() = (driver*) 0;
    }
    CfgList.clear();
    DriverTable.clear();
    AppUseKeyList.clear();
}

DeviceMng* DeviceMng::pDeviceCmd = NULL;
DeviceMng* DeviceMng::GetDeviceMng()
{
    if (pDeviceCmd == NULL)
    {
        pDeviceCmd = new DeviceMng();
    }
    return pDeviceCmd;
}

void DeviceMng::loadCfgFile(const QString filePath)
{
    QDomDocument doc;
    QString      filepathname = filePath;
    QFile        file(filepathname);

    QString errorStr;
    int     errorLine;
    int     errorColumn;

    if (!doc.setContent(&file, true, &errorStr, &errorLine, &errorColumn))
    {
        sysLogQE() << "DeviceMng Line " << errorLine << ", column: " << errorColumn << " " << errorStr.toLatin1().data();
        return;
    }

    QDomElement doc_root = doc.documentElement();
    if (doc_root.tagName() != "DriverList")
    {
        sysLogE("DeviceMng The driver list config file is not valid");
        return;
    }

    CfgList.clear();
    QDomElement element;
    QDomNode    doc_node = doc_root.firstChild();
    while (!doc_node.isNull())
    {
        if (doc_node.toElement().tagName() == "Driver")
        {
            element = doc_node.toElement();
            sDeviceCfg tmpnode;
            tmpnode.id     = element.attribute("id").toInt();
            tmpnode.name   = element.attribute("drivername");
            tmpnode.script = element.attribute("startscript");
            CfgList.append(tmpnode);
        }
        doc_node = doc_node.nextSibling();
    }
}

bool DeviceMng::FindDriver(uint8_t id, driver** ppdriver)
{
    mDriverTable::iterator item;

    item = DriverTable.find(id);
    if ((item != DriverTable.end()) && (item.key() == id))
    {
        *ppdriver = item.value();
        return true;
    }
    *ppdriver = (driver*) 0;
    return false;
}

void DeviceMng::loadShareParam(const QString filePath)
{
    QSettings settings(filePath, QSettings::IniFormat);
    settings.setIniCodec("UTF-8");

    QString value;

    settings.beginGroup("MEMERY_PARAM");
    value     = QString("SYS_KEY_RESERVE");
    SysResKey = settings.value(value).toInt();
    value     = QString("SYS_KEY_MIN");
    SysMinKey = settings.value(value).toInt();
    value     = QString("SYS_KEY_MAX");
    SysMaxKey = settings.value(value).toInt();
    settings.endGroup();
    settings.deleteLater();
}

int DeviceMng::OperateAppMsgKey(eOperateKeyType mode, int key)
{
    int remain = SysMaxKey - AppStartKey + 1 - AppUseKeyList.size();
    if (mode == KEY_ADD)
    {
        if (remain <= 0)
            return 0;
        for (int i = AppStartKey; i < SysMaxKey; i++)
        {
            if (!AppUseKeyList.contains(i))
            {
                AppUseKeyList.append(i);
                return i;
            }
        }
    }
    else if (mode == KEY_SUB)
    {
        if (AppUseKeyList.size() == 0)
            return 0;
        for (int i = 0; i < AppUseKeyList.size(); i++)
        {
            if (AppUseKeyList.at(i) == key)
            {
                AppUseKeyList.removeAt(i);
                return 1;
            }
        }
    }
    return 0;
}

bool DeviceMng::CheckParamValidity(void)
{
    for (int i = 1; i < CfgList.size(); i++)
    {
        for (int j = 0; j < i; j++)
        {
            if (CfgList.at(j).id == CfgList.at(i).id)
            {
                sysLogQE() << "DeviceMng cfglist id exist!";
                return false;
            }
        }
    }

    if (SysMaxKey < SysMinKey)
    {
        sysLogQE() << "DeviceMng SysMaxKey < SysMinKey!";
        return false;
    }

    AppUseKeyList.clear();
    if ((SysMaxKey - SysMinKey + 1) <= (DRIVER_SHARE_KEY_NUM * CfgList.size() + DEVICEMNG_SHARE_KEY_NUM))
    {
        sysLogQE() << "DeviceMng SysKey num fail,SysMaxKey:" << SysMaxKey << "SysMinKey:" << SysMinKey
                 << "CfgList.size() :" << CfgList.size();
        return false;
    }

    AppStartKey = SysMinKey + DRIVER_SHARE_KEY_NUM * CfgList.size() + DEVICEMNG_SHARE_KEY_NUM;
    return true;
}

int DeviceMng::GetDeviceMngKey(void)
{
    return SysMinKey;
}

int DeviceMng::GetDeviceMngResKey(void)
{
    return SysResKey;
}

void DeviceMng::ShellSetupDriver(sDeviceCfg& cfg, int key, int drivermsgkey)
{
    QString script;
    QString chmod = "chmod +x ";

    chmod = chmod + cfg.script;
    system(chmod.toLatin1().data());

    script = cfg.script + QString(" %1 %2 %3 %4 %5 %6 %7")
                              .arg(key)
                              .arg(key + 1)
                              .arg(key + 2)
                              .arg(key + 3)
                              .arg(drivermsgkey)
                              .arg(cfg.id)
                              .arg(key + 4);
    sysLogQD() << "DeviceMng ShellSetupDriver :" << script;
    system(script.toLatin1().data());
}

bool DeviceMng::SetupDriver(void)
{
    driver*    pdriver;
    int        keytemp;
    sDeviceCfg cfg;
    int        usekey = SysMinKey + DEVICEMNG_SHARE_KEY_NUM;

    for (int i = 0; i < CfgList.size(); i++)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        keytemp = SysMinKey + DEVICEMNG_SHARE_KEY_NUM + DRIVER_SHARE_KEY_NUM * (CfgList.at(i).id - 1);
        sysLogQD() << "keytemp = " << keytemp
                   << SysMinKey + DEVICEMNG_SHARE_KEY_NUM + DRIVER_SHARE_KEY_NUM * (CfgList.at(i).id - 1);
        cfg.id = CfgList.at(i).id;
        //sysLogQD() << "DeviceMng device[" << i << "]=" << cfg.id << " init time:" << tv.tv_sec;
        cfg.name = CfgList.at(i).name;
        pdriver  = new driver(cfg.id, cfg.name, keytemp, keytemp + 1, keytemp + 2, keytemp + 3, keytemp + 4);
        DriverTable.insert(cfg.id, pdriver);
        if (!pdriver->InitMsg())
            return false;

        cfg.script = CfgList.at(i).script;
        ShellSetupDriver(
            cfg, SysMinKey + DEVICEMNG_SHARE_KEY_NUM + DRIVER_SHARE_KEY_NUM * (CfgList.at(i).id - 1), SysMinKey + 1);
        usekey += DRIVER_SHARE_KEY_NUM;

        if (!pdriver->Init())
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            sysLogQE() << "DeviceMng device[" << i << "]=" << cfg.id << " Initmem error time:" << tv.tv_sec;
            return false;
        }
    }
    AppStartKey = usekey;

    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        sysLogQD() << "DeviceMng device SetupDriver end time:" << tv.tv_sec;
    }
    return true;
}

void DeviceMng::SendHeartToDriver(void)
{
    mDriverTable::iterator item;
    MsgMng*                pMsgMng = MsgMng::GetMsgMng();

    pthread_mutex_lock(&pMsgMng->RevTaskMutex);
    for (item = DriverTable.begin(); item != DriverTable.end(); ++item)
    {
        item.value()->Msg_SendHeart();
    }
    pthread_mutex_unlock(&pMsgMng->RevTaskMutex);
}

void DeviceMng::DriverHeartMng(void)
{
    int                    ret;
    mDriverTable::iterator item;
    uint8_t                data;
    MsgMng*                pMsgMng = MsgMng::GetMsgMng();

    for (item = DriverTable.begin(); item != DriverTable.end(); ++item)
    {
        ret = sem_trywait(&(item.value()->AckSem));
        if (ret >= 0)
        {
            if (item.value()->ComState != COMSTATE_NORMAL)
            {
                item.value()->ComState = COMSTATE_NORMAL;
                data                   = (uint8_t) item.key();
                pMsgMng->BroadcastToApp(MSG_TYPE_AppReportDriverComNormal, &data, 1);
                //qDebug() << "DeviceMng report driver com normal";
            }
        }
        else
        {
            if (item.value()->ComState != COMSTATE_ABNORMAL)
            {
                item.value()->ComState = COMSTATE_ABNORMAL;
                data                   = (uint8_t) item.key();
                pMsgMng->BroadcastToApp(MSG_TYPE_AppReportDriverComAbnormal, &data, 1);
                sysLogQE() << "DeviceMng report driver com abnormal**";
            }
        }
    }
    // debug↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
    //    driver *pdriver;
    //    double value = 0xff;
    //    this->FindDriver(0,&pdriver);
    //    for (int i = 0; i < 16; i++)
    //    {
    //        pdriver->pshm->shm_read(5,0,i+1,&value);
    //        qDebug()<<"value = "<<value<< "
    //        ---------------------------------------------------------------------------------------------------------";
    //    }
    // debug↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑
}
