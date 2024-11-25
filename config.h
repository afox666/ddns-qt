#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QComboBox>
#include <QJsonObject>

class Config
{
public:
    Config();
    void loadConfig();
    QString getConfigFilePath();
    void saveConfig();

    void setProvider(QComboBox *providerCombo);

public:
    QComboBox *providerCombo;
    QJsonObject provide_config;
    QString lastProvider;
};

#endif // CONFIG_H
