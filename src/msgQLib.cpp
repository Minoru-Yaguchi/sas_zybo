#include "msgQLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

MSG_Q_ID msgQCreate(int maxMsgs, int maxMsgLength)
{
	MSG_Q_ID lp_MsgID;
	struct mq_attr s_Attr;
	unsigned int l_MyThreadID;
	static unsigned int ul_MqSerialNo = 0;

	lp_MsgID = (MSG_Q_ID)malloc(sizeof(MsgQInfo));
	if ( lp_MsgID == NULL ) {
		perror("msgQCreate:malloc");
		return NULL;
	}

	/* メッセージキューの名前を作る */
	l_MyThreadID = (unsigned int)pthread_self();
	snprintf(lp_MsgID->msgQName, PATH_MAX, "/%u_%u", l_MyThreadID, ++ul_MqSerialNo);

	/* メッセージキューの属性 */
	s_Attr.mq_maxmsg  = maxMsgs;
	s_Attr.mq_msgsize = maxMsgLength;
	s_Attr.mq_flags   = 0;

	lp_MsgID->mq_id = mq_open(	lp_MsgID->msgQName,
								O_CREAT|O_EXCL|O_RDWR,
								S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH,
								&s_Attr);


	if ( lp_MsgID->mq_id == (mqd_t)-1 ) {
		/* アプリ再立ち上げ時のopenエラー防止 */
		mq_unlink(lp_MsgID->msgQName);
		lp_MsgID->mq_id = mq_open(lp_MsgID->msgQName,
								O_CREAT|O_EXCL|O_RDWR,
								S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH,
								&s_Attr);
		if ( lp_MsgID->mq_id == (mqd_t)-1 ) {
			perror("msgQCreate:mq_open");
			free(lp_MsgID);

			return NULL;
		}
	}
	return lp_MsgID;
}

int msgQDelete(MSG_Q_ID msgQId)
{
	if ( msgQId == NULL ) {
		printf("msgQId is NULL\n");
		return -1;
	}

	mq_close(msgQId->mq_id);
	mq_unlink(msgQId->msgQName);
	free(msgQId);
	return 0;
}

int msgQSend(MSG_Q_ID msgQId, char *buffer, int nBytes)
{
	struct mq_attr attr;
	int i_Ret;
	const int priority=0;

	if ( msgQId == NULL ) {
		printf("invalid msgQId(NULL)\n");
		return -1;
	}
	
	if ( mq_getattr(msgQId->mq_id, &attr) ) {
		printf("mq_getattr err\n");
		return -1;
	}
	
	/* メッセージサイズ溢れ防止 */
	if ( nBytes > attr.mq_msgsize ) {
		printf("send size over(%d:%ld)\n", nBytes, attr.mq_msgsize);
		nBytes = (int)attr.mq_msgsize;
	}
	/* メッセージサイズが異なる場合にはログ追加 */
	else if ( nBytes != attr.mq_msgsize ) {
//		printf("send size less(%d:%ld)\n", nBytes, attr.mq_msgsize);
	}

	while (1) {
		i_Ret = mq_send(msgQId->mq_id, buffer, nBytes, priority);

		if ( i_Ret != -1 )
			break;

		switch (errno) {
		/**
		 * EINTR
		 * 関数呼び出しがシグナルハンドラによる中断 
		 *  -> リトライ 
		 **/
		case EINTR:
			i_Ret = 0;
			break;
		/**
		 * ETIMEDOUT
		 * タイムアウト 
		 **/
		case ETIMEDOUT:
			break;
		/**
		 * EAGAIN
		 * キューフル(O_NONBLOCK指定のときのみ)
		 * NONBLOCKを使わないのでエラー 
		 **/
		case EAGAIN:
			break;
		/**
		 * EMSGSIZE
		 * 送信サイズの方が大きかった場合 
		 * キューの最大サイズ分に合わせる 
		 **/
		case EMSGSIZE:
			/* すでにリサイズ済みのためエラー */
			break;
		default:
			printf("(%p) unknown errno=%d tid=0x%lx\n",
					msgQId, errno, pthread_self());
			break;
		}

		if ( i_Ret == -1 )
			break;
	}

	return i_Ret;
}

int msgQReceive(MSG_Q_ID msgQId, char *buffer, int maxNBytes)
{
	struct mq_attr attr;
	int i_Ret;
	char *tmpbuf = buffer;
	long tmpsize = maxNBytes;

	if ( msgQId == NULL ) {
		printf("(%p, %p, %d) msgQId=NULL\n",
				msgQId, buffer, maxNBytes);
		return -1;
	}
	
	while (1) {
		i_Ret = mq_receive(msgQId->mq_id, tmpbuf, tmpsize, NULL);
		if ( i_Ret != -1 ) {
			if ( tmpbuf != buffer )
				memcpy(buffer, tmpbuf, maxNBytes); /* 64bit safe */
			break;
		}

		switch (errno) {
		/**
		 * EINTR
		 * 関数呼び出しがシグナルハンドラによる中断 
		 *  -> リトライ 
		 **/
		case EINTR:
			i_Ret = 0;
			break;
		/**
		 * ETIMEDOUT
		 * タイムアウト 
		 **/
		case ETIMEDOUT:
			break;
		/**
		 * EAGAIN
		 * キューが空(O_NONBLOCK指定のときのみ)
		 * NONBLOCKを使わないのでエラー 
		 **/
		case EAGAIN:
			break;
		/**
		 * EMSGSIZE
		 * 受信サイズが小さかった場合、テンポラリバッファを 
		 * 作って受信後、申請サイズ分コピー 
		 **/
		case EMSGSIZE:
			/* すでにリサイズ済みの2回目はエラー */
			if ( tmpbuf != buffer )
				break;

			i_Ret = mq_getattr(msgQId->mq_id, &attr);
			if ( i_Ret == -1 ) {
				printf("(%p) no message\n", msgQId);
				break;
			}

			tmpsize = attr.mq_msgsize;
			tmpbuf = (char*)malloc(tmpsize); /* 64bit safe */
			if ( tmpbuf == NULL )
				i_Ret = -1;
			break;
		default:
			printf("(%p) mq_receive unknown errno=%d tid=0x%lx\n",
					msgQId, errno, pthread_self());
			break;
		}

		if ( i_Ret == -1 )
			break;
	}//while(1)

	if ( tmpbuf && tmpbuf != buffer )
		free(tmpbuf);

	return i_Ret;
}


