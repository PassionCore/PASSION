#include "multitier.h"
#include "init.h"
#include "chain.h"
#include "validation.h"
#include "validationinterface.h"
#include "utilstrencodings.h"
#include "sync.h"
#include "txdb.h"
#include "wallet/wallet.h"
#include <openssl/aes.h>
#include <boost/thread.hpp>

CMultiTier* CMultiTier::instance = NULL;
Mutex m;
const int MULTI_TIER_LEVEL_COUNT = 4;
const int MULTI_TIER_MIN_CONFIRMATION = 5;

enum MultiTier : CAmount {
    TIER1 = 5000*COIN,
    TIER2 = 10000*COIN,
    TIER3 = 20000*COIN,
    TIER4 = 50000*COIN
};

CAmount CMultiTier::GetMultiTierLevelAmount(int level) {
    switch(level) {
        case 1: return MultiTier::TIER1;
        case 2: return MultiTier::TIER2;
        case 3: return MultiTier::TIER3;
        case 4: return MultiTier::TIER4;
    }
    return 0;
}

bool CMultiTier::IsMultiTierOutput(CAmount value) {
    switch(value) {
        case MultiTier::TIER1:
        case MultiTier::TIER2:
        case MultiTier::TIER3:
        case MultiTier::TIER4:
            return true;
        default: return false;
    }
    return false;
}
CAmount CMultiTier::GetActivePoSPhase(int nHeight) {
    if(nHeight <= 100000) {
        return GetBlockValue(100000);
    } else if(nHeight <= POSPHASE::PHASE0) {
        return GetBlockValue(POSPHASE::PHASE0);
    } else if(nHeight <= POSPHASE::PHASE1) {
        return GetBlockValue(POSPHASE::PHASE1);
    } else if(nHeight <= POSPHASE::PHASE1) {
        return GetBlockValue(POSPHASE::PHASE1);
    } else if(nHeight <= POSPHASE::PHASE2) {
        return GetBlockValue(POSPHASE::PHASE2);
    } else if(nHeight <= POSPHASE::PHASE3) {
        return GetBlockValue(POSPHASE::PHASE3);
    } else if(nHeight <= POSPHASE::PHASE4) {
        return GetBlockValue(POSPHASE::PHASE4);
    } else if(nHeight <= POSPHASE::PHASE5) {
        return GetBlockValue(POSPHASE::PHASE5);
    } else if(nHeight <= POSPHASE::PHASE6) {
        return GetBlockValue(POSPHASE::PHASE6);
    } else if(nHeight <= POSPHASE::PHASE7) {
        return GetBlockValue(POSPHASE::PHASE7);
    } else if(nHeight <= POSPHASE::PHASE8) {
        return GetBlockValue(POSPHASE::PHASE8);
    } else if(nHeight <= POSPHASE::PHASE9) {
        return GetBlockValue(POSPHASE::PHASE9);
    } else if(nHeight <= POSPHASE::PHASE10) {
        return GetBlockValue(POSPHASE::PHASE10);
    } else if(nHeight <= POSPHASE::PHASE11) {
        return GetBlockValue(POSPHASE::PHASE11);
    } else if(nHeight <= POSPHASE::PHASE12) {
        return GetBlockValue(POSPHASE::PHASE12);
    } else if(nHeight <= POSPHASE::PHASE13) {
        return GetBlockValue(POSPHASE::PHASE13);
    } else if(nHeight <= POSPHASE::PHASE14) {
        return GetBlockValue(POSPHASE::PHASE14);
    } else {
        return GetBlockValue(POSPHASE::PHASE14+1);
    }
    return 0;
}
CAmount CMultiTier::GetNextPoSPhase(int nHeight) {
    if(nHeight <= 100000) {
        return GetBlockValue(POSPHASE::PHASE0);
    } else if(nHeight <= POSPHASE::PHASE0) {
        return GetBlockValue(POSPHASE::PHASE1);
    } else if(nHeight <= POSPHASE::PHASE1) {
        return GetBlockValue(POSPHASE::PHASE2);
    } else if(nHeight <= POSPHASE::PHASE1) {
        return GetBlockValue(POSPHASE::PHASE2);
    } else if(nHeight <= POSPHASE::PHASE2) {
        return GetBlockValue(POSPHASE::PHASE3);
    } else if(nHeight <= POSPHASE::PHASE3) {
        return GetBlockValue(POSPHASE::PHASE4);
    } else if(nHeight <= POSPHASE::PHASE4) {
        return GetBlockValue(POSPHASE::PHASE5);
    } else if(nHeight <= POSPHASE::PHASE5) {
        return GetBlockValue(POSPHASE::PHASE6);
    } else if(nHeight <= POSPHASE::PHASE6) {
        return GetBlockValue(POSPHASE::PHASE7);
    } else if(nHeight <= POSPHASE::PHASE7) {
        return GetBlockValue(POSPHASE::PHASE8);
    } else if(nHeight <= POSPHASE::PHASE8) {
        return GetBlockValue(POSPHASE::PHASE9);
    } else if(nHeight <= POSPHASE::PHASE9) {
        return GetBlockValue(POSPHASE::PHASE10);
    } else if(nHeight <= POSPHASE::PHASE10) {
        return GetBlockValue(POSPHASE::PHASE11);
    } else if(nHeight <= POSPHASE::PHASE11) {
        return GetBlockValue(POSPHASE::PHASE12);
    } else if(nHeight <= POSPHASE::PHASE12) {
        return GetBlockValue(POSPHASE::PHASE13);
    } else if(nHeight <= POSPHASE::PHASE13) {
        return GetBlockValue(POSPHASE::PHASE14);
    } else if(nHeight <= POSPHASE::PHASE14) {
        return GetBlockValue(POSPHASE::PHASE14+1);
    } else {
        return 0;
    }
}


