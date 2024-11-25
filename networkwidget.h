#ifndef NETWORKWIDGET_H
#define NETWORKWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QNetworkInterface>

class NetworkWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkWidget(QWidget *parent = nullptr);

public slots:
    void refreshNetworkInterfaces();
    void onInterfaceSelected(int index);

private:
    QComboBox *interfaceComboBox;
    QLabel *ipInfoLabel;
    void setupUI();
    QString getInterfaceInfo(const QNetworkInterface &interface);
};

#endif // NETWORKWIDGET_H
