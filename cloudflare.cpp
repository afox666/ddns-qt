#include "cloudflare.h"
#include "common.h"

#include <QJsonDocument>
#include <QMessageBox>
#include <QObject>
#include <QJsonArray>
#include <QThread>
#include <QTimer>
#include <QSharedPointer>

void Cloudflare::updateDnsRecord(const QLineEdit *cfApiKey, const QLineEdit *cfZoneId, const QLineEdit *cfDomain,
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

    qInfo("start update dns record");
    qInfo() << QString("ipv4: %1").arg(ipv4);
    qInfo() << QString("ipv6: %1").arg(ipv6);

    // 更新IPv4记录
    if (!ipv4.isEmpty()) {
        ipv4_data_["type"] = "A";
        ipv4_data_["name"] = ipv4Record + "." + domain_;
        ipv4_data_["content"] = ipv4;
        if(ipv4RecordId_.isEmpty()) {
            searchCloudflareRecordId(true);
        }
    }

    // 更新IPv6记录
    if (!ipv6.isEmpty()) {
        ipv6_data_["type"] = "AAAA";
        ipv6_data_["name"] = ipv6Record + "." + domain_;
        ipv6_data_["content"] = ipv6;

        if(ipv6RecordId_.isEmpty()) {
            searchCloudflareRecordId(false);
        }
    }
}

void Cloudflare::searchCloudflareRecordId(bool isIpv4)
{
    QString name = isIpv4 ? ipv4_data_["name"].toString() : ipv6_data_["name"].toString();
    QNetworkRequest request(QUrl(QString("https://api.cloudflare.com/client/v4/zones/%1/dns_records?name=%2")
                                     .arg(zoneId_)
                                     .arg(name)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", apiKey_.toUtf8());

    QNetworkReply *reply = networkManager_->get(request);

    // 添加超时处理
    QTimer::singleShot(30000, reply, [reply]() {
        if (reply && reply->isRunning()) {
            qWarning("search time out");
            reply->abort();
            reply->deleteLater();
        }
    });

    qDebug("search in connect");

    connect(reply, &QNetworkReply::errorOccurred, reply, [this, reply](QNetworkReply::NetworkError error) {
        qCritical() << QString("error: %1").arg(error);
        reply->deleteLater();
    });

    reply->setParent(this);
    connect(reply, &QNetworkReply::finished, this, [this, reply, isIpv4]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "DDNS Update Error",
                                 QString("Search record ID error: %1")
                                     .arg(reply->errorString()));
            emit searchComplete(isIpv4);
            return;
        }

        QJsonParseError jsonError;
        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            QMessageBox::warning(this, "DDNS Update Error",
                                 QString("JSON parse error: %1")
                                     .arg(jsonError.error));

            emit searchComplete(isIpv4);
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj["success"].toBool()) {
            QString errorMsg = jsonObj["errors"].toArray().first().toObject()["message"].toString();
            QMessageBox::warning(this, "DDNS Update Error",
                                 QString("Search record ID error: %1")
                                     .arg(errorMsg));
            emit searchComplete(isIpv4);
            return;
        }

        QJsonArray result = jsonObj["result"].toArray();
        if(result.size() != 1) {
            QMessageBox::warning(this, "DDNS Update Error",
                                 QString("Record ID count is not equal to 1"));
            emit searchComplete(isIpv4);
            return;
        }

        QJsonObject recordInfo = result.at(0).toObject();
        QString recordId = recordInfo["id"].toString();
        QString recordType = recordInfo["type"].toString();

        // 使用 QMetaObject::invokeMethod 确保在主线程中更新成员变量
        if (!recordId.isEmpty() && !recordType.isEmpty()) {
            QMetaObject::invokeMethod(this, [this, recordId, recordType, isIpv4]() {
                if(recordType == "A") {
                    ipv4RecordId_ = recordId;
                }
                else if(recordType == "AAAA") {
                    ipv6RecordId_ = recordId;
                }
                emit searchComplete(isIpv4);
            }, Qt::QueuedConnection);
        }
        qInfo("search cf record id done");
    });
}

