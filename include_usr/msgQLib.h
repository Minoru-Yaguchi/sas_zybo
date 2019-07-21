#include <mqueue.h>

#define PATH_MAX 256

typedef struct _msg_q_t
{
    mqd_t mq_id;
    char msgQName[PATH_MAX];
} MsgQInfo;

typedef struct _msg_q_t *MSG_Q_ID;

MSG_Q_ID msgQCreate(int maxMsgs, int maxMsgLength);
int msgQSend(MSG_Q_ID msgQId, char *buffer, int nBytes);
int msgQReceive(MSG_Q_ID msgQId, char *buffer, int maxNBytes);
