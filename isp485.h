#ifndef ISP485_H
#define ISP485_H

#include <QWidget>
#include <QButtonGroup>

namespace Ui {
class Isp485;
}

class Isp485 : public QWidget
{
    Q_OBJECT

public:
    explicit Isp485(QWidget *parent = 0);
    ~Isp485();

private:
    Ui::Isp485 *ui;
    QButtonGroup *groupRadio;
};

#endif // ISP485_H
