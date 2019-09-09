#include "Server.h"

#include <QtWebSockets/QtWebSockets>

Server::Server(QObject *parent) : QObject(parent), m_port(8080), m_pWebSocketServer(nullptr)
{
    m_pWebSocketServer = new WebSocketServer(m_port);
    connect(m_pWebSocketServer, SIGNAL(newConnection(QWebSocket*)), this, SLOT(slotTakeNewConnection(QWebSocket*)), Qt::QueuedConnection);
    connect(m_pWebSocketServer, SIGNAL(clientDisconnect(QWebSocket*)), this, SLOT(slotTakeDisconnect(QWebSocket*)), Qt::QueuedConnection);
    connect(m_pWebSocketServer, SIGNAL(readStringData(QWebSocket*, QString)), this, SLOT(slotTakeTextData(QWebSocket*,QString)), Qt::QueuedConnection);
    connect(m_pWebSocketServer, SIGNAL(readBinaryData(QWebSocket*,QByteArray)), this, SLOT(slotTakeByteData(QWebSocket*,QByteArray)), Qt::QueuedConnection);

    QTimer *measureTimer = new QTimer(this);
    connect(measureTimer, &QTimer::timeout, [=]()
    {
        if (!m_etimer.isValid())
            m_etimer.start();
        else
        {
            static const float Kf = 0.95f;
            float dt = m_etimer.restart() * 0.001f;
            if (dt > 0)
            {
                float bpsTx = (m_bytesSent - m_bytesSentOld) / dt;
                float bpsRx = (m_bytesReceived - m_bytesReceivedOld) / dt;
                m_bpsTx = m_bpsTx * Kf + bpsTx * (1.0f - Kf);
                m_bpsRx = m_bpsRx * Kf + bpsRx * (1.0f - Kf);
                m_bytesSentOld = m_bytesSent;
                m_bytesReceivedOld = m_bytesReceived;
            }
        }
    });
    measureTimer->start(16);
}

Server::~Server()
{
    if (m_pWebSocketServer)
    {
        m_pWebSocketServer->closeAllConnection();
        delete m_pWebSocketServer;
        m_pWebSocketServer = nullptr;
    }
}

void Server::setPacketToSend(ModuleProxyONB* in_pConnection, const QByteArray& in_data)
{
    if (!in_pConnection)
        return;

    QWebSocket* socket = in_pConnection->property("socket").value<QWebSocket*>();
    if (socket)
        socket->sendBinaryMessage(in_data);
}

void Server::sendData(const QByteArray &data)
{
    ModuleProxyONB *proxy = qobject_cast<ModuleProxyONB*>(sender());
    setPacketToSend(proxy, data);

    quint64 sz = static_cast<quint64>(data.size());
    m_packetsSent++;
    m_bytesSent += sz;
}

void Server::slotTakeDisconnect(QWebSocket* in_pSocket)
{    
    if (!in_pSocket)
        return;

    if (!m_connections.contains(in_pSocket))
        return;

    ModuleProxyONB* pConnection = m_connections[in_pSocket];
    m_connections.remove(in_pSocket);
    emit moduleDisconnection(pConnection);
}

void Server::slotTakeNewConnection(QWebSocket* in_pSocket)
{
    m_connections[in_pSocket] = nullptr;
}

bool Server::startListening()
{
    stopListening();
    return m_pWebSocketServer->listen();
}

void Server::setPort(quint16 in_port)
{
    if (m_pWebSocketServer)
    {
        stopListening();
        m_pWebSocketServer->setPort(in_port);
        m_pWebSocketServer->listen();
    }
}

Server::ConnectionList Server::getConnections()
{
    return m_connections;
}

quint16 Server::getPort()
{
    return m_port;
}

void Server::slotTakeTextData(QWebSocket* in_pConnection, QString in_data)
{
    ModuleProxyONB *module = m_connections.value(in_pConnection, nullptr);
    if (!module) // in_data contains the name of the module in this case
        addNewComponent(in_pConnection, in_data);
    else
        qDebug() << module->name() << ">" << in_data;
}

void Server::addNewComponent(QWebSocket* in_pSocket, QString in_id)
{
    auto proxy = new ModuleProxyONB(in_id);
    proxy->setProperty("socket", QVariant::fromValue<QWebSocket*>(in_pSocket));
    connect(proxy, SIGNAL(newDataToSend(QByteArray)), SLOT(sendData(QByteArray)));
    m_connections[in_pSocket] = proxy;

    if(in_pSocket->peerAddress().toString() != "::ffff:127.0.0.1")
        remoteModulesByName[proxy->name()] = proxy;

    emit moduleConnection(proxy);
}

void Server::slotTakeByteData(QWebSocket *in_pConnection, const QByteArray &in_data)
{
    if (!m_connections.contains(in_pConnection))
        return;    

    ModuleProxyONB* pNetConnection = m_connections[in_pConnection];
    if (pNetConnection)
        pNetConnection->receiveData(in_data);
    m_packetsReceived++;
    m_bytesReceived += static_cast<quint64>(in_data.size());
}

void Server::stopListening()
{
    if (m_pWebSocketServer->isListening())
        m_pWebSocketServer->closeAllConnection();
}
