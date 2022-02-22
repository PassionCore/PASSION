// Copyright (c) 2019-2020 The Passion developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/passion/dashboardwidget.h"
#include "qt/passion/forms/ui_dashboardwidget.h"
#include "qt/passion/sendconfirmdialog.h"
#include "qt/passion/txrow.h"
#include "qt/passion/qtutils.h"
#include "qt/passion/guitransactionsutils.h"
#include "guiutil.h"
#include "coincontrol.h"
#include "walletmodel.h"
#include "clientmodel.h"
#include "optionsmodel.h"
#include "multitier.h"
#include "chat.h"
#include "utiltime.h"
#include <QPainter>
#include <QModelIndex>
#include <QList>
#include <QGraphicsLayout>
#include <QPropertyAnimation>

#define DECORATION_SIZE 65
#define NUM_ITEMS 3
#define SHOW_EMPTY_CHART_VIEW_THRESHOLD 4000
#define REQUEST_LOAD_TASK 1
#define CHART_LOAD_MIN_TIME_INTERVAL 15

DashboardWidget::DashboardWidget(PassionGUI* parent) :
    PWidget(parent),
    ui(new Ui::DashboardWidget)
{
    ui->setupUi(this);

    txHolder = new TxViewHolder(isLightTheme());
    txViewDelegate = new FurAbstractListItemDelegate(
        DECORATION_SIZE,
        txHolder,
        this
    );

    this->setStyleSheet(parent->styleSheet());
    this->setContentsMargins(0,0,0,0);

    // Containers
    setCssProperty({this, ui->left}, "container");
    ui->left->setContentsMargins(0,0,0,0);

    // Title
    setCssTitleScreen(ui->labelTitle);

    //Statistics
    setCssTitleScreen(ui->lblStatisticsTitle);
    setCssProperty({ui->grpOnChainStats,ui->grpStakingStats,ui->grpWalletStats}, "group-box-statistics");
    setCssTitleScreen(ui->lblChatTitle);
    setCssProperty(ui->right, "chat-container-tabwidget");
    setCssProperty(ui->tabStatistics, "tab-statistics-widget");
    setCssProperty(ui->tabChat, "tab-chat-widget");
    setCssProperty(ui->tabWidget, "tabWidgetDashboard");
    setCssProperty(ui->tabWidget->tabBar(), "tabWidgetDashboard");
    setCssProperty({ui->lblCurrentBlockHeight,
                    ui->lblCurrentRewards,
                    ui->lblUpcommingRewards,
                    ui->lblMoneySupplyShielded,
                    ui->lblMoneySupplyTransparent,
                    ui->lblAverageBlocks}, "group-box-statistics-label");
    setCssProperty({ui->lblBlockHeightValue,
                    ui->lblCurrentRewardsValue,
                    ui->lblUpcommingRewardsValue,
                    ui->lblMoneySupplyShieldedValue,
                    ui->lblMoneySupplyTransparentValue,
                    ui->lblAvergaeBlocksValue}, "group-box-statistics-value");
    setCssProperty({ui->lblPoSStaking,
                    ui->lblPoSTier1,
                    ui->lblPoSTier2,
                    ui->lblPoSTier3,
                    ui->lblPoSTier4}, "group-box-statistics-label");
    setCssProperty({ui->lblPoSStakingValue,
                    ui->lblPoSTier1Value,
                    ui->lblPoSTier2Value,
                    ui->lblPoSTier3Value,
                    ui->lblPoSTier4Value}, "group-box-statistics-value");
    setCssProperty({ui->lblUserStakeCoins,
                    ui->lblUserStakeRewards,
                    ui->lblUserStakeTier1,
                    ui->lblUserStakeTier2,
                    ui->lblUserStakeTier3,
                    ui->lblUserStakeTier4}, "group-box-statistics-label");
    setCssProperty({ui->lblUserStakeCoinsValue,
                    ui->lblUserStakeRewardsValue,
                    ui->lblUserStakeTier1Value,
                    ui->lblUserStakeTier2Value,
                    ui->lblUserStakeTier3Value,
                    ui->lblUserStakeTier4Value }, "group-box-statistics-value");

    setCssSubtitleScreen(ui->lblChatMessageDestination);
    setCssSubtitleScreen(ui->lblChatMessageToSend);
    setCssSubtitleScreen(ui->lblChatMessageReceived);
    setCssBtnSecondary(ui->btnSendMessage);
    setCssProperty(ui->txtChatMessageReceived, "chat-text-message-received");
    setCssProperty(ui->txtChatMessageToSend, "chat-text-message-to-send");
    ui->lblStatisticsTitle->setText("Various statistics");
    ui->lblChatTitle->setText("Anonymous Chat");
    ui->lblChatMessageDestination->setText("Destination Address:");
    ui->lblChatMessageReceived->setText("Received Messages:");
    ui->lblChatMessageToSend->setText("Message to send:");
    connect(ui->btnSendMessage, &QPushButton::clicked, this, &DashboardWidget::onChatSendButtonClicked);
    connect(ui->txtChatDestinationAddress, &QLineEdit::textChanged, [this](){chatDestinationAddressChanged(ui->txtChatDestinationAddress->text());});
    setCssProperty({ui->txtChatDestinationAddress}, "chat-primary-message");
    nTabIndex = 0;
    ui->tabWidget->setCurrentIndex(0);
    ui->right->setMinimumWidth(550);
    ui->tabWidget->setObjectName("tabpane");

    /* Subtitle */
    setCssSubtitleScreen(ui->labelSubtitle);

    // Sort Transactions
    SortEdit* lineEdit = new SortEdit(ui->comboBoxSort);
    connect(lineEdit, &SortEdit::Mouse_Pressed, [this](){ui->comboBoxSort->showPopup();});
    setSortTx(ui->comboBoxSort, lineEdit);
    connect(ui->comboBoxSort, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged), this, &DashboardWidget::onSortChanged);
    // Sort type
    SortEdit* lineEditType = new SortEdit(ui->comboBoxSortType);
    connect(lineEditType, &SortEdit::Mouse_Pressed, [this](){ui->comboBoxSortType->showPopup();});
    setSortTxTypeFilter(ui->comboBoxSortType, lineEditType);
    ui->comboBoxSortType->setCurrentIndex(0);
    connect(ui->comboBoxSortType, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentTextChanged),
        this, &DashboardWidget::onSortTypeChanged);

    // Statistics
    setCssProperty(ui->tabWidget, "container-statistics");

    // Transactions
    setCssProperty(ui->listTransactions, "container");
    ui->listTransactions->setItemDelegate(txViewDelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listTransactions->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->listTransactions->setUniformItemSizes(true);

    // Sync Warning
    ui->layoutWarning->setVisible(true);
    ui->lblWarning->setText(tr("Please wait until the wallet is fully synced to see your correct balance"));
    setCssProperty(ui->lblWarning, "text-warning");
    setCssProperty(ui->imgWarning, "ic-warning");

    //Empty List
    ui->emptyContainer->setVisible(false);
    setCssProperty(ui->pushImgEmpty, "img-empty-transactions");
    setCssProperty(ui->labelEmpty, "text-empty");
    setCssBtnSecondary(ui->btnHowTo);
    connect(ui->listTransactions, &QListView::clicked, this, &DashboardWidget::handleTransactionClicked);
}

