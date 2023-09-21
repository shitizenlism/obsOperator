#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>

#include <winsock2.h>
#include <windows.h>
#include <WS2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")

#include "qdebug.h"

#include "agle_conf.pb.h"
#include "commu.pb.cc"
#include "pbmsgclt.h"

using namespace ivi::agle;
using namespace ivi;

#define BASE 65521
uint32_t adler32(const unsigned char *buf, uint32_t len)
{
    uint32_t adler=1;
    uint32_t s1 = adler & 0xffff;
    uint32_t s2 = (adler >> 16) & 0xffff;

    uint32_t iLoop;
    for (iLoop = 0; iLoop < len; iLoop++){
        s1 = (s1 + buf[iLoop]) % BASE;
        s2 = (s2 + s1) % BASE;
    }
    return (s2 << 16) + s1;
}

unsigned char *pbmsg_pack(char *typeName, unsigned short nameLen,
						const char *pbbuf, unsigned short pbbuf_len)
{
	unsigned short len=2+2+nameLen+pbbuf_len+4;
	unsigned char *packMsgBuf=(unsigned char *)malloc(len);
	unsigned char *p;
	uint32_t sum=0;
	if (packMsgBuf){
		p=packMsgBuf;
		p[0]=len&0xFF;
		p[1]=len>>8;
		p[2]=nameLen&0xFF;
		p[3]=nameLen>>8;
		memcpy(p+4,typeName,nameLen);
		memcpy(p+nameLen,pbbuf,pbbuf_len);
		sum=adler32(p+2,len-2);
		p[len-4]=sum&0xFF;
		p[len-3]=sum>>8;
		p[len-2]=sum>>16;
		p[len-1]=sum>>24;
	}
	return packMsgBuf;
}

int pbmsg_unpack(unsigned char *packMsgBuf, unsigned short bufLen, char *oTypeName, char *oPbBuf)
{
	unsigned short msgLen=0,nameLen=0,pbbuf_len=0;
	unsigned char *p=packMsgBuf;
	uint32_t sum=0, sum1;

	if (bufLen<=8){
		return -2;
	}
	msgLen=(p[1]<<8) | p[0];
	if (bufLen != msgLen){
		return -3;
	}
	nameLen=(p[3]<<8) | p[2];
	if (nameLen<=1){
		return -4;
	}
	memcpy(oTypeName,p+4,nameLen);
	pbbuf_len = bufLen - nameLen - 8;
	if (pbbuf_len<=1){
		return -5;
	}
	memcpy(oPbBuf,p+nameLen,pbbuf_len);

	sum=adler32(p+2,bufLen-2);
	sum1 = p[bufLen-4] | (p[bufLen-3]<<8) | (p[bufLen-2]<<16) | (p[bufLen-1]<<24);
	if (sum != sum1){
		return -6;
	}

	return pbbuf_len;
}

int pbmsg_process(char *typeName, char *pbBuf, string &entityCUID)
{
	int ret=0;
	string strTemp = pbBuf;
	LoginReply login;
	int svr_ret=0;
	string net_cuid;

	if (typeName==MSGTYPE_LoginReply){
		login.ParseFromString(strTemp);
		svr_ret=login.error();
		net_cuid=login.cuid();
		cout<<"recv login reply msg. error="<<svr_ret<<",cuid="<<net_cuid<<endl;
		if (svr_ret==0){
			ret=0;
			entityCUID=net_cuid;
		}
		else{
			ret=-1;
		}
	}
	else if (typeName==MSGTYPE_HeartInfo){

	}

	return ret;
}

QMsgSockThread::QMsgSockThread(QObject *parent)
	: CAVThread(parent)
{
	m_sockd = -1;
	m_isLogin = false;
	m_entity_CUID = "0xffffffff";

}

QMsgSockThread::~QMsgSockThread()
{
}

