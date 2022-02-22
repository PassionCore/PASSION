#include "chat.h"
#include "netmessagemaker.h" //for messagemaker
#include "guiinterface.h" //for uiInterface
#include "net.h" // Conmanager
#include "key.h" // CKey // CPubKey
#include "base58.h"
#include "wallet/wallet.h"
#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <boost/thread.hpp>

void ProcessChatMessages()
{
    while(true) {
      sleep(5);
      // 1. Step is send messages which are waiting
      CChat::getInstance()->ProcessMessages();
      // 2. Remove outdated Entries or where we got no response
      CChat::getInstance()->RemoveOutdatedEntry();
    }
}

void static ThreadProcessChatMessages(void* parg)
{
    CChat* instance = (CChat*) parg;
    boost::this_thread::interruption_point();
    try {
        ProcessChatMessages();
        boost::this_thread::interruption_point();
    } catch (const std::exception& e) {
        std::cout << "Exception e \n";
    } catch (...) {
        std::cout << "Exception ... \n";
    }
}

CChat* CChat::instance = NULL;
CChat::CChat() {
    static boost::thread_group* chatThread = NULL;
    if (chatThread != NULL) {
        chatThread->interrupt_all();
        delete chatThread;
        chatThread = NULL;
    }
    chatThread = new boost::thread_group();
    chatThread->create_thread(std::bind(&ThreadProcessChatMessages, this));
}
void CChat::ProcessMessages() {
    mutMapChatMessages.lock();
    std::vector<uint256> send;
    for(auto &result : mapChatMessages) {
        if(IsAddressPubkeyAvailable(result.second.strDestination)) {
            //Pubkey Address available message can be send
            if(this->SendMessageToRecipient(result.second)) {
                send.emplace_back(result.first);
            }
        } else {
            if(!result.second.isPubKeyRequested) {
                CChatPubkeyExchange request;
                request.SetAddress(result.second.strDestination);
                vectRequestedPubkeys.emplace_back(result.second.strDestination);
                RelayPubkeyRequest(request);
                //Request the PubKey for given Address
                result.second.isPubKeyRequested = true;
                //std::cout << "ProcessMessages: PubkeyRequest\n";
            }
        }
    }
  for(auto res : send) {
      mapChatMessages.erase(res);
  }
  mutMapChatMessages.unlock();
}
void CChat::AddMessageToSend(std::string address, std::string message) {
    CChatMessage mess(address,message);
    mapChatMessages[mess.GetHash()] = mess;
}
bool CChat::SendMessageToRecipient(CChatMessage message)
{
    CPubKey destPubKey;
    GetPubKeyFromAddress(message.strDestination, destPubKey);
    if(!destPubKey.IsValid()) return false;
    std::set<CKeyID> keys;
    pwalletMain->GetKeys(keys);
    CKeyID keyid = *std::next(keys.begin(), GetTime()%(keys.size()-1));
    CKey cryptKey;
    pwalletMain->GetKey(keyid, cryptKey);
    if(!cryptKey.IsValid()) return false;
    message.setCryptoKey(uint256S(HexStr(cryptKey)));
    message.publicKey = cryptKey.GetPubKey();
    //TODO generate ECDH Key from private key and destination public key
    if(!GetECDHSecret(destPubKey,cryptKey,(unsigned char*)&message.key)) return false;
    message.cipher_length = message.encrypt((unsigned char*) message.strMessage.c_str(), message.strMessage.size(), (unsigned char*)&message.key, message.cipherText);
    //std::cout << "Encrypted Vector: " << HexStr(message.vchEncryptedMessage) << std::endl;
    this->RelayChatMessage(message);
    return true;
}
bool CChat::GetECDHSecret(CPubKey &pubkey, CKey &ckey, unsigned char *secret) {
    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    secp256k1_pubkey ppub1;
    if(!secp256k1_ec_pubkey_parse(ctx, &ppub1, pubkey.begin(), pubkey.size())) return false;
    if(!secp256k1_ecdh(ctx,secret,&ppub1,ckey.begin(),NULL,NULL)) return false;
    secp256k1_context_destroy(ctx);
    return true;
}

