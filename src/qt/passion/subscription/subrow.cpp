//Copyright (c) 2021 The PASSION CORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/passion/subscription/subrow.h"
#include "qt/passion/subscription/ui_subrow.h"
#include "qt/passion/qtutils.h"
#include <QDateTime>
#include <QString>

SubRow::SubRow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SubRow)
{
    ui->setupUi(this);
    setCssProperty(ui->labelDomain,  "subrow-lbl-domain");
    setCssProperty(ui->labelKey, "subrow-lbl-key");
    setCssProperty(ui->lblValidTitle, "subrow-lbl-valid-title");
    setCssProperty(ui->lblValidValue, "subrow-lbl-valid-value");
    setCssProperty(ui->lblPaymentAddressTitle, "subrow-lbl-paymentaddr-title");
    setCssProperty(ui->lblPaymentAddressValue, "subrow-lbl-paymentaddr-value");
    setCssProperty(ui->lblRegisteredAddressTitle, "subrow-lbl-regaddr-title");
    setCssProperty(ui->lblRegisteredAddressValue, "subrow-lbl-regaddr-value");
    setCssProperty(ui->lblBalanceRegisteredAddressTitle, "subrow-lbl-balance-regaddr-title");
    setCssProperty(ui->lblBalanceRegisteredAddressValue, "subrow-lbl-balance-regaddr-value");
    setCssProperty(ui->lblSiteFeeTitle, "subrow-lbl-sitefee-title");
    setCssProperty(ui->lblSiteFeeValue, "subrow-lbl-sitefee-value");
    setCssProperty(ui->lblDivisory, "subrow-lbl-divisory");
}
void SubRow::updateView(QString domain, QString key, QString paymentaddress,QString sitefee, QString registeraddress, QString registerAddressBalance, QString expire, int status){
    if(domain.length() > 50) {
        ui->labelDomain->setText(domain.left(50)+"...");
    } else {
        ui->labelDomain->setText(domain);
    }
    ui->labelKey->setText(key.left(16) + "..." + key.right(16));
    ui->lblValidValue->setStyleSheet("color:#B3FFFFFF");
    if(status == 8) {
        ui->lblValidTitle->setText("Time left");
        int expi = expire.toInt();
        int timestamp = QDateTime::currentSecsSinceEpoch();
        int hours = (expi - timestamp)/3600;
        int min = ((expi - timestamp)%3600/60);
        QString strh = QString::number(hours);
        QString strmin = QString::number(min).rightJustified(2, '0');
        if((expi-timestamp)<0) {
            ui->lblValidValue->setText("0:00");
            //ui->lblValidValue->setStyleSheet("color:#B3FFFFFF");
        } else {
            ui->lblValidValue->setText(strh+"h:" + strmin+"min");
            ui->lblValidValue->setStyleSheet("color:#008d36");
        }
    } else if(status >= 1 && status < 7) {
        ui->lblValidTitle->setText("Access pending");
        ui->lblValidValue->setText("" +QString::number(status) + " / 6 waiting");
    } else {
        ui->lblValidTitle->setText("Site payment");
        ui->lblValidValue->setText("not paid / expired");
    }
    ui->lblPaymentAddressTitle->setText("Payment to:");
    ui->lblPaymentAddressValue->setText(paymentaddress.left(10) + "..." + paymentaddress.right(10));
    ui->lblRegisteredAddressValue->setText(registeraddress.left(10) + "..." + registeraddress.right(10));
    ui->lblBalanceRegisteredAddressValue->setText(registerAddressBalance);
    ui->lblSiteFeeTitle->setText("Fee / h");
    ui->lblSiteFeeValue->setText(sitefee + " PASSION");
}

SubRow::~SubRow(){
    delete ui;
}