CAmount CMultiTier::GetMultiTierValue(CAmount blockValue, int level) {
    switch(level) {
        case 1:
            return 0.105 * blockValue;
            break;
        case 2:
            return 0.155 * blockValue;
            break;
        case 3:
            return 0.22 * blockValue;
            break;
        case 4:
            return 0.42 * blockValue;
            break;
        default:
            return 0;
    }
    return 0;
}

void CMultiTier::PrintMapMultiStaking(const CBlockIndex* const pindexPrev, const CCoinsViewCache &view) {
    std::cout << "MAP START: Size:" << mapMultiTierStaking.size() << std::endl;
    std::cout << "------------------------------------------------------------" << std::endl;
    m.lock();
    for(const auto& out : mapMultiTierStaking ) {
        Coin k;
        view.GetCoin(out.second,k);
        int age = pindexPrev->nHeight - k.nHeight;
        std::cout << "Hash: " << out.first.ToString().c_str() << " Value: " << k.out.nValue << " Age: " << age << std::endl;
    }
    m.unlock();
    std::cout << "MAP END:------------------------------------------------------------" << std::endl;
}

void CMultiTier::AddEntryMultiTierStaking(const COutPoint &out) {
    m.lock();
    mapMultiTierStaking[out.GetHash()] = out;
    std::cout << "Coin added or updated. Size of map " << mapMultiTierStaking.size() << std::endl;
    if(STAKE_AUTO_LOCK_UTXO  && pwalletMain) {
        LOCK(pwalletMain->cs_wallet);
        pwalletMain->LockCoin(out);
    }
    m.unlock();
}

