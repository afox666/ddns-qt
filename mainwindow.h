// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QLineEdit>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include <QNetworkAccessManager>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onProviderChanged(int index);
    void saveConfig();
    void updateIPAddresses();
    void handleIPv4Reply(QNetworkReply *reply);
    void handleIPv6Reply(QNetworkReply *reply);

private:
    void createCloudFlarePage();
    void createAliyunPage();
    void createDNSPodPage();
    void createDuckDNSPage();
    void setupIPAddressDisplay();

    QComboBox *providerCombo;
    QStackedWidget *stackedWidget;

    // IP address labels
    QLabel *ipv4Label;
    QLabel *ipv6Label;
    QTimer *ipUpdateTimer;
    QNetworkAccessManager *networkManager;

    // Cloudflare inputs
    QLineEdit *cfEmail;
    QLineEdit *cfApiKey;
    QLineEdit *cfZoneId;
    QLineEdit *cfDomain;

    // Aliyun inputs
    QLineEdit *aliyunAccessKey;
    QLineEdit *aliyunSecretKey;
    QLineEdit *aliyunDomain;

    // DNSPod inputs
    QLineEdit *dnspodToken;
    QLineEdit *dnspodDomain;

    // DuckDNS inputs
    QLineEdit *duckdnsToken;
    QLineEdit *duckdnsDomain;
};

#endif // MAINWINDOW_H