bool CChat::AlreadyReceived(uint256 messageHash) {
    bool ret = false;
    mutMapMessagesReceived.lock();
    auto found = mapMessagesReceived.find(messageHash);
    if (found != mapMessagesReceived.end()) {
        ret = true;
    } else {
        mapMessagesReceived[messageHash] = GetTime();
    }
    mutMapMessagesReceived.unlock();
    return ret;
}

bool CChat::GetPubKeyFromAddress(std::string address, CPubKey &pubkey) {
    bool ret = false;
    mutMapAddressPubkey.lock();
    auto result = mapAddressPubkey.find(address);
    if (result != mapAddressPubkey.end()) {
        pubkey = mapAddressPubkey[address];
        ret = true;
    }
    mutMapAddressPubkey.unlock();
    return ret;
}
void CChat::AddUpdateAddressWithPubKey(std::string address, CPubKey pubkey) {
    if(address.empty())
        return;
    mapAddressPubkey[address] = pubkey;
}
bool CChat::IsAddressPubkeyAvailable(std::string address) {
    bool ret = false;
    if(address.empty())
        return ret;
    mutMapAddressPubkey.lock();
    auto result = mapAddressPubkey.find(address);
    if (result != mapAddressPubkey.end()) {
        mutVectRequestPubkeys.lock();
        vectRequestedPubkeys.erase(std::remove(vectRequestedPubkeys.begin(), vectRequestedPubkeys.end(), address), vectRequestedPubkeys.end());
        mutVectRequestPubkeys.unlock();
        ret = true;
    }
    mutMapAddressPubkey.unlock();
    return ret;
}

