#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QComboBox>

class Config
{
public:
    Config();
    void loadConfig();
    QString getConfigFilePath();
    void saveConfig();

    void setProvider(QComboBox *providerCombo);
};

#endif // CONFIG_H
