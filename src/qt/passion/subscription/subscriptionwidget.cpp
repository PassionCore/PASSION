//Copyright (c) 2021 The PASSION CORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/passion/subscription/subscriptionwidget.h"
#include "qt/passion/subscription/ui_subscriptionwidget.h"
#include "qt/passion/qtutils.h"
#include "qt/passion/guitransactionsutils.h"
#include "qt/passion/subscription/subrow.h"
#include "qt/passion/subscription/subsite.h"
#include "qt/passion/subscription/subscriptioninfodialog.h"
#include "qt/passion/guitransactionsutils.h"
#include "qt/passion/optionbutton.h"
#include "qt/passion/sendconfirmdialog.h"
#include "messagesigner.h"
#include "clientmodel.h"
#include "guiutil.h"
#include "init.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "sync.h"
#include "wallet/wallet.h"
#include "walletmodel.h"
#include "askpassphrasedialog.h"
#include "messagesigner.h"
#include "util.h"
#include "base58.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QUrl>
#include <QtCore>
#include <QDesktopServices>
#include <QCryptographicHash>

#define DECORATION_SIZE 65
#define NUM_ITEMS 3

class SubHolder : public FurListRow<QWidget*>
{
public:
    SubHolder();

    explicit SubHolder(bool _isLightTheme) : FurListRow(), isLightTheme(_isLightTheme){}

    SubRow* createHolder(int pos) override{
        if(!cachedRow) cachedRow = new SubRow();
        return cachedRow;
    }

    void init(QWidget* holder,const QModelIndex &index, bool isHovered, bool isSelected) const override{
        SubRow* row = static_cast<SubRow*>(holder);
        QString domain = index.sibling(index.row(), SubscriptionModel::DOMAINS).data(Qt::DisplayRole).toString();
        QString key = index.sibling(index.row(), SubscriptionModel::KEY).data(Qt::DisplayRole).toString();
        QString paymentaddress = index.sibling(index.row(), SubscriptionModel::ADDRESS).data(Qt::DisplayRole).toString();
        QString expire = index.sibling(index.row(), SubscriptionModel::EXPIRE).data(Qt::DisplayRole).toString();
        int status = index.sibling(index.row(), SubscriptionModel::STATUS).data(Qt::DisplayRole).toInt();
        QString sitefee = index.sibling(index.row(), SubscriptionModel::SITEFEE).data(Qt::DisplayRole).toString();
        QString registeraddress = index.sibling(index.row(), SubscriptionModel::REGISTERADDRESS).data(Qt::DisplayRole).toString();
        QString registeraddressbalance = index.sibling(index.row(), SubscriptionModel::REGISTERADDRESSBALANCE).data(Qt::DisplayRole).toString();
        row->updateView(domain, key, paymentaddress, sitefee,registeraddress,registeraddressbalance, expire, status);
    }

    QColor rectColor(bool isHovered, bool isSelected) override{
        return getRowColor(isLightTheme, isHovered, isSelected);
    }

    ~SubHolder() override{}

    bool isLightTheme;
    SubRow* cachedRow = nullptr;
};

SubscriptionWidget::SubscriptionWidget(PassionGUI *parent) :
    PWidget(parent),
    ui(new Ui::SubscriptionWidget)
{
    ui->setupUi(this);
    delegate = new FurAbstractListItemDelegate(
            DECORATION_SIZE,
            new SubHolder(isLightTheme()),
            this
    );
    subscriptionModel = new SubscriptionModel(this);
    coinControl = new CCoinControl();
    this->setStyleSheet(parent->styleSheet());
    /* Containers */
    setCssProperty(ui->left, "container");
    ui->left->setContentsMargins(0,20,0,20);

    /* Light Font */
    QFont fontLight;
    fontLight.setWeight(QFont::Light);

    /* Title */
    ui->labelTitle->setText(tr("Website Subscription System"));
    setCssTitleScreen(ui->labelTitle);
    ui->labelTitle->setFont(fontLight);

    ui->labelSubtitle1->setText(tr("PASSION provides a one-click subscription system where you can register and pay with one click."));
    setCssSubtitleScreen(ui->labelSubtitle1);

    setCssProperty(ui->lblDomain, "subscription-lbl-domain");
    initCssEditLine(ui->txtDomainInput);
    setCssBtnSecondary(ui->btnPay);
    setCssBtnSecondary(ui->btnRegister);
    ui->btnPay->setVisible(false);
    setCssProperty(ui->listSites, "container");
    ui->listSites->setItemDelegate(delegate);
    ui->listSites->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listSites->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listSites->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listSites->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->btnRegister, SIGNAL(clicked()), this, SLOT(onRegisterClicked()));
    connect(ui->btnPay, SIGNAL(clicked()), this, SLOT(onPayClicked()));
    connect(ui->listSites, SIGNAL(clicked(QModelIndex)), this, SLOT(onMNClicked(QModelIndex)));
}

