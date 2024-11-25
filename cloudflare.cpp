#include "cloudflare.h"
#include "common.h"

#include <QJsonDocument>
#include <QMessageBox>
#include <QObject>
#include <QJsonArray>
#include <QThread>
#include <QTimer>

void Cloudflare::searchCloudflareRecordId(const QString &record)
{
    QString name = record + "." + domain_;
    QNetworkRequest request(QUrl(QString("https://api.cloudflare.com/client/v4/zones/%1/dns_records?name=%2")
                                     .arg(zoneId_)
                                     .arg(name)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", apiKey_.toUtf8());

    QNetworkReply *reply = networkManager_->get(request);

    // 添加超时处理
    QTimer::singleShot(30000, reply, [reply]() {
        if (reply && reply->isRunning()) {
            qDebug() << "search time out";
            reply->abort();
            reply->deleteLater();
        }
    });

    qDebug() << "search in connect" ;

    connect(reply, &QNetworkReply::errorOccurred, reply, [this, reply](QNetworkReply::NetworkError error) {
        qDebug() << "error: " << error;
        reply->deleteLater();
    });

    reply->setParent(this);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "DDNS Update Error",
                                 QString("Search record ID error: %1")
                                     .arg(reply->errorString()));
            emit searchComplete();
            return;
        }

        QJsonParseError jsonError;
        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            QMessageBox::warning(this, "DDNS Update Error",
                                 QString("JSON parse error: %1")
                                     .arg(jsonError.error));

            emit searchComplete();
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj["success"].toBool()) {
            QString errorMsg = jsonObj["errors"].toArray().first().toObject()["message"].toString();
            QMessageBox::warning(this, "DDNS Update Error",
                                 QString("Search record ID error: %1")
                                     .arg(errorMsg));
            emit searchComplete();
            return;
        }

        QJsonArray result = jsonObj["result"].toArray();
        if(result.size() != 1) {
            QMessageBox::warning(this, "DDNS Update Error",
                                 QString("Record ID count is not equal to 1"));
            emit searchComplete();
            return;
        }

        QJsonObject recordInfo = result.at(0).toObject();
        QString recordId = recordInfo["id"].toString();
        QString recordType = recordInfo["type"].toString();

        // 使用 QMetaObject::invokeMethod 确保在主线程中更新成员变量
        if (!recordId.isEmpty() && !recordType.isEmpty()) {
            QMetaObject::invokeMethod(this, [this, recordId, recordType]() {
                if(recordType == "A") {
                    ipv4RecordId_ = recordId;
                }
                else if(recordType == "AAAA") {
                    ipv6RecordId_ = recordId;
                }
                emit searchComplete();
            }, Qt::QueuedConnection);
        }
        qDebug() << "search cf record id done";
    });
}