void CMultiTier::RemoveEntryMultiTierStaking(const COutPoint &out) {
    m.lock();
    size_t cnt = mapMultiTierStaking.erase(out.GetHash());
    if(cnt){
        std::cout << cnt << " Coin(s) removed. Size of map " << mapMultiTierStaking.size() << std::endl;
        if(pwalletMain) {
            LOCK(pwalletMain->cs_wallet);
            pwalletMain->UnlockCoin(out);
        }
    }
    m.unlock();
}
bool CMultiTier::FillTransactionMultiTier(CMutableTransaction& txNew, CAmount& blockValue, const CBlockIndex* const pindexPrev) {
    int nMultiTierCount = 0;
    CAmount nTotalMultiTierRewards = 0;
    uint256 prevBlockHash = pindexPrev->GetBlockHash();
    LOCK(cs_main);
    CCoinsViewCache view(pcoinsTip);
    std::cout << "FillBlock at height " << pindexPrev->nHeight+1 << " with hash: " << prevBlockHash.ToString() << std::endl;
    //PrintMapMultiStaking(pindexPrev,view);
    m.lock();
    for(int i = 1; i <= MULTI_TIER_LEVEL_COUNT; i++ ) {
        CScript destination;
        uint256 hashLevel = 0;
        
        for(const auto& out : mapMultiTierStaking ) {
            Coin k;
            if(!view.GetCoin(out.second,k)) {
                continue;
            }
            int age = pindexPrev->nHeight - k.nHeight;
            if(k.out.nValue == GetMultiTierLevelAmount(i) && age > MULTI_TIER_MIN_CONFIRMATION) {
                uint256 hash = SerializeHash((out.second.GetHash()^prevBlockHash));
                if(hashLevel != 0) {
                    if(hash < hashLevel) {
                        hashLevel = hash;
                        destination = k.out.scriptPubKey;
                    }
                } else {
                    hashLevel = hash;
                    destination = k.out.scriptPubKey;
                }
            }
        }
        if(hashLevel > 0) {
            CAmount payment = GetMultiTierValue(blockValue,i);
            txNew.vout.emplace_back(payment,destination);
            nMultiTierCount++;
            nTotalMultiTierRewards += payment;
        }
    }
    m.unlock();
    unsigned int i = txNew.vout.size();
    if (i == (nMultiTierCount + 2)) {
            // Majority of cases; do it quick and move on
        txNew.vout[i - (nMultiTierCount + 1)].nValue -= nTotalMultiTierRewards;
    } else if (i > (nMultiTierCount + 2)) {
        // special case, stake is split between (i-1) outputs
        unsigned int outputs = i-(nMultiTierCount + 1);
        CAmount stakeSplit = (nTotalMultiTierRewards) / outputs;
        CAmount residue = (nTotalMultiTierRewards) - (stakeSplit * outputs);
        for (unsigned int j=1; j<=outputs; j++) {
            txNew.vout[j].nValue -= stakeSplit;
        }
        // in case it's not an even division, take the last bit of dust from the last one
        txNew.vout[outputs].nValue -= residue;
    }
    return true;
}

void CMultiTier::CheckBlockTransactionsForSpendOutputs(const CBlock& block) {
    m.lock();
    for(int i = 0; i < block.vtx.size(); i++) {
        const CTransaction& txNew = *(block.vtx[i]);
        for(const auto& in : txNew.vin) {
            uint256 out = in.prevout.GetHash();
            if(mapMultiTierStaking.find(out) != mapMultiTierStaking.end()) {
                mapMultiTierStaking.erase(out);
                std::cout << "TX Output used for multi staking was spend, remove it from map at block "<< HexStr(block.GetHash()) << std::endl;
            }
        }
    }
    m.unlock();
}
void CMultiTier::CheckBlockTransactionsForIncommingOutputs(const CBlock& block) {
    for(int i = 0; i < block.vtx.size(); i++) {
        const CTransaction& txNew = *(block.vtx[i]);
        uint256 hash = txNew.GetHash();
        for(int j = 0; j < txNew.vout.size(); j++) {
            COutPoint outp{hash,(unsigned int)j};
            CAmount val = txNew.vout[j].nValue;
            if(IsMultiTierOutput(val)) {
                AddEntryMultiTierStaking(outp);
                std::cout << "New TX out for multi tier staking found, add/update to map at block " << HexStr(block.GetHash()) << std::endl;
            }
        }
    }
}
bool CMultiTier::CheckTransactionMultiTier( int nHeight, const CBlock& block, CAmount blockValue, const CBlockIndex* const pindexPrev, const CCoinsViewCache &view) {
    const CTransaction& txNew = *(block.vtx[1]);
    CScript destination;
    int found = 0;
    uint256 prevBlockHash = pindexPrev->pprev->GetBlockHash();
    for(int i = 1; i <= MULTI_TIER_LEVEL_COUNT; i++ ) {
        uint256 hashLevel = 0;
        m.lock();
        for(const auto& out : mapMultiTierStaking ) {
            Coin k;
            if(!view.GetCoin(out.second,k)) {
                continue;
            }
            int age = pindexPrev->pprev->nHeight - k.nHeight;
            if(k.out.nValue == GetMultiTierLevelAmount(i) && age > MULTI_TIER_MIN_CONFIRMATION) {
                uint256 hash = SerializeHash((out.second.GetHash()^prevBlockHash));
                if(hashLevel != 0) {
                    if(hash < hashLevel) {
                        hashLevel = hash;
                        destination = k.out.scriptPubKey;
                    }
                } else {
                    hashLevel = hash;
                    destination = k.out.scriptPubKey;
                }
            }
        }
        m.unlock();
        if(hashLevel > 0) {
            bool found = false;
            CAmount payment = GetMultiTierValue(blockValue,i);
            for(const auto & out : txNew.vout) {
                if(out.scriptPubKey == destination && out.nValue >= payment) {
                    //std::cout << "Tier -> " << i << " payment found" << std::endl;
                    found = true;
                    break;
                }
            }
            if(!found) {
                //std::cout << "Required payment to: " << HexStr(destination).c_str() << " at tier " << i << " was missing." << std::endl;
                return false;
            }
        }
    }
    return true;
}