void SubscriptionWidget::loadWalletModel(){
    if(walletModel) {
        ui->listSites->setModel(subscriptionModel);
        ui->listSites->setModelColumn(0);
        subscriptionModel->setWallet(this->walletModel);
        setNumBlocks(clientModel->getNumBlocks());
        connect(clientModel, &ClientModel::numBlocksChanged, this, &SubscriptionWidget::setNumBlocks);
        updateListState();
    }
}
int SubscriptionWidget::setNumBlocks(int blocks) {
    //std::cout << "Update Subscription Data at blockheight: " << blocks << std::endl;
    subscriptionModel->updateSubscriptionSiteList(blocks);
    return 1;
}

void SubscriptionWidget::updateListState() {
    if (subscriptionModel->rowCount() > 0) {
        ui->listSites->setVisible(true);
    } else {
        ui->listSites->setVisible(false);
    }
}

void SubscriptionWidget::onMNClicked(const QModelIndex &index){
    ui->listSites->setCurrentIndex(index);
    QRect rect = ui->listSites->visualRect(index);
    QPoint pos = rect.topRight();
    pos.setX(pos.x() - (DECORATION_SIZE * 5));
    pos.setY(pos.y() + (DECORATION_SIZE * 2));
    if(!this->menu){
        this->menu = new SubscriptionTipMenu(window, this);
        this->menu->setVisitBtnText(tr("Visit"));
        this->menu->setCopyBtnText(tr("Copy"));
        this->menu->setFundBtnText(tr("Fund"));
        this->menu->setPayBtnText(tr("Pay"));
        this->menu->setInfoBtnText(tr("Info"));
        this->menu->setTrashBtnText(tr("Delete"));
        connect(this->menu, &TooltipMenu::message, this, &AddressesWidget::message);
        connect(this->menu, SIGNAL(onVisitClicked()), this, SLOT(onVisitMNClicked()));
        connect(this->menu, SIGNAL(onFundClicked()), this, SLOT(onFundMNClicked()));
        connect(this->menu, SIGNAL(onCopyClicked()), this, SLOT(onCopyMNClicked()));
        connect(this->menu, SIGNAL(onPayClicked()), this, SLOT(onPayMNClicked()));
        connect(this->menu, SIGNAL(onInfoClicked()), this, SLOT(onInfoMNClicked()));
        connect(this->menu, SIGNAL(onTrashClicked()), this, SLOT(onDeleteMNClicked()));
        this->menu->adjustSize();
    }else {
        this->menu->hide();
    }
    this->index = index;
    menu->move(pos);
    menu->show();

    // Back to regular status
    ui->listSites->scrollTo(index);
    ui->listSites->clearSelection();
    ui->listSites->setFocus();
}