void Cloudflare::updateCloudflareDns()
{
    qDebug() << "start update cloudflare dns" ;
    QNetworkRequest request(QUrl(QString("https://api.cloudflare.com/client/v4/zones/%1/dns_records").arg(zoneId_)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", apiKey_.toUtf8());

    QJsonDocument jsonDoc(data_);
    QByteArray jsonData = jsonDoc.toJson();

    QAbstractSocket::NetworkLayerProtocol protocol = getIpType(data_["content"].toString());

    QString recordId = QString();
    bool isIpv4 = false;
    if(protocol == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
        isIpv4 = true;
        recordId = ipv4RecordId_;
    } else if(protocol == QAbstractSocket::NetworkLayerProtocol::IPv6Protocol) {
        isIpv4 = false;
        recordId = ipv6RecordId_;
    } else {
        QMessageBox::warning(this, "DDNS Update Error",
                             QString("ip address is error"));
        return ;
    }

    qDebug() << "recordID: " << recordId;

    if (recordId.isEmpty()) {
        // 创建新记录
        qDebug() << "no record, create new";
        QNetworkReply *reply = networkManager_->post(request, jsonData);
        connect(reply, &QNetworkReply::finished, [this, reply, isIpv4]() {
            handleCloudflareReply(reply, isIpv4);
        });
    } else {
        // 更新现有记录
        QUrl updateUrl(QString("https://api.cloudflare.com/client/v4/zones/%1/dns_records/%2")
                           .arg(zoneId_).arg(recordId));
        QNetworkRequest updateRequest(updateUrl);
        updateRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", apiKey_.toUtf8());

        QNetworkReply *reply = networkManager_->put(updateRequest, jsonData);
        connect(reply, &QNetworkReply::finished, [this, reply, isIpv4]() {
            handleCloudflareReply(reply, isIpv4);
        });
    }
}

void Cloudflare::updateCloudflare(const QLineEdit *cfApiKey, const QLineEdit *cfZoneId, const QLineEdit *cfDomain,
                                  const QString &ipv4, const QString &ipv6,
                                  const QLineEdit *ipv4RecordName, const QLineEdit *ipv6RecordName)
{
    // 获取配置
    apiKey_ = "Bearer " + cfApiKey->text();
    zoneId_ = cfZoneId->text();
    domain_ = cfDomain->text();
    QString ipv4Record = ipv4RecordName->text();
    QString ipv6Record = ipv6RecordName->text();

    // 检查记录名称
    if (!ipv4.isEmpty() && ipv4RecordName->text().isEmpty()) {
        QMessageBox::warning(this, "Configuration Error", "Please specify IPv4 record name");
        return;
    }
    if (!ipv6.isEmpty() && ipv6RecordName->text().isEmpty()) {
        QMessageBox::warning(this, "Configuration Error", "Please specify IPv6 record name");
        return;
    }

    connect(this, &Cloudflare::searchComplete, this, &Cloudflare::updateCloudflareDns);

    qDebug() << "start update dns record";
    qInfo() << "ipv4: " << ipv4;
    qInfo() << "ipv6: " << ipv6;

    // 更新IPv4记录
    if (!ipv4.isEmpty()) {
        data_["type"] = "A";
        data_["name"] = ipv4Record + "." + domain_;
        data_["content"] = ipv4;
        data_["proxied"] = false;

        if(ipv4RecordId_.isEmpty()) {
            searchCloudflareRecordId(ipv4Record);
        }
    }

    // 更新IPv6记录
    if (!ipv6.isEmpty()) {
        data_["type"] = "AAAA";
        data_["name"] = ipv6Record + "." + domain_;
        data_["content"] = ipv6;
        data_["proxied"] = false;

        if(ipv6RecordId_.isEmpty()) {
            searchCloudflareRecordId(ipv6Record);
        }
    }
}

void Cloudflare::handleCloudflareReply(QNetworkReply *reply, bool isIPv4)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj["success"].toBool()) {
            QJsonObject result = jsonObj["result"].toObject();
            QString recordId = result["id"].toString();

            if (isIPv4) {
                ipv4RecordId_ = recordId;
                qDebug() << "Updated IPv4 record ID:" << recordId;
            } else {
                ipv6RecordId_ = recordId;
                qDebug() << "Updated IPv6 record ID:" << recordId;
            }

            QString recordType = isIPv4 ? "IPv4 (A)" : "IPv6 (AAAA)";
            QMessageBox::information(this, "DDNS Update Success",
                                     recordType + " record updated successfully");
        } else {
            QString errorMsg = jsonObj["errors"].toString();
            QMessageBox::warning(this, "DDNS Update Error",
                                 QString("%1 record update failed: %2")
                                     .arg(isIPv4 ? "IPv4" : "IPv6")
                                     .arg(errorMsg));
        }
    } else {
        QMessageBox::warning(this, "DDNS Update Error",
                             QString("%1 record update failed: %2")
                                 .arg(isIPv4 ? "IPv4" : "IPv6")
                                 .arg(reply->errorString()));
    }
    reply->deleteLater();
}
