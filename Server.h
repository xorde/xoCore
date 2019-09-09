#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QSharedPointer>
#include <QElapsedTimer>

#include "Protocol/ONBPacket.h"
#include "Module/ModuleProxyONB.h"
#include "network/WebSocket/WebSocketServer.h"
#include "xoCore_global.h"
using namespace std;

class XOCORESHARED_EXPORT Server : public QObject
{
    Q_OBJECT        
    typedef QHash<QWebSocket*, ModuleProxyONB*> ConnectionList;
public:
    explicit Server(QObject *parent = nullptr);
    ~Server();

    bool startListening();
    void setPort(quint16 in_port);

    QHash<QString, ModuleProxyONB*> remoteModulesByName;

    ConnectionList getConnections();

    quint64 bytesSent() const {return m_bytesSent;}
    quint64 bytesReceived() const {return m_bytesReceived;}
    quint64 packetsSent() const {return m_packetsSent;}
    quint64 packetsReceived() const {return m_packetsReceived;}
    float transmitBPS() const {return m_bpsTx;}
    float receiveBPS() const {return m_bpsRx;}

    quint16 getPort();

signals:
    // не путаем терминологию модуля и компонента:
    void moduleConnection(ModuleProxyONB* in_pConnection);
    void moduleDisconnection(ModuleProxyONB* in_pConnection);

protected slots:
    void slotTakeTextData(QWebSocket* in_pConnection, QString in_data);
    void slotTakeByteData(QWebSocket* in_pConnection, const QByteArray& in_data);
    void slotTakeDisconnect(QWebSocket* in_pSocket);
    void slotTakeNewConnection(QWebSocket* in_pSocket);

    void setPacketToSend(ModuleProxyONB *in_pConnection, const QByteArray &in_data);
    void sendData(const QByteArray &data);
protected:    
    quint16 m_port;
    ConnectionList m_connections;
    WebSocketServer* m_pWebSocketServer;

    void stopListening();
    void addNewComponent(QWebSocket* in_pSocket, QString in_id);

private:
    QElapsedTimer m_etimer;
    quint64 m_bytesSent = 0, m_bytesReceived = 0;
    quint64 m_bytesSentOld = 0, m_bytesReceivedOld = 0;
    quint64 m_packetsSent = 0, m_packetsReceived = 0;
    float m_bpsTx = 0, m_bpsRx = 0; // bytes per second
};

#endif // SERVER_H
