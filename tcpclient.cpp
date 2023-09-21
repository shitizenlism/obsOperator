#include "tcpsock.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>

#include "EnumCmdID.pb.h"
#include "message.pb.h"

using namespace std;
using namespace pb;

typedef struct{
	int32_t id;
	int32_t len;  //len(body), not including field:id
	char pbData[1];	//body
}MSG_T;

#define TCP_MSG_LEN 500

int GetTimeofday(struct timeval* tv, struct timezone* tz)
{
	time_t clock;
	struct tm tm;
	SYSTEMTIME win_time;

	GetLocalTime(&win_time);

	tm.tm_year = win_time.wYear - 1900;
	tm.tm_mon = win_time.wMonth - 1;
	tm.tm_mday = win_time.wDay;
	tm.tm_hour = win_time.wHour;
	tm.tm_min = win_time.wMinute;
	tm.tm_sec = win_time.wSecond;
	tm.tm_isdst = -1;

	clock = mktime(&tm);

	tv->tv_sec = (long)clock;
	tv->tv_usec = win_time.wMilliseconds * 1000;

	return 0;
}

int64_t GetTimestamp(void)
{
	struct timeval tv;
	GetTimeofday(&tv, NULL);
	return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

int TcpSendPingMsg(string host, int port, char resBuf[])
{
	string resp_s = "";
	cout << "host=" << host << ",port=" << port << endl;
	int ret = 0;
	char buf[TCP_MSG_LEN]={0};
	MSG_T *pMsg= (MSG_T*)buf;

	Ping pingReq;
	pingReq.set_timestamp(GetTimestamp());
	pingReq.set_hello("ping");
	string pingReq_s;
	pingReq.SerializeToString(&pingReq_s);
	pMsg->id = PING;
	strncpy(pMsg->pbData, pingReq_s.c_str(),pingReq_s.size());
	pMsg->len = pingReq_s.length();
	int len = 4 + 4 + pMsg->len;
	//cout << "msg.id=" << pMsg->id << ",len=" << pMsg->len << endl;
	try{
		SocketClient s(host,port);
		ret = s.SendBuf(buf, len);
		
		Ping pingRes;
		memset(buf, 0, sizeof(buf));
		s.RecvBuf(buf, sizeof(buf));
		resp_s = pMsg->pbData;
		pingRes.ParseFromString(resp_s);
		int64_t ts = pingRes.timestamp();
		string msg = pingRes.hello();
		cout << "recv. msg.id="<<pMsg->id<<",len="<<pMsg->len<<endl;
		cout << "recv pbData. ts=" << ts << ",hello=" << msg.c_str() << endl;
		
		sprintf(resBuf,"ts=%lld,%s\n",ts,msg.c_str());
		return strlen(resBuf);
	}
	catch(const char *s){
		cerr<<s<<endl;
	}
	catch(std::string s){
		cerr<<s<<endl;
	}
	catch(...){
		cerr<<"unknown exception happen"<<endl;
	}
	
	return 0;
}

int TcpSendSceneMsg(string host, int port, string devId,  string streamId, string scene, char resBuf[])
{
	string resp_s = "";
	cout << "host=" << host << ",port=" << port << endl;
	int ret = 0;
	char buf[TCP_MSG_LEN]={0};
	MSG_T *pMsg= (MSG_T*)buf;

	//login firstly then send scene-cmd.
	LoginIndication loginReq;
	loginReq.set_timestamp(GetTimestamp());
	loginReq.set_devid(devId);
	loginReq.set_streamid(streamId);
	string loginReq_s;
	loginReq.SerializeToString(&loginReq_s);
	pMsg->id = LOGIN;
	strncpy(pMsg->pbData, loginReq_s.c_str(),loginReq_s.size());
	pMsg->len = loginReq_s.length();
	int len = 4 + 4 + pMsg->len;
	//cout << "msg.id=" << pMsg->id << ",len=" << pMsg->len << endl;
	try{
		SocketClient s(host,port);
		ret = s.SendBuf(buf, len);
		
		LoginReply loginRes;
		memset(buf, 0, sizeof(buf));
		s.RecvBuf(buf, sizeof(buf));
		resp_s = pMsg->pbData;
		loginRes.ParseFromString(resp_s);
		int64_t ts = loginRes.timestamp();
		string result = loginRes.result();
		cout << "recv. msg.id="<<pMsg->id<<",len="<<pMsg->len<<endl;
		cout << "recv pbData. ts=" << ts << ",result=" << result.c_str() << endl;
	
		SceneCommand sceneCmd;
		sceneCmd.set_timestamp(GetTimestamp());
		sceneCmd.set_streamid(streamId);
		sceneCmd.set_scenename(scene);
		sceneCmd.set_inputname("");
		string sceneCmd_s;
		sceneCmd.SerializeToString(&sceneCmd_s);
		pMsg->id = SCENE;
		strncpy(pMsg->pbData, sceneCmd_s.c_str(), sceneCmd_s.size());
		pMsg->len = sceneCmd_s.length();
		int len = 4 + 4 + pMsg->len;
		ret = s.SendBuf(buf, len);

		//Sleep(200);

		SceneResponse sceneRes;
		memset(buf, 0, sizeof(buf));
		s.RecvBuf(buf, sizeof(buf));
		resp_s = pMsg->pbData;
		sceneRes.ParseFromString(resp_s);
		ts = sceneRes.timestamp();
		result = sceneRes.result();
		string currStreamId = sceneRes.streamid();
		string currSceneName = sceneRes.scenename();
		cout << "recv. msg.id=" << pMsg->id << ",len=" << pMsg->len << endl;
		cout << "recv pbData. ts=" << ts << ",result=" << result.c_str() << ",streamId="<<currStreamId<<",sceneName="<<currSceneName<<endl;
		sprintf(resBuf,"ts=%lld,result=%s,streamId=%s,sceneId=%s\n",ts,result.c_str(),currStreamId.c_str(),currSceneName.c_str());
		return strlen(resBuf);
	}
	catch(const char *s){
		cerr<<s<<endl;
	}
	catch(std::string s){
		cerr<<s<<endl;
	}
	catch(...){
		cerr<<"unknown exception happen"<<endl;
	}
	
	return 0;
}