void DashboardWidget::handleTransactionClicked(const QModelIndex &index)
{
    ui->listTransactions->setCurrentIndex(index);
    QModelIndex rIndex = filter->mapToSource(index);

    window->showHide(true);
    TxDetailDialog *dialog = new TxDetailDialog(window, false);
    dialog->setData(walletModel, rIndex);
    openDialogWithOpaqueBackgroundY(dialog, window, 3, 17);

    // Back to regular status
    ui->listTransactions->scrollTo(index);
    ui->listTransactions->clearSelection();
    ui->listTransactions->setFocus();
    dialog->deleteLater();
}

void DashboardWidget::loadWalletModel()
{
    if (walletModel && walletModel->getOptionsModel()) {
        txModel = walletModel->getTransactionTableModel();
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setDynamicSortFilter(true);
        filter->setSortCaseSensitivity(Qt::CaseInsensitive);
        filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
        filter->setSortRole(Qt::EditRole);

        // Read filter settings
        QSettings settings;
        quint32 filterByType = settings.value("transactionType", TransactionFilterProxy::ALL_TYPES).toInt();
        int filterIndex = ui->comboBoxSortType->findData(filterByType); // Find index
        filterByType = (filterIndex == -1) ? TransactionFilterProxy::ALL_TYPES : filterByType;
        filter->setTypeFilter(filterByType); // Set filter
        ui->comboBoxSortType->setCurrentIndex(filterIndex); // Set item in ComboBox
        // Read sort settings
        changeSort(settings.value("transactionSort", SortTx::DATE_DESC).toInt());

        filter->setSourceModel(txModel);
        txHolder->setFilter(filter);
        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        if (txModel->size() == 0) {
            ui->emptyContainer->setVisible(true);
            ui->listTransactions->setVisible(false);
            ui->comboBoxSortType->setVisible(false);
            ui->comboBoxSort->setVisible(false);
        }

        connect(ui->pushImgEmpty, &QPushButton::clicked, [this](){window->openFAQ();});
        connect(ui->btnHowTo, &QPushButton::clicked, [this](){window->openFAQ();});
        connect(txModel, &TransactionTableModel::txArrived, this, &DashboardWidget::onTxArrived);

        // Notification pop-up for new transaction
        connect(txModel, &TransactionTableModel::rowsInserted, this, &DashboardWidget::processNewTransaction);
    }

    setNumBlocks(clientModel->getNumBlocks());
    connect(clientModel, &ClientModel::numBlocksChanged, this, &DashboardWidget::setNumBlocks);
    connect(clientModel, &ClientModel::newChatMessage, this, &DashboardWidget::incommingChatMessage);
    // update the display unit, to not use the default ("PASSION")
    updateDisplayUnit();
}
int DashboardWidget::incommingChatMessage(QString strAddress, QString strMessage) {

    QString from ="<b>"+strAddress+" wrote: </b><br>" + strMessage +"<br>";
    QString history = ui->txtChatMessageReceived->toHtml();
    ui->txtChatMessageReceived->setHtml(from+history);
    return 1;
}
void DashboardWidget::onTxArrived(const QString& hash, const bool& isCoinStake, const bool& isCSAnyType)
{
    showList();
}