void SubscriptionWidget::onVisitMNClicked(){
    QString domain = index.sibling(this->index.row(), SubscriptionModel::DOMAINS).data(Qt::DisplayRole).toString();
    QString key = index.sibling(this->index.row(), SubscriptionModel::KEY).data(Qt::DisplayRole).toString();
    QString sitehash = QCryptographicHash::hash(domain.toUtf8(),QCryptographicHash::Sha256).toHex();
    inform("Site will be opend in the browser");
    QDesktopServices::openUrl(QUrl((domain+"?key=" + key + "&sitehash=" + sitehash), QUrl::TolerantMode));
}
void SubscriptionWidget::onFundMNClicked(){
    QString registerAddress = index.sibling(index.row(), SubscriptionModel::REGISTERADDRESS).data(Qt::DisplayRole).toString();
    CAmount nSiteFee = index.sibling(index.row(), SubscriptionModel::SITEFEE).data(Qt::DisplayRole).toLongLong() * COIN;
    // const QString& addr, const QString& label, const CAmount& amount, const QString& message
    SendCoinsRecipient sendCoinsRecipient(registerAddress, "Funding ->" + registerAddress, nSiteFee, "");
    // Send the 10 tx to one of your address
    QList<SendCoinsRecipient> recipients;
    recipients.append(sendCoinsRecipient);
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;
    prepareStatus = walletModel->prepareTransaction(&currentTransaction, nullptr, false);
    QString returnMsg = tr("Unknown error");
    // process prepareStatus and on error generate message shown to user
    CClientUIInterface::MessageBoxFlags informType;
    returnMsg = GuiTransactionsUtils::ProcessSendCoinsReturn(
                this,
                prepareStatus,
                walletModel,
                informType, // this flag is not needed
                BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(),
                                             currentTransaction.getTransactionFee()),
                true
        );
    if (prepareStatus.status == WalletModel::OK) {
        walletModel->sendCoins(currentTransaction);
    }
    
    // process sendStatus and on error generate message shown to user
    WalletModel::SendCoinsReturn sendStatus;
    // process sendStatus and on error generate message shown to user
    GuiTransactionsUtils::ProcessSendCoinsReturnAndInform(
        this,
        sendStatus,
        walletModel
    );
    if (sendStatus.status == WalletModel::OK) {
        inform("Internal transaction to " + registerAddress + " successfully sent");
        //subscriptionModel->setData(index, QVariant(), Qt::DisplayRole);
    } else {
        inform("Error occurred the addres... " + returnMsg);        
    }
}
void SubscriptionWidget::onRegisterClicked() {
    RegisterToSite();
}
void SubscriptionWidget::onPayClicked() {    
    //int nDisplayUnit = this->walletModel->getOptionsModel()->getDisplayUnit();
    QString destinationAddress = index.sibling(index.row(), SubscriptionModel::ADDRESS).data(Qt::DisplayRole).toString();
    QString registerAddress = index.sibling(index.row(), SubscriptionModel::REGISTERADDRESS).data(Qt::DisplayRole).toString();
    QString domain = "WSS: " + index.sibling(index.row(), SubscriptionModel::DOMAINS).data(Qt::DisplayRole).toString();
    CAmount nSiteFee = index.sibling(index.row(), SubscriptionModel::SITEFEE).data(Qt::DisplayRole).toLongLong() * COIN;
    CCoinControl *cControl = new CCoinControl();
    std::map<WalletModel::ListCoinsKey, std::vector<WalletModel::ListCoinsValue>> mapCoins;
    if(this->walletModel) {
        CAmount nSum = 0;
        //TODO LOCK WALLET MODELL
        this->walletModel->listCoins(mapCoins);   
        for (const std::pair<WalletModel::ListCoinsKey, std::vector<WalletModel::ListCoinsValue>>& coins : mapCoins) {
            QString sWalletAddress = coins.first.address;            
            if(!QString::compare(sWalletAddress, registerAddress, Qt::CaseSensitive)) {
                for(const WalletModel::ListCoinsValue& out: coins.second) {                    
                    nSum  += out.nValue;
                    BaseOutPoint p(out.txhash,out.outIndex);
                    cControl->Select(p,out.nValue);
                }
            }
        }

        // It is necessary that we can have at least on input from the registered address otherwise it could not be detected that you are wright one
        if(!nSum) {
            inform("No funds at registered address. Do internal funding first");
            return;
        } else if(nSum <= nSiteFee) {            
            for (const std::pair<WalletModel::ListCoinsKey, std::vector<WalletModel::ListCoinsValue>>& coins : mapCoins) {
                for(const WalletModel::ListCoinsValue& out: coins.second) {
                    nSum  += out.nValue;
                    BaseOutPoint p(out.txhash,out.outIndex);
                    cControl->Select(p,out.nValue);           
                }
                if(nSum > nSiteFee) break;
            }
        }
        std::cout << "sum: " << nSum << std::endl;
    }
    
    cControl->destChange = DecodeDestination(registerAddress.toStdString());
    // const QString& addr, const QString& label, const CAmount& amount, const QString& message
    SendCoinsRecipient sendCoinsRecipient(destinationAddress, domain, nSiteFee, "");
    std::cout << "Dest: " << destinationAddress.toStdString() << std::endl;
    std::cout << "Dom: " << domain.toStdString() << std::endl;
    std::cout << "Fee: " << nSiteFee << std::endl;
    QList<SendCoinsRecipient> recipients;
    recipients.append(sendCoinsRecipient);
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;
    prepareStatus = walletModel->prepareTransaction(&currentTransaction, cControl, false);
    QString returnMsg = tr("Unknown error");
    // process prepareStatus and on error generate message shown to user
    CClientUIInterface::MessageBoxFlags informType;
    returnMsg = GuiTransactionsUtils::ProcessSendCoinsReturn(
                this,
                prepareStatus,
                walletModel,
                informType, // this flag is not needed
                BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(),currentTransaction.getTransactionFee()),
                true
        );
    if (prepareStatus.status == WalletModel::OK) {
        walletModel->sendCoins(currentTransaction);
    }    
    // process sendStatus and on error generate message shown to user
    WalletModel::SendCoinsReturn sendStatus;
    // process sendStatus and on error generate message shown to user
    GuiTransactionsUtils::ProcessSendCoinsReturnAndInform(
        this,
        sendStatus,
        walletModel
    );
    if (sendStatus.status == WalletModel::OK) {
        // look for the tx index of the collateral
        CTransactionRef walletTx = currentTransaction.getTransaction();
        QString tx = "";
        if(walletTx) {
        std::string txID = walletTx->GetHash().GetHex();
         tx = QString::fromStdString(txID) + ":0"; // + QString::number(indexOut);
        }
        inform("Payment transaction sent. Wait for confirmation to get access");
        std::cout << "after inform" << std::endl;
        subscriptionModel->setData(index, QVariant(tx), Qt::DisplayRole);
    } else {
        inform("Error sending...");        
    }
    std::cout << "before delete Select" << std::endl;
    delete(cControl);
}

