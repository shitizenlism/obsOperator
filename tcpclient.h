#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

int TcpSendPingMsg(std::string host, int port, char resBuf[]);
int TcpSendSceneMsg(std::string host, int port, std::string devId, std::string streamId, std::string scene, char resBuf[]);

#endif
