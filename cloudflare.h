#ifndef CLOUDFLARE_H
#define CLOUDFLARE_H

#include <QWidget>
#include <QString>
#include <QJsonObject>
#include <QNetworkReply>
#include <QLineEdit>

class Cloudflare : public QWidget
{
    Q_OBJECT
public:
    Cloudflare(QNetworkAccessManager *networkManager) : networkManager_(networkManager) {};


    void updateDnsRecord(const QLineEdit *cfApiKey, const QLineEdit *cfZoneId, const QLineEdit *cfDomain,
                          const QString &ipv4, const QString &ipv6,
                          const QLineEdit *ipv4RecordName, const QLineEdit *ipv6RecordName);

    void deleteDnsRecord(bool isIpv4);

private:
    void checkIpRecordMatch(bool isIpv4);
    void createNewRecord(bool isIpv4);
    void updateExistRecord(bool isIpv4);
    void handleCloudflareReply(QNetworkReply *reply, bool isIPv4);
    void searchCloudflareRecordId(bool isIpv4);

private slots:
    void updateCloudflareDns(bool isIpv4);

signals:
    void searchComplete(bool isIpv4);
    void IpRecordExist(bool isIpv4);
    void IpRecordNotMatch(bool isIpv4);

private:
    QNetworkAccessManager *networkManager_;

    QString apiKey_;
    QString zoneId_;
    QString domain_;
    QJsonObject ipv4_data_;
    QJsonObject ipv6_data_;

    // 用于存储记录ID的变量
    QString ipv4RecordId_ = QString();
    QString ipv6RecordId_ = QString();
};

#endif // CLOUDFLARE_H
