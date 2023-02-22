#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include <QTimer>
#include <vector>
#include <visiontransfer/deviceinfo.h>

namespace Ui {
class ConnectionDialog;
}

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent = 0);
    ~ConnectionDialog();

    const visiontransfer::DeviceInfo& getSelectedDevice() const {return selectedDevice;}

private:
    static constexpr int UPDATE_INTERVAL_MS = 1000;

    Ui::ConnectionDialog *ui;
    std::vector<QTreeWidgetItem> hostItems;
    std::vector<visiontransfer::DeviceInfo> currDeviceList;
    std::vector<visiontransfer::DeviceInfo> prevDeviceList;
    QTimer updateTimer;
    visiontransfer::DeviceInfo selectedDevice;

    void queryDevices();
    void createHostList();
    void connectToHost();
    void openConfigForHost();
};

#endif // CONNECTIONDIALOG_H
