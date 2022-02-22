//Copyright (c) 2021 The PASSION CORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/passion/subscription/subscriptionmodel.h"
#include "qt/passion/subscription/subsite.h"
#include "sync.h"
#include "uint256.h"
#include "wallet/wallet.h"
#include "guiutil.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QString>
#include <QMapIterator>
#include <QDateTime>


SubscriptionModel::SubscriptionModel(QObject *parent) : QAbstractTableModel(parent){
    loadSubscriptionSiteList();
}

void SubscriptionModel::loadSubscriptionSiteList(){
    int end = nodes.size();
    nodes.clear();
    collateralTxAccepted.clear();
    beginInsertRows(QModelIndex(), nodes.size(), nodes.size());
    #ifdef _WIN32
        QString path = QString::fromStdWString(GetDataDir().generic_wstring());
    #else
        QString path = QString::fromStdString(GetDataDir().native());
    #endif

    QFile siteFile(path + "/sites.conf");
    if (siteFile.open(QIODevice::ReadOnly))
    {
        //TODO SUBSCRIPTION ERROR HANDLING
       QTextStream in(&siteFile);
       //SubSite *site = nullptr;
       while (!in.atEnd())
       {
          QString line = in.readLine();
          if(!line.startsWith(QLatin1Char('#'))) {
              QStringList list2 = line.split(QLatin1Char(' '));
              SubSite *site = new SubSite();              
              site->setSiteName(list2[0]);
              site->setSiteDomain(list2[1]);
              site->setSiteKey(list2[2]);
              site->setSiteAddr(list2[3]);
              site->setSiteFee(list2[4].toInt());
              site->setSiteAddrRegister(list2[5]);
              site->setSiteAddrRegisterBalance(getAddressBalance(list2[5])); //getAddressBalance(list2[5])
              site->setSiteExpire(list2[6].toInt());
              site->setSiteState(list2[7].toInt());
              if(list2[8].length() > 5)
                site->setSiteLastTransaction(list2[8]);
              nodes.insert(list2[0],site);
          }
       }
       siteFile.close();
    } else {
        LogPrintf("Failed to open file! %s\n", path.toStdString().c_str());
    }
    endInsertRows();
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(end, 9, QModelIndex()) );
}

QString SubscriptionModel::getAddressBalance(QString address) {
    
    std::map<WalletModel::ListCoinsKey, std::vector<WalletModel::ListCoinsValue>> mapCoins;
    CAmount nSum = 0;
    if(wModel) {
        //TODO
        wModel->listCoins(mapCoins);   
        for (const std::pair<WalletModel::ListCoinsKey, std::vector<WalletModel::ListCoinsValue>>& coins : mapCoins) {
            QString sWalletAddress = coins.first.address;            
            if(!QString::compare(sWalletAddress, address, Qt::CaseSensitive)) {
                for(const WalletModel::ListCoinsValue& out: coins.second) {                    
                    nSum  += out.nValue;
                }
            }
        }
    }
    return GUIUtil::formatBalance(nSum);
}
int SubscriptionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return nodes.size();
}

int SubscriptionModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 10;
}

void SubscriptionModel::writeSitesToDisk() {
    // Create a new file
    #ifdef _WIN32
        QString path = QString::fromStdWString(GetDataDir().generic_wstring());
    #else
        QString path = QString::fromStdString(GetDataDir().native());
    #endif
    QFile siteFile(path + "/sites.conf");
    if (siteFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
       QTextStream fout(&siteFile);
       SubSite *site;
       QMapIterator<QString, SubSite*> i(nodes);
       while (i.hasNext()) {
           i.next();
           site = i.value();
           fout << i.key() <<  " " << site->getSiteDomain() <<
                                " " << site->getSiteKey() <<
                                " " << site->getSiteAddr() <<
                                " " << site->getSiteFee() <<
                                " " << site->getSiteAddrRegister() <<
                                " " << site->getSiteExpire() <<
                                " " << site->getSiteState() <<
                                " " << site->getSiteLastTransaction() << '\n';

       }
       siteFile.close();
    } else {
        qDebug("Fehler beim öffnen der Datei");
    }
}