void SubscriptionWidget::RegisterToSite() {
    
    QString val = walletModel->getOptionsModel()->getSubscriptionAutofundingValue();
    QString strHost = walletModel->getOptionsModel()->getSubscriptionServiceIP();
    QUrl url(strHost + "getid.php");
    QNetworkRequest request(url);
    QNetworkAccessManager nam;
    QNetworkReply *reply = nam.get(request);
    while (!reply->isFinished())
    {
        qApp->processEvents();
    }
    QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();
    QJsonObject obj = json.object();
    int expire = obj["expire"].toInt(); // QString::number(expire)
    int time = obj["time"].toInt(); // QString::number(time)
    QString messageToSign = obj["hash"].toString();
    QString siteurl = ui->txtDomainInput->text();

    QString registerAddr = getNewAddress();
    QString signature = signRegisterMessage(registerAddr,messageToSign);
    QString sitehash = QCryptographicHash::hash(siteurl.toUtf8(),QCryptographicHash::Sha256).toHex();

    QUrl serviceUrl = QUrl(strHost + "register2.php");
    QByteArray postData;
    QUrlQuery query;
    query.addQueryItem("addr",registerAddr);
    query.addQueryItem("hash",messageToSign);
    query.addQueryItem("url",siteurl);
    query.addQueryItem("sig",signature);
    query.addQueryItem("sitehash", sitehash);
    query.addQueryItem("submit","true");

    postData = query.toString(QUrl::FullyEncoded).toUtf8();

    // Call the webservice
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QNetworkRequest networkRequest(serviceUrl);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
    QNetworkReply *reply2 = networkManager->post(networkRequest,postData);
    while (!reply2->isFinished())
    {
        qApp->processEvents();
    }
    QByteArray response_data2 = reply2->readAll();
    QJsonDocument json2 = QJsonDocument::fromJson(response_data2);
    reply->deleteLater();
    QString strJson(json2.toJson(QJsonDocument::Compact));
    QJsonObject obj2 = json2.object();
    if(obj2["status"] == 0) {
        inform("ERROR " +obj2["message"].toString());
    } else {
        QString domain = siteurl;
        QString key = obj2["key"].toString();
        QString paymentaddress = obj2["siteaddr"].toString();
        int sitefee = obj2["sitefee"].toInt();
        int expire = obj["time"].toInt()*0;
        int status = obj2["status"].toInt();
        subscriptionModel->addSite(domain,key,paymentaddress,registerAddr,sitefee,expire,0);
        if(!pwalletMain->IsLocked()) {
            if(walletModel->getOptionsModel()->getSubscriptionAutofunding()) {
                inform("Site successfully registered, now fund address");
                subscriptionModel->setData(index, QVariant(), Qt::DisplayRole);
                sendCoinsto(val,registerAddr);
            }
        }
        inform("Site successfully registered");
        updateListState();
    }
}
void SubscriptionWidget::sendCoinsto(QString amount, QString addressto, QString addressfrom, int type) {
    
    QString registerAddress = addressto;
    CAmount nSiteFee =  amount.toInt() * COIN;
    // const QString& addr, const QString& label, const CAmount& amount, const QString& message
    SendCoinsRecipient sendCoinsRecipient(registerAddress, "Funding ->" + registerAddress, nSiteFee, "");
    // Send the 10 tx to one of your address
    QList<SendCoinsRecipient> recipients;
    recipients.append(sendCoinsRecipient);
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;
    prepareStatus = walletModel->prepareTransaction(&currentTransaction, nullptr, false);
    QString returnMsg = tr("Unknown error");
    // process prepareStatus and on error generate message shown to user
    CClientUIInterface::MessageBoxFlags informType;
    returnMsg = GuiTransactionsUtils::ProcessSendCoinsReturn(
                this,
                prepareStatus,
                walletModel,
                informType, // this flag is not needed
                BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(),
                                             currentTransaction.getTransactionFee()),
                true
        );
    if (prepareStatus.status == WalletModel::OK) {
        walletModel->sendCoins(currentTransaction);
    }    
    // process sendStatus and on error generate message shown to user
    WalletModel::SendCoinsReturn sendStatus;
    // process sendStatus and on error generate message shown to user
    GuiTransactionsUtils::ProcessSendCoinsReturnAndInform(
        this,
        sendStatus,
        walletModel
    );
    if (sendStatus.status == WalletModel::OK) {
        inform("Internal transaction to " + registerAddress + " successfully sent");
    } else {
        inform("Error occurred the addres... " + returnMsg);        
    }
}
QString SubscriptionWidget::getNewAddress() {
    try {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if (!ctx.isValid()) {
            return nullptr;
        }   
        Destination address;
        PairResult r = walletModel->getNewAddress(address, "");
        // Check for validity
        if(!r.result) {
            inform(r.status->c_str());
            return nullptr;
        }
        return QString::fromStdString(address.ToString());
    } catch (const std::runtime_error& error){
        // Error generating address
        inform("Error generating address");
        return nullptr;
    }
}
QString SubscriptionWidget::signRegisterMessage(const QString& signAddress, const QString message) {
    
    if (!walletModel) {
        inform(tr("Error: No Wallet Model"));
        return nullptr;
    }
    CTxDestination dest = DecodeDestination(signAddress.toStdString());
    CKeyID keyID;
    if (!walletModel->getKeyId(dest,keyID)) {
        return nullptr;
    }
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid()) {
        return nullptr;
    }
    CKey key;
    if (!pwalletMain->GetKey(keyID, key)) {
        return nullptr;
    }

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << message.toStdString(); //obj["hash"].toString().toStdString();

    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(Hash(ss.begin(), ss.end()), vchSig)) {
        return nullptr;
    }
    return QString::fromStdString(EncodeBase64(&vchSig[0], vchSig.size()));
}

