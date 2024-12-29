#include "rs485serialserver.h"
#include "ui_rs485serialserver.h"

Rs485SerialServer::Rs485SerialServer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Rs485SerialServer) {
    ui->setupUi(this);
}

Rs485SerialServer::~Rs485SerialServer() {
    delete ui;
}
