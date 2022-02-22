#ifndef MULTITIER_H
#define MULTITIER_H
#include <stdio.h>
#include <stdlib.h>
#include "amount.h"
#include "chain.h"
#include "script/script.h"
#include "primitives/transaction.h"
#include "primitives/block.h"
#include "coins.h"
#include "validationinterface.h"
#include <mutex>

class Coin;
class CBlock;
class CChain;
class CBlockHeader;
class CCoinsViewCache;
class CWallet;
class CWalletTx;

static bool STAKE_AUTO_LOCK_UTXO = true;

class CMultiTier
{
   public:      
      static CMultiTier* getInstance()
      {
            if (instance == NULL)
            {
                  instance = new CMultiTier();
            }
            return instance;
      }

   private:
      static CMultiTier* instance;
      int nAverageBlockCount;
      CAmount supplyTransparent;
      CAmount supplyShielded;
      CAmount nUserRewards;
      CAmount nStakeableCoins;
      CAmount nUserStakeValue;
      CAmount nUserStakeTier1;
      CAmount nUserStakeTier2;
      CAmount nUserStakeTier3;
      CAmount nUserStakeTier4;
      void CheckBlockTransactionsForSpendOutputs(const CBlock& block);
      void CheckBlockTransactionsForIncommingOutputs(const CBlock& block);
      void RemoveEntryMultiTierStaking(const COutPoint &out);
      void CalculateUserStakes();
      CMultiTier() {
         supplyShielded = 0;
         nUserRewards = 0;
         supplyTransparent = 0;
         nAverageBlockCount = 0;
         nStakeableCoins = 0;
         nUserStakeValue = 0;
         nUserStakeTier1 = 0;
         nUserStakeTier2 = 0;
         nUserStakeTier3 = 0;
         nUserStakeTier4 = 0;
      }
      std::map<uint256,COutPoint> mapMultiTierStaking;
      std::mutex g_mutMultiTierStaking;
      //CMultiTier( const CMultiTier& );
      //~CMultiTier();

   public:
      bool CheckTransactionMultiTier(int nHeight, const CBlock& block, CAmount blockValue, const CBlockIndex* const pindexPrev, const CCoinsViewCache &view);
      bool FillTransactionMultiTier(CMutableTransaction& txNew, CAmount& blockValue, const CBlockIndex* const pindexPrev);
      void AddEntryMultiTierStaking(const COutPoint &out);
      CAmount GetMultiTierValue(CAmount blockValue, int level);
      void PrintMapMultiStaking(const CBlockIndex* const pindexPrev, const CCoinsViewCache &view);
      CAmount GetMultiTierLevelAmount(int level);
      bool IsMultiTierOutput(CAmount value);
      bool ConnectBlock(const CBlock& block, const CCoinsViewCache &view, const CBlockIndex* pBlockIndex);
      bool DisconnectBlock(const CBlock& block, CCoinsViewCache &view, const CBlockIndex* pBlockIndex);
      //Functions for Statistics
      CAmount GetCurrentSupplyTransparent();
      CAmount GetCurrentSupplyShielded();
      CAmount GetStakingTierLevelCount(int level);
      int GetAverageBlockCount();
      void LockMultiTierUTXOs();
      CAmount GetActivePoSPhase(int nHeight);
      CAmount GetNextPoSPhase(int nHeight);
      CAmount GetEligableStakeAmount();
      CAmount GetUserStakeAmount(CAmount level);
      CAmount GetUserStakeRewards();
};
#endif //MULTITIER_H
