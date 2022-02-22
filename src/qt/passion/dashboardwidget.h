// Copyright (c) 2019-2020 The Passion developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include "qt/passion/pwidget.h"
#include "qt/passion/furabstractlistitemdelegate.h"
#include "qt/passion/furlistrow.h"
#include "transactiontablemodel.h"
#include "qt/passion/txviewholder.h"
#include "transactionfilterproxy.h"

#include <atomic>
#include <cstdlib>
#include <QWidget>
#include <QLineEdit>
#include <QMap>

#if defined(HAVE_CONFIG_H)
#include "config/passion-config.h" /* for USE_QTCHARTS */
#endif

class PassionGUI;
class WalletModel;

namespace Ui {
class DashboardWidget;
}

class SortEdit : public QLineEdit{
    Q_OBJECT
public:
    explicit SortEdit(QWidget* parent = nullptr) : QLineEdit(parent){}

    inline void mousePressEvent(QMouseEvent *) override{
        Q_EMIT Mouse_Pressed();
    }

    ~SortEdit() override{}

Q_SIGNALS:
    void Mouse_Pressed();

};

enum SortTx {
    DATE_DESC = 0,
    DATE_ASC = 1,
    AMOUNT_DESC = 2,
    AMOUNT_ASC = 3
};


QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class DashboardWidget : public PWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(PassionGUI* _window);
    ~DashboardWidget();

    void loadWalletModel() override;
    void loadChart();
    int setNumBlocks(int blocks);
    int incommingChatMessage(QString strAddress, QString strMessage);


public Q_SLOTS:
    void walletSynced(bool isSync);
    /**
     * Show incoming transaction notification for new transactions.
     * The new items are those between start and end inclusive, under the given parent item.
    */
    void processNewTransaction(const QModelIndex& parent, int start, int /*end*/);

Q_SIGNALS:
    /** Notify that a new transaction appeared */
    void incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address);
private Q_SLOTS:
    void handleTransactionClicked(const QModelIndex &index);
    void changeTheme(bool isLightTheme, QString &theme) override;
    void onSortChanged(const QString&);
    void onSortTypeChanged(const QString& value);
    void updateDisplayUnit();
    void combineUTXOs();
    void showList();
    void onChatSendButtonClicked();
    void onTxArrived(const QString& hash, const bool& isCoinStake, const bool& isCSAnyType);
    void on_tabWidget_tabBarClicked(int index);
    bool chatDestinationAddressChanged(const QString& str);

private:
    Ui::DashboardWidget *ui{nullptr};
    FurAbstractListItemDelegate* txViewDelegate{nullptr};
    TransactionFilterProxy* filter{nullptr};
    TxViewHolder* txHolder{nullptr};
    TransactionTableModel* txModel{nullptr};
    int nDisplayUnit{-1};
    int nTabIndex{0};
    bool isSync{false};

    void changeSort(int nSortIndex);
};

#endif // DASHBOARDWIDGET_H
