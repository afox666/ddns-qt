#ifndef COMMON_H
#define COMMON_H

#endif // COMMON_H

#include <QString>
#include <QAbstractSocket>
#include <QHostAddress>

QAbstractSocket::NetworkLayerProtocol getIpType(const QString &ip)
{
    QHostAddress address(ip);
    return address.protocol();
}
