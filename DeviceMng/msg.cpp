#include "msg.h"
#include "libdefdebuglog.h"
msg::~msg()
{
}

bool msg::SendMsg(sMsgUnit* pdata, uint16_t size)
{
    bool    ret;
    uint8_t temp[sizeof(sMsgUnit)];

    memset(temp, 0, sizeof(temp));
    temp[0] = pdata->source.driver.id_driver;
    temp[1] = pdata->source.driver.id_parent;
    temp[2] = pdata->source.driver.id_child;
    temp[3] = pdata->source.driver.id_point;
    temp[4] = pdata->dest.driver.id_driver;
    temp[5] = pdata->dest.driver.id_parent;
    temp[6] = pdata->dest.driver.id_child;
    temp[7] = pdata->dest.driver.id_point;
    temp[8] = (uint8_t) ((pdata->type & 0xff00) >> 8);
    temp[9] = (uint8_t) (pdata->type & 0x00ff);
    memcpy(&temp[10], pdata->data, size);

    ret = send_object(temp, size + MSG_UNIT_HEAD_LEN);
    return ret;
}

bool msg::ReceiveMsg(sMsgUnit* pdata, uint16_t* psize, int mode)
{
    bool    ret;
    uint8_t temp[sizeof(sMsgUnit)];
    int     len = 0;

    memset(temp, 0, sizeof(temp));
    ret = receive_object(temp, &len, mode);
    if (ret)
    {
        *psize = len;
        if (*psize >= MSG_UNIT_HEAD_LEN)
        {
            *psize                         = *psize - MSG_UNIT_HEAD_LEN;
            pdata->source.driver.id_driver = temp[0];
            pdata->source.driver.id_parent = temp[1];
            pdata->source.driver.id_child  = temp[2];
            pdata->source.driver.id_point  = temp[3];
            pdata->dest.driver.id_driver   = temp[4];
            pdata->dest.driver.id_parent   = temp[5];
            pdata->dest.driver.id_child    = temp[6];
            pdata->dest.driver.id_point    = temp[7];
            pdata->type                    = ((uint16_t) (temp[8] << 8)) | temp[9];
            memcpy(pdata->data, &temp[10], *psize);
            return true;
        }
    }
    return false;
}