bool CMultiTier::ConnectBlock(const CBlock& block, const CCoinsViewCache &view, const CBlockIndex* pBlockIndex) {
    if(IsBlockchainSynced()) {
        CalculateUserStakes();
        if(pBlockIndex->nHeight > 200) {
            CBlockIndex* pIndex = chainActive[(pBlockIndex->nHeight-60)];
            if(pIndex && pBlockIndex) {
                nAverageBlockCount = (int)(3600 / ((pBlockIndex->GetBlockTime() - pIndex->GetBlockTime())/60));
            }
        }
        Optional<CAmount> shield = pBlockIndex->pprev->nChainSaplingValue;
        this->supplyShielded = shield ? *shield : 0;
    }
    if(!CheckTransactionMultiTier(pBlockIndex->nHeight,block,GetBlockValue(pBlockIndex->nHeight),pBlockIndex,view)) {
        std::cout << "MultiTierStaking::ConnectBlock -> Check Transactions failed, necessary payments not correct or missing" << std::endl;
        return false;
    }
    CheckBlockTransactionsForSpendOutputs(block);
    CheckBlockTransactionsForIncommingOutputs(block);

    return true;
}

bool CMultiTier::DisconnectBlock(const CBlock& block, CCoinsViewCache &view, const CBlockIndex* pBlockIndex) {
    for(int i = 0; i < block.vtx.size(); i++) {
        const CTransaction& txNew = *(block.vtx[i]);
        uint256 hash = txNew.GetHash();
        for(int j = 0; j < txNew.vout.size(); j++) {
            COutPoint outp{hash,(unsigned int)j};
            CAmount val = txNew.vout[j].nValue;
            if(IsMultiTierOutput(val)) {
                RemoveEntryMultiTierStaking(outp);
                std::cout << "DisconnetBlock: TransactionOutput removed" << std::endl;
            }
        }
        for(const auto& in : txNew.vin) {
            uint256 out = in.prevout.GetHash();
            Coin c;
            view.GetCoin(in.prevout,c);
            if(IsMultiTierOutput(c.out.nValue)) {
                AddEntryMultiTierStaking(in.prevout);
                std::cout << "DisconnetBlock: TransactionOutput added" << std::endl;
            }
        }
    }
    return true;
}

void CMultiTier::LockMultiTierUTXOs() {
    if(!pwalletMain) {
        return;
    }
    m.lock();
    {
        LOCK(pwalletMain->cs_wallet);
        for(const auto& out : mapMultiTierStaking ) {
            pwalletMain->LockCoin(out.second);
        }
    }
    m.unlock();
}

CAmount CMultiTier::GetCurrentSupplyTransparent() {
    return this->supplyTransparent;
}
CAmount CMultiTier::GetCurrentSupplyShielded() {
    return this->supplyShielded;
}
CAmount CMultiTier::GetStakingTierLevelCount(int level) {
    CAmount count = 0;
    m.lock();
    for(const auto& out : mapMultiTierStaking ) {
        Coin k;
        if(!pcoinsTip->GetCoin(out.second,k)) {
            continue;
        }
        if(GetMultiTierLevelAmount(level)==k.out.nValue)
            count += k.out.nValue;
    }
    m.unlock();
    return count;
}
int CMultiTier::GetAverageBlockCount() {
    return nAverageBlockCount;
}

