#include "config.h"
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

Config::Config() {}

QString Config::getConfigFilePath()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return configPath + "/config.json";
}

void Config::loadConfig()
{
    QFile file(getConfigFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open config file for reading";
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "Failed to parse config file";
        return;
    }

    QJsonObject config = doc.object();

    // 加载上次选择的提供商
    QString lastProvider = config["last_provider"].toString();
    int index = providerCombo->findText(lastProvider);
    if (index >= 0) {
        providerCombo->setCurrentIndex(index);
    }

    // 加载各提供商的配置
    if (config.contains("providers")) {
        QJsonObject providers = config["providers"].toObject();

        // Cloudflare
        if (providers.contains("Cloudflare")) {
            QJsonObject cf = providers["Cloudflare"].toObject();
            cfApiKey->setText(cf["api_key"].toString());
            cfZoneId->setText(cf["zone_id"].toString());
            cfDomain->setText(cf["domain"].toString());
        }

        // Aliyun
        if (providers.contains("Aliyun")) {
            QJsonObject aliyun = providers["Aliyun"].toObject();
            aliyunAccessKey->setText(aliyun["access_key"].toString());
            aliyunSecretKey->setText(aliyun["secret_key"].toString());
            aliyunDomain->setText(aliyun["domain"].toString());
        }

        // DNSPod
        if (providers.contains("DNSPod")) {
            QJsonObject dnspod = providers["DNSPod"].toObject();
            dnspodToken->setText(dnspod["token"].toString());
            dnspodDomain->setText(dnspod["domain"].toString());
        }

        // DuckDNS
        if (providers.contains("DuckDNS")) {
            QJsonObject duckdns = providers["DuckDNS"].toObject();
            duckdnsToken->setText(duckdns["token"].toString());
            duckdnsDomain->setText(duckdns["domain"].toString());
        }
    }
}