bool QMsgSockThread::init(string TcpSvrHost, unsigned short port, string entity_type, string entity_id, int retry_interval)
{
	m_TcpSvrHost = TcpSvrHost;
	m_TcpPort = port;

	m_entity_type = entity_type;
	m_entity_id = entity_id;
	m_login_retry_interval = retry_interval;

	int ret=0;
	WSADATA wsaData;
	SOCKET sockd=-1;

	do{
		int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != NO_ERROR) {
			qCritical() << "WSAStartup failed with error:"<<iResult;
			ret=-1; break;
		}

		sockd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sockd < 0) {
			qCritical()<<"Error at socket(): "<<WSAGetLastError();
			ret=-2; break;
		}

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(m_TcpPort);

		struct in_addr s;
		inet_pton(AF_INET, (PCSTR)m_TcpSvrHost.c_str(), (void *)&s);
		addr.sin_addr.s_addr = s.s_addr;
		memset(&(addr.sin_zero), 0, 8);
		qInfo()<<"inet_pton: "<<s.s_addr;

		iResult = ::connect(sockd, (SOCKADDR*) &addr, sizeof(addr));
		if (iResult == SOCKET_ERROR) {
			qCritical()<<"Unable to connect to server: "<<WSAGetLastError();
			ret=-3; break;
		}

		//prepare login msg
		Login LoginRequest;
		string login_s;
		unsigned char *pData;
		int len=0;

		LoginRequest.set_devtype(m_entity_type);
		LoginRequest.set_tableid(m_entity_id);
		LoginRequest.SerializeToString(&login_s);
		cout<<"send login[]="<<login_s.c_str()<<endl;
		//cout<<"login[],size="<<login_s.size()<<",length="<<login_s.length()<<endl;

		//send login msg
		pData=pbmsg_pack(MSGTYPE_Login, strlen(MSGTYPE_Login), (const char *)login_s.c_str(), login_s.size());
		len=strlen(MSGTYPE_Login)+login_s.size()+8;
		if (!pData){
			ret=-4;break;
		}

		if((ret=send(sockd, (const char*)pData, len, 0))== -1){
			cout << "Error sending data: "<< errno << endl;
			free(pData);
			ret=-5;break;
		}
		else{
			cout<<"send msg done."<<endl;
			free(pData);
		}

		unsigned char revbuf[200]={0};
		len = recv(sockd, (char *)revbuf, sizeof(revbuf), 0);
	    if(len<=0){
	        cout << "Error receiving data: " <<  errno  << endl;
			ret=-6;	break;
		}

		char typeName[128]={0}, pbBuf[200]={0};
		ret=pbmsg_unpack(revbuf, len, typeName, pbBuf);
		if (ret<0){
			cout << "failed when parse data from server. ret=" << ret << endl;
			ret=-7;break;
		}

		ret=pbmsg_process(typeName, pbBuf, m_entity_CUID);

	}while(0);

	if (ret<0){
		if (sockd>=0)
			::closesocket(sockd);
		WSACleanup();
		qWarning()<<"failed to connnect. close socket and exit.";
		return false;
	}
	else{
		m_sockd = sockd;
		qInfo()<<"TCP socket connnects success!";
		return true;
	}

}

void QMsgSockThread::finit()
{
	if (m_sockd>=0)
		::closesocket(m_sockd);
	WSACleanup();

	m_sockd = -1;
}

void QMsgSockThread::run()
{
	int ret=0;

	qInfo()<<"enter QMsgSockThread().";

	while (!m_bStop){

		if (!m_isLogin){
			ret = msgLogin();
			if (ret!=0){
				qWarning()<<"###login failure! ret="<<ret;
				m_isLogin = false;
				sleep(m_login_retry_interval);
				continue;
			}
			else{
				m_isLogin = true;
				qInfo()<<"###login success!";
			}
		}

		sleep(1);
	}

	qInfo()<<"exit QMsgSockThread().";
}

int QMsgSockThread::msgLogin()
{
	int ret=0;
	int len=0;
	char buf[200]={0};

	Login LoginRequest;
	LoginReply LoginReply;
	LoginRequest.set_devtype(m_entity_type);
	LoginRequest.set_tableid(m_entity_id);
	string login_s;
	LoginRequest.SerializeToString(&login_s);
	qInfo()<<"send login[]="<<login_s.c_str();

	if((ret=::send(m_sockd, login_s.c_str(), login_s.size(), 0))== -1){
		qCritical() << "Error sending data: "<< errno;
		return -1;
	}
	else{
		qInfo()<<"send msg done.";
	}

	ret = ::recv(m_sockd, buf, sizeof(buf), 0);
	if(ret<=0){
		qCritical() << "Error receiving data: " << errno;
		return -2;
	}

	string resp_s = buf;
	LoginReply.ParseFromString(resp_s);
	qInfo()<<"recv[]="<<resp_s.c_str();

	int net_error=LoginReply.error();
	string net_cuid=LoginReply.cuid();
	qInfo()<<"recv login reply msg. error="<<net_error<<",cuid="<<net_cuid.c_str();
	if (net_error==0){
		m_entity_CUID=net_cuid;
	}

	return net_error;
}

void QMsgSockThread::msgKeepalive()
{

}

void QMsgSockThread::msgRevHandler()
{

}

int QMsgSockThread::msgSndReport()
{

	return 1;
}





