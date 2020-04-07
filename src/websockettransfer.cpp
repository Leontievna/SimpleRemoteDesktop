#include "websockettransfer.h"

WebSocketTransfer::WebSocketTransfer(QObject *parent) : QObject(parent),
    m_webSocketServer(Q_NULLPTR),
    m_port(8081)
{

}

void WebSocketTransfer::start()
{
    if(m_webSocketServer)
        return;

    m_webSocketServer = new QWebSocketServer("Web Socket Server",QWebSocketServer::NonSecureMode,this);

    if(m_webSocketServer->listen(QHostAddress::Any, m_port))
    {
        qDebug()<<"OK: Web-Server is started on port: "<<m_port;
        connect(m_webSocketServer,&QWebSocketServer::newConnection,this,&WebSocketTransfer::setSocketConnected);
    }
    else qDebug()<<"ERROR: Web-Server is not started on port:"<<m_port;
}

void WebSocketTransfer::stop()
{
    if(m_webSocketServer)
    {
        if(m_webSocketServer->isListening())
            m_webSocketServer->close();
    }

    emit finished();
}

void WebSocketTransfer::setPort(quint16 port)
{
    m_port = port;
}

void WebSocketTransfer::setLoginPass(const QString &login, const QString &pass)
{
    m_login = login;
    m_pass = pass;
}

void WebSocketTransfer::setSocketConnected()
{
    QWebSocket *webSocket = m_webSocketServer->nextPendingConnection();

    WebSocketHandler *socketHandler = new WebSocketHandler(this);
    connect(socketHandler, &WebSocketHandler::disconnected, this, &WebSocketTransfer::socketDisconnected);
    socketHandler->setLoginPass(m_login, m_pass);
    socketHandler->setSocket(webSocket);
    m_sockets.append(socketHandler);

    emit newSocketConnected(socketHandler);
}

void WebSocketTransfer::socketDisconnected(WebSocketHandler *pointer)
{
    bool isDisconnectedAll = false;

    if(m_sockets.contains(pointer))
    {
        m_sockets.removeOne(pointer);

        if(m_sockets.size() == 0)
            isDisconnectedAll = true;
    }

    if(pointer)
    {
        pointer->disconnect();
        pointer->deleteLater();
    }

    if(isDisconnectedAll)
        emit disconnectedAll();
}