void DashboardWidget::showList()
{
    if (txModel->size() == 0) {
        ui->emptyContainer->setVisible(true);
        ui->listTransactions->setVisible(false);
        ui->comboBoxSortType->setVisible(false);
        ui->comboBoxSort->setVisible(false);
    } else {
        ui->emptyContainer->setVisible(false);
        ui->listTransactions->setVisible(true);
        ui->comboBoxSortType->setVisible(true);
        ui->comboBoxSort->setVisible(true);
    }
}

void DashboardWidget::updateDisplayUnit()
{
    if (walletModel && walletModel->getOptionsModel()) {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        txHolder->setDisplayUnit(nDisplayUnit);
        ui->listTransactions->update();
    }
}
void DashboardWidget::on_tabWidget_tabBarClicked(int index) {
    int maxWidth = ui->right->maximumWidth();
    int minWidth = ui->right->minimumWidth();
    if(nTabIndex == index) {
        if(maxWidth == 50 && minWidth == 50) {
            ui->right->setMaximumWidth(550);
            ui->right->setMinimumWidth(550);
        } else {
            ui->right->setMaximumWidth(50);
            ui->right->setMinimumWidth(50);
        }
    }
    this->nTabIndex = index;
}
void DashboardWidget::onChatSendButtonClicked() {
    QString message = ui->txtChatMessageToSend->toPlainText();
    QString destination = ui->txtChatDestinationAddress->text().trimmed();
    bool staking_only = false;
    bool is_shielded = false;
    const bool valid = walletModel->validateAddress(destination, staking_only, is_shielded);
    if(!valid) {
        inform("No valid Passion address.");
        return;
    }
    if(message.length() > 1000) {
        inform("Message size greater than allowed (1000)");
        return;
    }
    if(message.length() < 1) {
        inform("Message empty nothing to send!");
        return;
    }
    if (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) == 0) {
        inform("No active connections to Passion Network, can't send message!");
        return;
    }
    CChat::getInstance()->AddMessageToSend(destination.toStdString(),message.toStdString());
    ui->txtChatMessageToSend->setText("");
    inform("Message sent to "+ destination);
}

void DashboardWidget::onSortChanged(const QString& value)
{
    if (!filter) return;

    if (!value.isNull()) {
        changeSort(ui->comboBoxSort->currentIndex());
    } else {
        changeSort(SortTx::DATE_DESC);
    }
}

