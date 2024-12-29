#ifndef RS485SERIALSERVER_H
#define RS485SERIALSERVER_H

#include <QWidget>

namespace Ui {
class Rs485SerialServer;
}

class Rs485SerialServer : public QWidget
{
    Q_OBJECT

public:
    explicit Rs485SerialServer(QWidget *parent = 0);
    ~Rs485SerialServer();

private:
    Ui::Rs485SerialServer *ui;
};

#endif // RS485SERIALSERVER_H