CAmount CMultiTier::GetEligableStakeAmount() {
    LOCK(cs_main);
    pcoinsTip->Flush();
    nStakeableCoins = 0;
    const CAmount minStake = Params().GetConsensus().nMinStakeValue;
    std::unique_ptr<CCoinsViewCursor> pcursor(pcoinsTip->Cursor());
    while (pcursor->Valid()) {
        Coin coin;

        if (pcursor->GetValue(coin) && !coin.IsSpent() && coin.out.nValue >= minStake) {
            bool multitier = coin.out.nValue == MultiTier::TIER1 || coin.out.nValue == MultiTier::TIER2 ||
                         coin.out.nValue == MultiTier::TIER3 || coin.out.nValue == MultiTier::TIER4;

            if(!multitier)
                nStakeableCoins += coin.out.nValue;
        }
        pcursor->Next();
    }
    return  nStakeableCoins;
}
void CMultiTier::CalculateUserStakes() {
    if(!pwalletMain) {
        return;
    }
    nUserStakeValue = 0;
    nUserStakeTier1 = 0;
    nUserStakeTier2 = 0;
    nUserStakeTier3 = 0;
    nUserStakeTier4 = 0;
    nUserRewards = 0;
    CAmount nMinStake = Params().GetConsensus().nMinStakeValue;
    CWallet::AvailableCoinsFilter coinsFilter;
    coinsFilter.fIncludeDelegated = true;
    coinsFilter.fIncludeLocked = true;
    std::vector<COutput> vCoins;
    {
        LOCK(pwalletMain->cs_wallet);
        pwalletMain->AvailableCoins(&vCoins, nullptr, coinsFilter);
    }    
    for (const COutput& out : vCoins) {
        CAmount val = out.tx->tx->vout[out.i].nValue;
        if(val >= nMinStake) {
            switch(val) {
                case TIER1:
                    nUserStakeTier1 += val;
                    break;
                case TIER2:
                    nUserStakeTier2 += val;
                    break;
                case TIER3:
                    nUserStakeTier3 += val;
                    break;
                case TIER4:
                    nUserStakeTier4 += val;
                    break;
                default:
                    nUserStakeValue += out.tx->tx->vout[out.i].nValue;
                    break;
            }
        }
    }
    CAmount blockReward = GetActivePoSPhase(chainActive.Height());
    CAmount blockRewardTier1 = GetMultiTierValue(blockReward,1);
    CAmount blockRewardTier2 = GetMultiTierValue(blockReward,2);
    CAmount blockRewardTier3 = GetMultiTierValue(blockReward,3);
    CAmount blockRewardTier4 = GetMultiTierValue(blockReward,4);
    CAmount temp1 = (int64_t)(((double)nUserStakeTier1/GetStakingTierLevelCount(1))*blockRewardTier1*nAverageBlockCount);
    CAmount temp2 = (int64_t)(((double)nUserStakeTier2/GetStakingTierLevelCount(2))*blockRewardTier2*nAverageBlockCount);
    CAmount temp3 = (int64_t)(((double)nUserStakeTier3/GetStakingTierLevelCount(3))*blockRewardTier3*nAverageBlockCount);
    CAmount temp4 = (int64_t)(((double)nUserStakeTier4/GetStakingTierLevelCount(4))*blockRewardTier4*nAverageBlockCount);
    if(temp1 <= 0) {
        temp1 = 0;
    }
        if(temp2 <= 0) {
        temp2 = 0;
    }
        if(temp3 <= 0) {
        temp3 = 0;
    }
        if(temp4 <= 0) {
        temp4 = 0;
    }
    nUserRewards = nAverageBlockCount*24*(blockReward-blockRewardTier1-blockRewardTier2-blockRewardTier3-blockRewardTier4);
    nUserRewards = (int64_t )(nUserRewards*((double)nUserStakeValue/(double)nStakeableCoins));
    nUserRewards = nUserRewards + temp1 + temp2 + temp3 + temp4;
}
CAmount CMultiTier::GetUserStakeAmount(CAmount level) {
    switch (level)
    {
    case 1:
        return nUserStakeTier1;
        break;
    case 2:
        return nUserStakeTier2;
        break;
    case 3:
        return nUserStakeTier3;
        break;
    case 4:
        return nUserStakeTier4;
        break;
    default:
        break;
    }
    return nUserStakeValue;
}
CAmount CMultiTier::GetUserStakeRewards() {
    if(nUserRewards <= 0)
        return 0;
    return nUserRewards;
}
