#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QComboBox>
#include <QJsonObject>

static const QString KEY_LAST_PROVIDER = "last_provider";

class Config
{
public:
    static Config& getInstance() {
        static Config instance;
        return instance;
    }
    Config(const Config &) = delete;
    int init();

    bool saveConfig(const QJsonObject &config);
    bool getProvider(QJsonObject &provider, QString &provider_name);

    QString getLastProviderName();
    QString getIpv4RecordName();
    QString getIpv6RecordName();
private:
    Config() = default;
    ~Config() = default;
    bool getConfig(QJsonObject &config);
    QString getConfigFilePath();

private:
    QJsonObject config_;
};

#endif // CONFIG_H