QVariant SubscriptionModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    int row = index.row();
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
            case NAME:
                return nodes.uniqueKeys().value(row);
                //return nodes.values().value(row)->getSiteName();
            case DOMAINS:
                return nodes.values().value(row)->getSiteDomain();
            case KEY:
                return nodes.values().value(row)->getSiteKey();
            case ADDRESS:
                return nodes.values().value(row)->getSiteAddr();
            case REGISTERADDRESS:
                return nodes.values().value(row)->getSiteAddrRegister();
            case REGISTERADDRESSBALANCE:
                return nodes.values().value(row)->getSiteAddrRegisterAmount();
            case SITEFEE:
                return nodes.values().value(row)->getSiteFee();
            case EXPIRE:
                return nodes.values().value(row)->getSiteExpire();
            case STATUS:
                return nodes.values().value(row)->getSiteState();
        }
    }

    return QVariant();
}
bool SubscriptionModel::setData(QModelIndex &modelIndex, const QVariant insert, int role)
{
    int idx = modelIndex.row();
    int timestamp = QDateTime::currentSecsSinceEpoch();
    QString balance = this->getAddressBalance(nodes.values().value(idx)->getSiteAddrRegister());
    nodes.values().value(idx)->setSiteLastTransaction(insert.toString());
    writeSitesToDisk();
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, 9, QModelIndex()) );
}
void SubscriptionModel::setWallet(WalletModel* w) {
    wModel = w;
}
bool SubscriptionModel::removeSite(const QModelIndex& modelIndex) {
    QString alias = modelIndex.data(Qt::DisplayRole).toString();
    int idx = modelIndex.row();
    beginRemoveRows(QModelIndex(), idx, idx);
    nodes.take(alias);
    endRemoveRows();
    writeSitesToDisk();
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, 9, QModelIndex()) );
    return true;
}

bool SubscriptionModel::addSite(const QString domain, const QString key, const QString addr, const QString registerAddr,int sitefee, int expire, int status){
    beginInsertRows(QModelIndex(), nodes.size(), nodes.size());
    QString name = GetRandomString();
    SubSite *site = new SubSite();
    site->setSiteName(name);
    site->setSiteDomain(domain);
    site->setSiteKey(key);
    site->setSiteAddr(addr);
    site->setSiteAddrRegister(registerAddr);
    site->setSiteFee(sitefee);
    site->setSiteAddrRegisterBalance(getAddressBalance(registerAddr)); //getAddressBalance
    site->setSiteExpire(expire);
    site->setSiteState(status);
    nodes.insert(name,site);
    endInsertRows();
    writeSitesToDisk();
    return true;
}
void SubscriptionModel::updateSubscriptionSiteList(int blockheight) {
    int end = nodes.size();
    QList<SubSite*> sites = nodes.values();
    SubSite* item;
    CCoinsViewCache view(pcoinsTip);
    bool changed = false;
    Q_FOREACH( item , sites )
    { 
        QString balance = this->getAddressBalance(item->getSiteAddrRegister());
        item->setSiteAddrRegisterBalance(balance);
        QStringList split = item->getSiteLastTransaction().split(":");
        unsigned int _index = 0;
        uint256 hash;
        if(split.count() > 1) {
            hash = uint256S(split.at(0).toStdString());
            _index = split.at(1).toUInt();
        }

        int timestamp = QDateTime::currentSecsSinceEpoch();
        if(timestamp > item->getSiteExpire() && _index == 1) {
            item->setSiteState(9);
        } else {
            CBlockIndex* blockindex = nullptr;
            CTransactionRef tx;
            uint256 hash_block;
            GetTransaction(hash, tx, hash_block, true, blockindex);           
            BlockMap::iterator mi = mapBlockIndex.find(hash_block);
            int age = 0;
            int time = 0;
            if (mi != mapBlockIndex.end() && (*mi).second) {
                CBlockIndex* pindex = (*mi).second;
                if (chainActive.Contains(pindex)) {
                
                    time = (int) pindex->GetBlockTime();
                    age = (1 + chainActive.Height() - pindex->nHeight);
                }
            }
            if(age > 6) {
                item->setSiteExpire(time + 3600 + 400 );
                item->setSiteLastTransaction(split.at(0)+":1");
                changed = true;
                item->setSiteState(8);
            } else {
                item->setSiteState(age);
            }        
        }
    }
    if(changed) {
        writeSitesToDisk();
    }
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(end, 9, QModelIndex()) );
}

QString SubscriptionModel::GetRandomString()
{
   const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
   const int randomStringLength = 12; // assuming you want random strings of 12 characters

   QString randomString;
   for(int i=0; i<randomStringLength; ++i)
   {
       int index = qrand() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
   }
   return randomString;
}