bool CChat::IsPubkeyRequest(std::string address) {
    bool ret = false;
    if(address.empty()) return ret;
    mutVectRequestPubkeys.lock();
    if(std::find(vectRequestedPubkeys.begin(), vectRequestedPubkeys.end(), address) != vectRequestedPubkeys.end()) {
        ret = true;
    }
    mutVectRequestPubkeys.unlock();
    return ret;
}
void CChat::RelayPubkeyRequest(CChatPubkeyExchange request) {
    g_connman->ForEachNode([request](CNode* pnode) {
        //std::cout << "Send Pubkey Request to " << pnode->id << " for Address: " <<  request.GetAddress() << std::endl;
        g_connman->PushMessage(pnode, CNetMsgMaker(INIT_PROTO_VERSION).Make(NetMsgType::CHATMESSAGEPUBKEYREQUEST, request));
    });
}
void CChat::RelayPubkeyResponse(CChatPubkeyExchange response) {
    g_connman->ForEachNode([response](CNode* pnode) {
        //std::cout << "Send Pubkey Response to " << pnode->id << " for Address: " <<  response.GetAddress() << std::endl;
        g_connman->PushMessage(pnode, CNetMsgMaker(INIT_PROTO_VERSION).Make(NetMsgType::CHATMESSAGEPUBKEYRESPONSE, response));
    });
}
void CChat::RelayChatMessage(CChatMessage message) {
    g_connman->ForEachNode([message](CNode* pnode) {
        //std::cout << "Relay Message to " << pnode->id << " with destination: " <<  message.strDestination << std::endl;
        g_connman->PushMessage(pnode, CNetMsgMaker(INIT_PROTO_VERSION).Make(NetMsgType::CHATMESSAGE, message));
    });
}
void CChat::RemoveOutdatedEntry() {
    int64_t timestamp = GetTime();

    // Remove entries from received messages
    std::vector<uint256> cleanup;
    mutMapMessagesReceived.lock();
    for(auto& entry : mapMessagesReceived) {
        if(entry.second > timestamp - 3600) {
            cleanup.emplace_back(entry.first);
        }
    }
    for(auto& val : cleanup) {
        mapMessagesReceived.erase(val);
    }
    mutMapMessagesReceived.unlock();
    // Remove entries from Messages to send

}
void CChat::HandleReceivedChatMessage(CChatMessage message) {
    if(this->AlreadyReceived(message.GetHash())) {
        //std::cout << "Chatmessage already received and processed\n";
        return;
    }
    //std::cout << "Encrypted Vector: " << HexStr(message.vchEncryptedMessage) << std::endl;
    CTxDestination dest = DecodeDestination(message.strDestination);
    const CKeyID* keyID = boost::get<CKeyID>(&dest);
    //std::cout << "HandleReceivedChatMessage: " << message.strDestination << " is destination and keyID length: "<<keyID->size() << std::endl;
    CKey ckey;
    pwalletMain->GetKey(*keyID,ckey);
    if(ckey.IsValid()) {
        // Wallet has key we can decode the message and we are the recipient
        CTxDestination dFrom{message.publicKey.GetID()};
        std::string addrFrom = EncodeDestination(dFrom,false);
        //std::cout << "Message comes from: " << addrFrom << std::endl;
        message.strMessageFrom = addrFrom;
        GetECDHSecret(message.publicKey,ckey,(unsigned char*)&message.key);
        int decryptedtext_len = message.decrypt(message.vchEncryptedMessage.data(), message.cipher_length, (unsigned char*)&message.key, message.plainText);
        message.plainText[decryptedtext_len] = '\0';
        uiInterface.NotifyChatMessage(message);
    } else {
        // No key in wallet found therefore the message is not for us relay
        //std::cout << "HandleReceivedChatMessage: no key in walled found\n";
        this->RelayChatMessage(message);
    }
}
void CChat::HandlePubkeyRequest(CChatPubkeyExchange request) {
    if(this->AlreadyReceived(request.GetHash())) {
        //std::cout << "Pubkey Request already received and processed\n";
        return;
    } else {
        //std::cout << "Request Pubkey for address: "<< request.GetAddress() << " with Hash: " << HexStr(request.GetHash())<< std::endl;
        CTxDestination dest = DecodeDestination(request.GetAddress());
        const CKeyID* keyID = boost::get<CKeyID>(&dest);
        CPubKey pkey;
        pwalletMain->GetPubKey(*keyID,pkey);
        if(pkey.IsValid()) {
            //std::cout << "Pubkey: " << HexStr(pkey) << std::endl;
            request.SetPubKey(pkey);
            //this->HandlePubkeyResponse(request); // Todo to send internal Messages
            this->RelayPubkeyResponse(request);
            return;
        } else {
            //std::cout << "Address not inside the Keystore no Pubkey found -> Relay Message if needed" << std::endl;
        }
        this->RelayPubkeyRequest(request);
    }
}
void CChat::HandlePubkeyResponse(CChatPubkeyExchange response) {
    if(this->AlreadyReceived(response.GetHash())) {
        //std::cout << "Pubkey Response already received and processed\n";
        return;
    }
    if(!this->IsPubkeyRequest(response.GetAddress()))  {
        //std::cout << "Pubkey was not requested relay pubkey response\n";
        this->RelayPubkeyResponse(response);
    }
    // Check if it was requested otherwise relay it
    if(response.GetPubKey().IsValid()) {
        this->AddUpdateAddressWithPubKey(response.GetAddress(),response.GetPubKey());
        //std::cout << "Chatmessage Pubkey Response received  " << HexStr(response.GetPubKey()) << std::endl;
    } else {
        //std::cout << "Chatmessage Pubkey Response has no valid Pubkey" << std::endl;
    }
}

int CChatMessage::decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    if(!(ctx = EVP_CIPHER_CTX_new()))
        this->handleErrors();
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, NULL))
        this->handleErrors();
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        this->handleErrors();
    plaintext_len = len;
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        this->handleErrors();
    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}
int CChatMessage::encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        this->handleErrors();
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, NULL))
        this->handleErrors();
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        this->handleErrors();
    ciphertext_len = len;
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        this->handleErrors();
    ciphertext_len += len;
    vchEncryptedMessage = std::vector<unsigned char>(ciphertext,ciphertext+MESSAGE_BUFFER_LENGTH);
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}
void CChatMessage::handleErrors()
{
    ERR_print_errors_fp(stderr);
    abort();
}
void CChatMessage::setCryptoKey(uint256 key) {
  this->key = key;
}
