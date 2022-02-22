#ifndef CHAT_H
#define CHAT_H
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "serialize.h"
#include "key.h"
#include "utiltime.h"
#include <mutex>

class CChatMessage;
class CChatPubkeyExchange;
class CPubKey;

void ProcessChatMessages();

class CChat
{
   public:
      static CChat* getInstance()
      {
            if (instance == NULL)
            {
                  instance = new CChat();
            }
            return instance;
      }

   private:
      static CChat* instance;
      std::map<uint256,int64_t> mapMessagesReceived;
      std::map<std::string,CPubKey> mapAddressPubkey;
      std::vector<std::string> vectRequestedPubkeys;
      std::map<uint256,CChatMessage> mapChatMessages;
      std::mutex mutMapMessagesReceived;
      std::mutex mutMapAddressPubkey;
      std::mutex mutMapChatMessages;
      std::mutex mutVectRequestPubkeys;
      CChat();
      //CChat( const CChat& );
      //~CChat();

   public:
     void RemoveOutdatedEntry();
     void ProcessMessages();
     bool SendMessageToRecipient(CChatMessage message);
     bool AlreadyReceived(uint256 messageHash);
     bool GetPubKeyFromAddress(std::string address, CPubKey &pubkey);
     void AddUpdateAddressWithPubKey(std::string address, CPubKey pubkey);
     bool IsAddressPubkeyAvailable(std::string address);
     bool IsPubkeyRequest(std::string address);
     void RelayChatMessage(CChatMessage message);
     void RelayPubkeyRequest(CChatPubkeyExchange request);
     void RelayPubkeyResponse(CChatPubkeyExchange response);
     void HandleReceivedChatMessage(CChatMessage message);
     void HandlePubkeyRequest(CChatPubkeyExchange request);
     void HandlePubkeyResponse(CChatPubkeyExchange response);
     void AddMessageToSend(std::string address, std::string message);
     bool GetECDHSecret(CPubKey &pubkey, CKey &ckey, unsigned char *secret);
};

#define  MESSAGE_BUFFER_LENGTH 1024
class CChatMessage
{
   public:
      bool isCrypted;
      bool isRelayed;
      bool isPubKeyRequested;
      uint64_t timestamp;
      std::string strDestination;
      std::string strMessage;
      std::string strMessageFrom;
      std::vector<unsigned char> vchEncryptedMessage;
      unsigned char cipherText[MESSAGE_BUFFER_LENGTH];
      unsigned char plainText[MESSAGE_BUFFER_LENGTH];
      uint64_t cipher_length;
      uint256 key;
      CPubKey publicKey;

      ADD_SERIALIZE_METHODS;

      template <typename Stream, typename Operation>
      inline void SerializationOp(Stream& s, Operation ser_action) {
         READWRITE(isCrypted);
         READWRITE(isRelayed);
         READWRITE(strDestination);
         READWRITE(vchEncryptedMessage);
         READWRITE(cipher_length);
         READWRITE(publicKey);
         READWRITE(timestamp);
      }
      CChatMessage() {
         setNull();
      }
      CChatMessage(std::string dest, std::string message) {
         setNull();
         strDestination = dest;
         strMessage = message;
      }
      CChatMessage(const CChatMessage &message) {
         isCrypted = message.isCrypted;
         isRelayed = message.isRelayed;
         vchEncryptedMessage = message.vchEncryptedMessage;
         cipher_length = message.cipher_length;
         publicKey = message.publicKey;
         timestamp = message.timestamp;
         strDestination = message.strDestination;
         strMessage = message.strMessage;
         strMessageFrom = message.strMessageFrom;
         memcpy(this->cipherText,message.cipherText,MESSAGE_BUFFER_LENGTH);
         memcpy(this->plainText,message.plainText, MESSAGE_BUFFER_LENGTH);
         key = message.key;
      }
      bool operator==(CChatMessage &b) {
        if(this->strMessage == b.strMessage && this->strDestination == b.strDestination)
            return true;
        return false;
      }
      int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,unsigned char *ciphertext);
      int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *plaintext);
      void setCryptoKey(uint256 key);
      uint256 GetHash() const {
          return SerializeHash(*this);
      }
   std::string GetDecryptedMessage() {
      char* s = (char*)plainText;
      return std::string(s);
   }
   private:
      //CChat( const CChat& );
      //~CChat();
      void setNull() {
         isCrypted = false;
         isRelayed = false;
         isPubKeyRequested = false;
         timestamp = GetTime();
         cipher_length = 0;
         memset(&plainText[0], 0, MESSAGE_BUFFER_LENGTH);
         memset(&cipherText[0], 0, MESSAGE_BUFFER_LENGTH);
      }
      void handleErrors();
   public:

};

class CChatPubkeyExchange
{

   public:
      ADD_SERIALIZE_METHODS;
      template <typename Stream, typename Operation>
      inline void SerializationOp(Stream& s, Operation ser_action) {
          READWRITE(pkey);
          READWRITE(address);
      }
      CChatPubkeyExchange() {
          setNull();
      }
      CChatPubkeyExchange(const CChatPubkeyExchange &message) {
          this->pkey = message.pkey;
          this->address = message.address;
      }
   private:
      CPubKey pkey;
      std::string address;
      void setNull() {
        this->address = "";
      }
   public:
      uint256 GetHash() const {
          return SerializeHash(*this);
      }
      std::string GetAddress() const {
          return this->address;
      }
      CPubKey GetPubKey() const {
          return this->pkey;
      }
      void SetAddress(std::string addr) {
          if(!addr.empty()) {
              this->address = addr;
          }
      }
      void SetPubKey(CPubKey pubkey) {
          this->pkey = pubkey;
      }
};
#endif //CHAT_H
