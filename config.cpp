#include "config.h"
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

int Config::init()
{
    if(!getConfig(config_)) {
        return -1;
    }
    return 0;
}

QString Config::getConfigFilePath()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return configPath + "/config.json";
}

bool Config::getConfig(QJsonObject& config)
{
    const QString configPath = getConfigFilePath();
    if(configPath.isEmpty()) {
        qWarning() << "Config file path is empty";
        return false;
    }

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open config file for reading:" << file.errorString();
        return false;
    }

    const QByteArray jsonData = file.readAll();
    if(jsonData.isEmpty()) {
        qWarning() << "Config file is empty";
        return false;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
    if (doc.isNull()) {
        qWarning() << "Failed to parse config file:" << error.errorString();
        return false;
    }

    config = doc.object();
    return true;
}

QString Config::getLastProviderName()
{
    return config_[KEY_LAST_PROVIDER].toString();
}

QString Config::getIpv4RecordName() {
    return config_["ipv4_record"].toString();
}

QString Config::getIpv6RecordName() {
    return config_["ipv6_record"].toString();
}

bool Config::getProvider(QJsonObject &provider, QString &provider_name)
{
    if(!config_.contains("providers")) {
        return false;
    }
    provider = config_["providers"].toObject();
    return true;
}

bool Config::saveConfig(const QJsonObject &config) {
    // 保存到文件
    QJsonDocument doc(config);
    QFile file(getConfigFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not save configuration file.";
        return false;
    }

    file.write(doc.toJson());
    qInfo() << "Configuration saved successfully.";
    return true;
}