void SubscriptionWidget::onInfoMNClicked(){
    
    showHideOp(true);
    SubscriptionInfoDialog* dialog = new SubscriptionInfoDialog(window);
    
    dialog->setData("pubKey", "label", "address", "txId", "outIndex", "status");
    dialog->adjustSize();
    openDialogWithOpaqueBackgroundY(dialog, window);
    dialog->deleteLater();

    QString destinationAddress = index.sibling(index.row(), SubscriptionModel::ADDRESS).data(Qt::DisplayRole).toString();
    QString registerAddress = index.sibling(index.row(), SubscriptionModel::REGISTERADDRESS).data(Qt::DisplayRole).toString();
    CAmount nSiteFee = index.sibling(index.row(), SubscriptionModel::SITEFEE).data(Qt::DisplayRole).toLongLong() * COIN;
    inform("dest: " + destinationAddress + " : " + registerAddress + " : " + QString::number(nSiteFee/COIN));
}

void SubscriptionWidget::onDeleteMNClicked(){
    subscriptionModel->removeSite(index);
    inform(tr("Site successfully removed"));
}

void SubscriptionWidget::onPayMNClicked(){
    onPayClicked();
}
void SubscriptionWidget::onCopyMNClicked(){
    QString domain = index.sibling(this->index.row(), SubscriptionModel::DOMAINS).data(Qt::DisplayRole).toString();
    QString key = index.sibling(this->index.row(), SubscriptionModel::KEY).data(Qt::DisplayRole).toString();
    QString sitehash = QCryptographicHash::hash(domain.toUtf8(),QCryptographicHash::Sha256).toHex();
    QUrl addr = QUrl((domain+"?key=" + key + "&sitehash=" + sitehash), QUrl::TolerantMode);
    GUIUtil::setClipboard(addr.toString());
    inform(tr("URL copied to clipboard"));
}

void SubscriptionWidget::changeTheme(bool isLightTheme, QString& theme){
    static_cast<SubHolder*>(this->delegate->getRowFactory())->isLightTheme = isLightTheme;
}

SubscriptionWidget::~SubscriptionWidget()
{
    delete ui;
}
