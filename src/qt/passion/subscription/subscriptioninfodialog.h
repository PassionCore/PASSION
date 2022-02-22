// Copyright (c) 2019 The Passion developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SUBSCRIPTIONINFODIALOG_H
#define SUBSCRIPTIONINFODIALOG_H

#include "askpassphrasedialog.h"
#include "qt/passion/focuseddialog.h"
#include "qt/passion/snackbar.h"

class WalletModel;

namespace Ui {
class SubscriptionInfoDialog;
}

class SubscriptionInfoDialog : public FocusedDialog
{
    Q_OBJECT

public:
    explicit SubscriptionInfoDialog(QWidget *parent = nullptr);
    ~SubscriptionInfoDialog();

    bool exportMN = false;

    void setData(QString privKey, QString name, QString address, QString txId, QString outputIndex, QString status);

public Q_SLOTS:
    void reject() override;

private:
    Ui::SubscriptionInfoDialog *ui;
    SnackBar *snackBar = nullptr;
    int nDisplayUnit = 0;
    WalletModel *model = nullptr;
    QString txId;
    QString pubKey;

    void copyInform(QString& copyStr, QString message);
};

#endif // SUBSCRIPTIONINFODIALOG_H
