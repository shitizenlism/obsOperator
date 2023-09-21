#pragma once

#include "cavthread.h"

using namespace std;

typedef struct MsgInProtoBuf {
	unsigned short len;
	unsigned short nameLen;
	char *typeName;
	unsigned char *protobufData;
	unsigned int checkSum;
} MsgInProtoBuf_T;

#define MSGTYPE_Login		"Login"
#define MSGTYPE_LoginReply 	"LoginReply"
#define MSGTYPE_HeartInfo 	"HeartInfo"

#define MSGTYPE_NtpMsg		"NtpMsg"
#define MSGTYPE_LogMsg 		"LogMsg"
#define MSGTYPE_ManagementMsg 	"ManagementMsg"
#define MSGTYPE_RecordMsg		"RecordMsg"
#define MSGTYPE_RtmpServerMsg	"RtmpServerMsg"
#define MSGTYPE_AudiosMsg		"AudiosMsg"
#define MSGTYPE_VideosMsg		"VideosMsg"

#define DEVICE_TYPE "video_encoder"

class QMsgSockThread : public CAVThread
{
	Q_OBJECT

public:
	QMsgSockThread(QObject *parent=0);
	~QMsgSockThread();

	bool init(string TcpSvrHost, unsigned short port, string entity_type, \
		string entity_id, int retry_interval);
	void finit();

	int msgSndReport();


protected:
	void run();

private:
    string m_TcpSvrHost;
    unsigned short m_TcpPort{11000};

	bool 	m_isLogin;		// login status
	string  m_entity_CUID;	// device UUID(token)

	string 	m_entity_type;	// device type
	string 	m_entity_id;	// device ID
	int 	m_login_retry_interval;	//interval if failed to login(unit:second)

	int m_sockd;

	int msgLogin();
	void msgKeepalive();
	void msgRevHandler();

};