// TODO 待测试
void Cloudflare::deleteDnsRecord(bool isIpv4)
{
    QString recordId = QString();
    if(isIpv4) {
        recordId = ipv4RecordId_;
    } else {
        recordId = ipv6RecordId_;
    }

    QNetworkRequest request(QUrl(QString("https://api.cloudflare.com/client/v4/zones/%1/dns_records/%2")
                                     .arg(zoneId_)
                                     .arg(recordId)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", apiKey_.toUtf8());

    QNetworkReply *reply = networkManager_->deleteResource(request);
    connect(reply, &QNetworkReply::errorOccurred, reply, [this, reply](QNetworkReply::NetworkError error) {
        qCritical() << QString("error: %1").arg(error);
        reply->deleteLater();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, recordId]() {
        reply->deleteLater();

        QJsonParseError jsonError;
        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            QMessageBox::warning(this, "DDNS Delete Error",
                                 QString("JSON parse error: %1")
                                     .arg(jsonError.error));

            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject result = jsonObj["result"].toObject();
        if(result["id"] != recordId) {
            QMessageBox::warning(this, "DDNS Delete Error",
                                 QString("cf call return id not matched!"));

            return ;
        }
    });
}

void Cloudflare::checkIpRecordMatch(bool isIpv4)
{
    QString recordId = QString();
    if(isIpv4) {
        recordId = ipv4RecordId_;
    } else {
        recordId = ipv6RecordId_;
    }
    QNetworkRequest request(QUrl(QString("https://api.cloudflare.com/client/v4/zones/%1/dns_records/%2")
                                     .arg(zoneId_)
                                     .arg(recordId)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", apiKey_.toUtf8());

    QNetworkReply *reply = networkManager_->get(request);
    connect(reply, &QNetworkReply::errorOccurred, reply, [this, reply](QNetworkReply::NetworkError error) {
        qCritical() << QString("error: %1").arg(error);
        reply->deleteLater();
    });

    connect(this, &Cloudflare::IpRecordNotMatch, this, &Cloudflare::updateExistRecord);

    connect(reply, &QNetworkReply::finished, this, [this, reply, isIpv4]() {
        reply->deleteLater();

        QJsonParseError jsonError;
        QByteArray data = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            QMessageBox::warning(this, "DDNS Check Error",
                                 QString("JSON parse error: %1")
                                     .arg(jsonError.error));

            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj["success"].toBool()) {
            QString errorMsg = jsonObj["errors"].toArray().first().toObject()["message"].toString();
            QMessageBox::warning(this, "DDNS Check Error",
                                 QString("cloudflare call fails: %1")
                                     .arg(errorMsg));
            return;
        }

        QJsonObject result = jsonObj["result"].toObject();
        QString ip_content = isIpv4 ? ipv4_data_["content"].toString() : ipv6_data_["content"].toString();

        qDebug() << QString("call return: %1").arg(result["content"].toString());
        qDebug() << QString("ip_content: %1").arg(ip_content);
        if(result["content"].toString() != ip_content) {
            emit IpRecordNotMatch(isIpv4);
            return ;
        }

        qInfo("IP Record matched, not update.");
    });
}

void Cloudflare::createNewRecord(bool isIpv4)
{
    // 创建新记录
    qInfo("no record, create new");
    QNetworkRequest request(QUrl(QString("https://api.cloudflare.com/client/v4/zones/%1/dns_records").arg(zoneId_)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", apiKey_.toUtf8());

    QJsonObject data = isIpv4 ? ipv4_data_ : ipv6_data_;
    QJsonDocument jsonDoc(data);
    QByteArray jsonData = jsonDoc.toJson();

    QNetworkReply *reply = networkManager_->post(request, jsonData);
    reply->setParent(this);
    connect(reply, &QNetworkReply::finished, this, [this, reply, isIpv4]() {
        handleCloudflareReply(reply, isIpv4);
    });
}

void Cloudflare::updateExistRecord(bool isIpv4)
{
    QNetworkRequest request(QUrl(QString("https://api.cloudflare.com/client/v4/zones/%1/dns_records").arg(zoneId_)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", apiKey_.toUtf8());

    QJsonObject data = isIpv4 ? ipv4_data_ : ipv6_data_;
    QJsonDocument jsonDoc(data);
    QByteArray jsonData = jsonDoc.toJson();

    QString recordId = isIpv4 ? ipv4RecordId_ : ipv6RecordId_;
    // 更新现有记录
    QUrl updateUrl(QString("https://api.cloudflare.com/client/v4/zones/%1/dns_records/%2")
                       .arg(zoneId_).arg(recordId));
    QNetworkRequest updateRequest(updateUrl);
    updateRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    updateRequest.setRawHeader("Authorization", apiKey_.toUtf8());

    qInfo("start update dns");
    qDebug() << QString("start request: %1").arg(updateUrl.toString());

    QNetworkReply *reply = networkManager_->put(updateRequest, jsonData);

    connect(reply, &QNetworkReply::errorOccurred, reply, [this, reply](QNetworkReply::NetworkError error) {
        qWarning() <<  QString("error: %1").arg(error);
        reply->deleteLater();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, isIpv4]() {
        handleCloudflareReply(reply, isIpv4);
        reply->deleteLater();
    });
}

void Cloudflare::updateCloudflareDns(bool isIpv4)
{
    qInfo( "start update cloudflare dns" );
    QString recordId = isIpv4 ? ipv4RecordId_ : ipv6RecordId_;
    qDebug() << QString("recordID: %1").arg(recordId);

    connect(this, &Cloudflare::IpRecordExist, this, &Cloudflare::checkIpRecordMatch);

    if (recordId.isEmpty()) {
        createNewRecord(isIpv4);
    } else {
        qInfo() << "record exist.";
        emit IpRecordExist(isIpv4);
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
            if (!recordId.isEmpty()) {
                QMetaObject::invokeMethod(this, [this, recordId, isIPv4]() {
                    if(isIPv4) {
                        ipv4RecordId_ = recordId;
                        qDebug() << QString("Updated IPv4 record ID: %1").arg(recordId);
                    }
                    else {
                        ipv6RecordId_ = recordId;
                        qDebug() << QString("Updated IPv6 record ID: %1").arg(recordId);
                    }
                }, Qt::QueuedConnection);
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
}
