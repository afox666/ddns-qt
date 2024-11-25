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

    void searchCloudflareRecordId(const QString &record);
    void updateCloudflare(const QLineEdit *cfApiKey, const QLineEdit *cfZoneId, const QLineEdit *cfDomain,
                          const QString &ipv4, const QString &ipv6,
                          const QLineEdit *ipv4RecordName, const QLineEdit *ipv6RecordName);
    void handleCloudflareReply(QNetworkReply *reply, bool isIPv4);

private slots:
    void updateCloudflareDns();

signals:
    void searchComplete();

private:
    QNetworkAccessManager *networkManager_;

    QString apiKey_;
    QString zoneId_;
    QString domain_;
    QJsonObject data_;

    // 用于存储记录ID的变量
    QString ipv4RecordId_ = QString();
    QString ipv6RecordId_ = QString();


};

#endif // CLOUDFLARE_H