void DashboardWidget::changeSort(int nSortIndex)
{
    int nColumnIndex = TransactionTableModel::Date;
    Qt::SortOrder order = Qt::DescendingOrder;

    switch (nSortIndex) {
        case SortTx::DATE_DESC:
        {
            nColumnIndex = TransactionTableModel::Date;
            break;
        }
        case SortTx::DATE_ASC:
        {
            nColumnIndex = TransactionTableModel::Date;
            order = Qt::AscendingOrder;
            break;
        }
        case SortTx::AMOUNT_DESC:
        {
            nColumnIndex = TransactionTableModel::Amount;
            break;
        }
        case SortTx::AMOUNT_ASC:
        {
            nColumnIndex = TransactionTableModel::Amount;
            order = Qt::AscendingOrder;
            break;
        }
    }

    ui->comboBoxSort->setCurrentIndex(nSortIndex);
    filter->sort(nColumnIndex, order);

    // Store settings
    QSettings settings;
    settings.setValue("transactionSort", nSortIndex);
}

void DashboardWidget::onSortTypeChanged(const QString& value)
{
    if (!filter) return;
    int filterIndex = ui->comboBoxSortType->currentIndex();
    int filterByType = ui->comboBoxSortType->itemData(filterIndex).toInt();

    filter->setTypeFilter(filterByType);
    ui->listTransactions->update();

    if (filter->rowCount() == 0) {
        ui->emptyContainer->setVisible(true);
        ui->listTransactions->setVisible(false);
    } else {
        showList();
    }

    // Store settings
    QSettings settings;
    settings.setValue("transactionType", filterByType);
}

void DashboardWidget::walletSynced(bool sync)
{
    if (this->isSync != sync) {
        this->isSync = sync;
        ui->layoutWarning->setVisible(!this->isSync);
    }
}

void DashboardWidget::changeTheme(bool isLightTheme, QString& theme)
{
    static_cast<TxViewHolder*>(this->txViewDelegate->getRowFactory())->isLightTheme = isLightTheme;
}

bool DashboardWidget::chatDestinationAddressChanged(const QString& str)
{
    bool shielded = false;
    if (!str.isEmpty()) {
        QString trimmedStr = str.trimmed();
        const bool valid = walletModel->validateAddress(trimmedStr, false, shielded);
        if (!valid) {
            // check URI
            SendCoinsRecipient rcp;
            if (GUIUtil::parseBitcoinURI(trimmedStr, &rcp)) {
                ui->txtChatDestinationAddress->setText(rcp.address);
                QString label = walletModel->getAddressTableModel()->labelForAddress(rcp.address);
                if (!label.isNull() && !label.isEmpty()){
                    ui->txtChatDestinationAddress->setText(label);
                } else if (!rcp.message.isEmpty())
                    ui->txtChatDestinationAddress->setText(rcp.message);

            } else {
                setCssProperty(ui->txtChatDestinationAddress, "chat-address-destination-error");
            }
        } else {
            setCssProperty(ui->txtChatDestinationAddress, "chat-address-destination");
        }
        updateStyle(ui->txtChatDestinationAddress);
        return valid;
    }
    setCssProperty(ui->txtChatDestinationAddress, "chat-address-destination");
    updateStyle(ui->txtChatDestinationAddress);
    return false;
}

int DashboardWidget::setNumBlocks(int blocks) {

    if(!IsBlockchainSynced()) {
        ui->lblBlockHeightValue->setText(QString::number(blocks) + " Blocks -- syncing");
        return 1;
    }
    if(!pwalletMain->IsLocked()) {
        if(walletModel->getOptionsModel()->getCombineUTXO() ) {
            combineUTXOs();
        }
    }
    ui->lblBlockHeightValue->setText(QString::number(blocks) + " Blocks");
    ui->lblAvergaeBlocksValue->setText(QString::number(CMultiTier::getInstance()->GetAverageBlockCount()) + " Blocks");
    ui->lblPoSTier1Value->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetStakingTierLevelCount(1)));
    ui->lblPoSTier2Value->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetStakingTierLevelCount(2)));
    ui->lblPoSTier3Value->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetStakingTierLevelCount(3)));
    ui->lblPoSTier4Value->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetStakingTierLevelCount(4)));
    LOCK(cs_main);
    pcoinsTip->Flush();
    ui->lblMoneySupplyTransparentValue->setText(GUIUtil::formatBalance(pcoinsTip->GetTotalAmount()));
    ui->lblMoneySupplyShieldedValue->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetCurrentSupplyShielded()));
    //ui->lblMoneySupplyTransparentValue->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetCurrentSupplyTransparent()));
    CAmount val = CMultiTier::getInstance()->GetActivePoSPhase(blocks);
    QString strTier1 = GUIUtil::formatBalance(CMultiTier::getInstance()->GetMultiTierValue(val,1)).replace(" PASSION","") + " / ";
    QString strTier2 = GUIUtil::formatBalance(CMultiTier::getInstance()->GetMultiTierValue(val,2)).replace(" PASSION","") + " / ";
    QString strTier3 = GUIUtil::formatBalance(CMultiTier::getInstance()->GetMultiTierValue(val,3)).replace(" PASSION","") + " / ";
    QString strTier4 = GUIUtil::formatBalance(CMultiTier::getInstance()->GetMultiTierValue(val,4));
    ui->lblCurrentRewardsValue->setText(strTier1 + strTier2 + strTier3 + strTier4);
    val = CMultiTier::getInstance()->GetNextPoSPhase(blocks);
    strTier1 = GUIUtil::formatBalance(CMultiTier::getInstance()->GetMultiTierValue(val,1)).replace(" PASSION","") + " / ";
    strTier2 = GUIUtil::formatBalance(CMultiTier::getInstance()->GetMultiTierValue(val,2)).replace(" PASSION","") + " / ";
    strTier3 = GUIUtil::formatBalance(CMultiTier::getInstance()->GetMultiTierValue(val,3)).replace(" PASSION","") + " / ";
    strTier4 = GUIUtil::formatBalance(CMultiTier::getInstance()->GetMultiTierValue(val,4));
    ui->lblUpcommingRewardsValue->setText(strTier1 + strTier2 + strTier3 + strTier4);
    ui->lblPoSStakingValue->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetEligableStakeAmount()));
    ui->lblUserStakeTier1Value->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetUserStakeAmount(1)));
    ui->lblUserStakeTier2Value->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetUserStakeAmount(2)));
    ui->lblUserStakeTier3Value->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetUserStakeAmount(3)));
    ui->lblUserStakeTier4Value->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetUserStakeAmount(4)));
    ui->lblUserStakeCoinsValue->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetUserStakeAmount(0)));
    ui->lblUserStakeRewardsValue->setText(GUIUtil::formatBalance(CMultiTier::getInstance()->GetUserStakeRewards()));
    return 1;
}
void DashboardWidget::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent notifications-spam when initial block download is in progress
    if (!walletModel || !clientModel || clientModel->inInitialBlockDownload())
        return;

    if (!txModel || txModel->processingQueuedTransactions())
        return;

    QString date = txModel->index(start, TransactionTableModel::Date, parent).data().toString();
    qint64 amount = txModel->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = txModel->index(start, TransactionTableModel::Type, parent).data().toString();
    QString address = txModel->index(start, TransactionTableModel::ToAddress, parent).data().toString();

    Q_EMIT incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address);
}

void DashboardWidget::combineUTXOs() {
    CAmount sendValue = 0;
    CCoinControl* coinControl = new CCoinControl();
    walletModel->combineUTXO(sendValue,coinControl);
    if(sendValue > Params().GetConsensus().nMinStakeValue || coinControl->QuantitySelected() > 400) {
        Destination dest;
        walletModel->getNewAddress(dest);
        SendCoinsRecipient sendCoinsRecipient(
                    QString::fromStdString(dest.ToString()),
                    QString::fromStdString(""),
                    sendValue - 1500000,
                    "");
        QList<SendCoinsRecipient> recipients;
        recipients.append(sendCoinsRecipient);
        WalletModelTransaction currentTransaction(recipients);
        WalletModel::SendCoinsReturn prepareStatus;
        prepareStatus = walletModel->prepareTransaction(&currentTransaction, coinControl, false);
        if (prepareStatus.status == WalletModel::OK) {
            walletModel->sendCoins(currentTransaction);
        }
    }
}
DashboardWidget::~DashboardWidget()
{
    delete ui;
}
