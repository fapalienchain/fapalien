// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2014-2021 The Dash Core developers
// Copyright (c) 2020-2022 The Fapalien developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletdb.h>

#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <key_io.h>
#include <fs.h>
#include <governance/governance-object.h>
#include <protocol.h>
#include <serialize.h>
#include <sync.h>
#include <util.h>
#include <utiltime.h>
#include <wallet/wallet.h>
#include <validation.h>

#include <atomic>

#include <boost/thread.hpp>

//
// WalletBatch
//

bool WalletBatch::WriteName(const std::string& strAddress, const std::string& strName)
{
    return WriteIC(std::make_pair(std::string("name"), strAddress), strName);
}

bool WalletBatch::EraseName(const std::string& strAddress)
{
    // This should only be used for sending addresses, never for receiving addresses,
    // receiving addresses must always have an address book entry if they're not change return.
    return EraseIC(std::make_pair(std::string("name"), strAddress));
}

bool WalletBatch::WritePurpose(const std::string& strAddress, const std::string& strPurpose)
{
    return WriteIC(std::make_pair(std::string("purpose"), strAddress), strPurpose);
}

bool WalletBatch::ErasePurpose(const std::string& strAddress)
{
    return EraseIC(std::make_pair(std::string("purpose"), strAddress));
}

bool WalletBatch::WriteTx(const CWalletTx& wtx)
{
    return WriteIC(std::make_pair(std::string("tx"), wtx.GetHash()), wtx);
}

bool WalletBatch::EraseTx(uint256 hash)
{
    return EraseIC(std::make_pair(std::string("tx"), hash));
}

bool WalletBatch::WriteKeyMeta(const CPubKey& vchPubKey, const CKeyMetadata& keyMeta)
{
    return WriteIC(std::make_pair(std::string("keymeta"), vchPubKey), keyMeta, true);
}

bool WalletBatch::WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata& keyMeta)
{
    if (!WriteIC(std::make_pair(std::string("keymeta"), vchPubKey), keyMeta, false))
    {
        return false;
    }

    // hash pubkey/privkey to accelerate wallet load
    std::vector<unsigned char> vchKey;
    vchKey.reserve(vchPubKey.size() + vchPrivKey.size());
    vchKey.insert(vchKey.end(), vchPubKey.begin(), vchPubKey.end());
    vchKey.insert(vchKey.end(), vchPrivKey.begin(), vchPrivKey.end());

    return WriteIC(std::make_pair(std::string("key"), vchPubKey), std::make_pair(vchPrivKey, Hash(vchKey.begin(), vchKey.end())), false);
}

bool WalletBatch::WriteCryptedKey(const CPubKey& vchPubKey, const std::vector<unsigned char>& vchCryptedSecret, const CKeyMetadata &keyMeta)
{
    if (!WriteIC(std::make_pair(std::string("keymeta"), vchPubKey), keyMeta))
    {
        return false;
    }

    if (!WriteIC(std::make_pair(std::string("ckey"), vchPubKey), vchCryptedSecret, false))
    {
        return false;
    }
    EraseIC(std::make_pair(std::string("key"), vchPubKey));
    EraseIC(std::make_pair(std::string("wkey"), vchPubKey));
    return true;
}

bool WalletBatch::WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey)
{
    return WriteIC(std::make_pair(std::string("mkey"), nID), kMasterKey, true);
}

bool WalletBatch::WriteCScript(const uint160& hash, const CScript& redeemScript)
{
    return WriteIC(std::make_pair(std::string("cscript"), hash), redeemScript, false);
}

bool WalletBatch::WriteWatchOnly(const CScript &dest, const CKeyMetadata& keyMeta)
{
    if (!WriteIC(std::make_pair(std::string("watchmeta"), dest), keyMeta))
    {
        return false;
    }
    return WriteIC(std::make_pair(std::string("watchs"), dest), '1');
}

bool WalletBatch::EraseWatchOnly(const CScript &dest)
{
    if (!EraseIC(std::make_pair(std::string("watchmeta"), dest)))
    {
        return false;
    }
    return EraseIC(std::make_pair(std::string("watchs"), dest));
}

bool WalletBatch::WriteBestBlock(const CBlockLocator& locator)
{
    WriteIC(std::string("bestblock"), CBlockLocator()); // Write empty block locator so versions that require a merkle branch automatically rescan
    return WriteIC(std::string("bestblock_nomerkle"), locator);
}

bool WalletBatch::ReadBestBlock(CBlockLocator& locator)
{
    if (m_batch.Read(std::string("bestblock"), locator) && !locator.vHave.empty()) return true;
    return m_batch.Read(std::string("bestblock_nomerkle"), locator);
}

bool WalletBatch::WriteOrderPosNext(int64_t nOrderPosNext)
{
    return WriteIC(std::string("orderposnext"), nOrderPosNext);
}

bool WalletBatch::ReadPool(int64_t nPool, CKeyPool& keypool)
{
    return m_batch.Read(std::make_pair(std::string("pool"), nPool), keypool);
}

bool WalletBatch::WritePool(int64_t nPool, const CKeyPool& keypool)
{
    return WriteIC(std::make_pair(std::string("pool"), nPool), keypool);
}

bool WalletBatch::ErasePool(int64_t nPool)
{
    return EraseIC(std::make_pair(std::string("pool"), nPool));
}

bool WalletBatch::WriteMinVersion(int nVersion)
{
    return WriteIC(std::string("minversion"), nVersion);
}

bool WalletBatch::ReadAccount(const std::string& strAccount, CAccount& account)
{
    account.SetNull();
    return m_batch.Read(std::make_pair(std::string("acc"), strAccount), account);
}

bool WalletBatch::WriteAccount(const std::string& strAccount, const CAccount& account)
{
    return WriteIC(std::make_pair(std::string("acc"), strAccount), account);
}

bool WalletBatch::EraseAccount(const std::string& strAccount)
{
    return EraseIC(std::make_pair(std::string("acc"), strAccount));
}

bool WalletBatch::WriteAccountingEntry(const uint64_t nAccEntryNum, const CAccountingEntry& acentry)
{
    return WriteIC(std::make_pair(std::string("acentry"), std::make_pair(acentry.strAccount, nAccEntryNum)), acentry);
}

bool WalletBatch::ReadCoinJoinSalt(uint256& salt, bool fLegacy)
{
    // TODO: Remove legacy checks after few major releases
    return m_batch.Read(std::string(fLegacy ? "ps_salt" : "cj_salt"), salt);
}

bool WalletBatch::WriteCoinJoinSalt(const uint256& salt)
{
    return WriteIC(std::string("cj_salt"), salt);
}

bool WalletBatch::WriteGovernanceObject(const CGovernanceObject& obj)
{
    return WriteIC(std::make_pair(std::string("gobject"), obj.GetHash()), obj, false);
}

CAmount WalletBatch::GetAccountCreditDebit(const std::string& strAccount)
{
    std::list<CAccountingEntry> entries;
    ListAccountCreditDebit(strAccount, entries);

    CAmount nCreditDebit = 0;
    for (const CAccountingEntry& entry : entries)
        nCreditDebit += entry.nCreditDebit;

    return nCreditDebit;
}

void WalletBatch::ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& entries)
{
    bool fAllAccounts = (strAccount == "*");

    Dbc* pcursor = m_batch.GetCursor();
    if (!pcursor)
        throw std::runtime_error(std::string(__func__) + ": cannot create DB cursor");
    bool setRange = true;
    while (true)
    {
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        if (setRange)
            ssKey << std::make_pair(std::string("acentry"), std::make_pair((fAllAccounts ? std::string("") : strAccount), uint64_t(0)));
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = m_batch.ReadAtCursor(pcursor, ssKey, ssValue, setRange);
        setRange = false;
        if (ret == DB_NOTFOUND)
            break;
        else if (ret != 0)
        {
            pcursor->close();
            throw std::runtime_error(std::string(__func__) + ": error scanning DB");
        }

        // Unserialize
        std::string strType;
        ssKey >> strType;
        if (strType != "acentry")
            break;
        CAccountingEntry acentry;
        ssKey >> acentry.strAccount;
        if (!fAllAccounts && acentry.strAccount != strAccount)
            break;

        ssValue >> acentry;
        ssKey >> acentry.nEntryNo;
        entries.push_back(acentry);
    }

    pcursor->close();
}

class CWalletScanState {
public:
    unsigned int nKeys;
    unsigned int nCKeys;
    unsigned int nWatchKeys;
    unsigned int nHDPubKeys;
    unsigned int nKeyMeta;
    unsigned int m_unknown_records;
    bool fIsEncrypted;
    bool fAnyUnordered;
    int nFileVersion;
    std::vector<uint256> vWalletUpgrade;

    CWalletScanState() {
        nKeys = nCKeys = nWatchKeys = nHDPubKeys = nKeyMeta = m_unknown_records = 0;
        fIsEncrypted = false;
        fAnyUnordered = false;
        nFileVersion = 0;
    }
};

bool
ReadKeyValue(CWallet* pwallet, CDataStream& ssKey, CDataStream& ssValue,
             CWalletScanState &wss, std::string& strType, std::string& strErr) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)
{
    try {
        // Unserialize
        // Taking advantage of the fact that pair serialization
        // is just the two items serialized one after the other
        ssKey >> strType;
        if (strType == "name")
        {
            std::string strAddress;
            ssKey >> strAddress;
            ssValue >> pwallet->mapAddressBook[DecodeDestination(strAddress)].name;
        }
        else if (strType == "purpose")
        {
            std::string strAddress;
            ssKey >> strAddress;
            ssValue >> pwallet->mapAddressBook[DecodeDestination(strAddress)].purpose;
        }
        else if (strType == "tx")
        {
            uint256 hash;
            ssKey >> hash;
            CWalletTx wtx(nullptr /* pwallet */, MakeTransactionRef());
            ssValue >> wtx;
            CValidationState state;
            if (!(CheckTransaction(*wtx.tx, state, 0, 0) && (wtx.GetHash() == hash) && state.IsValid()))
                return false;

            // Undo serialize changes in 31600
            if (31404 <= wtx.fTimeReceivedIsTxTime && wtx.fTimeReceivedIsTxTime <= 31703)
            {
                if (!ssValue.empty())
                {
                    char fTmp;
                    char fUnused;
                    ssValue >> fTmp >> fUnused >> wtx.strFromAccount;
                    strErr = strprintf("LoadWallet() upgrading tx ver=%d %d '%s' %s", wtx.fTimeReceivedIsTxTime, fTmp, wtx.strFromAccount, hash.ToString());
                    wtx.fTimeReceivedIsTxTime = fTmp;
                }
                else
                {
                    strErr = strprintf("LoadWallet() repairing tx ver=%d %s", wtx.fTimeReceivedIsTxTime, hash.ToString());
                    wtx.fTimeReceivedIsTxTime = 0;
                }
                wss.vWalletUpgrade.push_back(hash);
            }

            if (wtx.nOrderPos == -1)
                wss.fAnyUnordered = true;

            pwallet->LoadToWallet(wtx);
        }
        else if (strType == "acentry")
        {
            std::string strAccount;
            ssKey >> strAccount;
            uint64_t nNumber;
            ssKey >> nNumber;
            if (nNumber > pwallet->nAccountingEntryNumber)
            {
                pwallet->nAccountingEntryNumber = nNumber;
            }

            if (!wss.fAnyUnordered)
            {
                CAccountingEntry acentry;
                ssValue >> acentry;
                if (acentry.nOrderPos == -1)
                    wss.fAnyUnordered = true;
            }
        }
        else if (strType == "watchs")
        {
            wss.nWatchKeys++;
            CScript script;
            ssKey >> script;
            char fYes;
            ssValue >> fYes;
            if (fYes == '1')
                pwallet->LoadWatchOnly(script);
        }
        else if (strType == "key" || strType == "wkey")
        {
            CPubKey vchPubKey;
            ssKey >> vchPubKey;
            if (!vchPubKey.IsValid())
            {
                strErr = "Error reading wallet database: CPubKey corrupt";
                return false;
            }
            CKey key;
            CPrivKey pkey;
            uint256 hash;

            if (strType == "key")
            {
                wss.nKeys++;
                ssValue >> pkey;
            } else {
                CWalletKey wkey;
                ssValue >> wkey;
                pkey = wkey.vchPrivKey;
            }

            // Old wallets store keys as "key" [pubkey] => [privkey]
            // ... which was slow for wallets with lots of keys, because the public key is re-derived from the private key
            // using EC operations as a checksum.
            // Newer wallets store keys as "key"[pubkey] => [privkey][hash(pubkey,privkey)], which is much faster while
            // remaining backwards-compatible.
            try
            {
                ssValue >> hash;
            }
            catch (...) {}

            bool fSkipCheck = false;

            if (!hash.IsNull())
            {
                // hash pubkey/privkey to accelerate wallet load
                std::vector<unsigned char> vchKey;
                vchKey.reserve(vchPubKey.size() + pkey.size());
                vchKey.insert(vchKey.end(), vchPubKey.begin(), vchPubKey.end());
                vchKey.insert(vchKey.end(), pkey.begin(), pkey.end());

                if (Hash(vchKey.begin(), vchKey.end()) != hash)
                {
                    strErr = "Error reading wallet database: CPubKey/CPrivKey corrupt";
                    return false;
                }

                fSkipCheck = true;
            }

            if (!key.Load(pkey, vchPubKey, fSkipCheck))
            {
                strErr = "Error reading wallet database: CPrivKey corrupt";
                return false;
            }
            if (!pwallet->LoadKey(key, vchPubKey))
            {
                strErr = "Error reading wallet database: LoadKey failed";
                return false;
            }
        }
        else if (strType == "mkey")
        {
            unsigned int nID;
            ssKey >> nID;
            CMasterKey kMasterKey;
            ssValue >> kMasterKey;
            if(pwallet->mapMasterKeys.count(nID) != 0)
            {
                strErr = strprintf("Error reading wallet database: duplicate CMasterKey id %u", nID);
                return false;
            }
            pwallet->mapMasterKeys[nID] = kMasterKey;
            if (pwallet->nMasterKeyMaxID < nID)
                pwallet->nMasterKeyMaxID = nID;
        }
        else if (strType == "ckey")
        {
            CPubKey vchPubKey;
            ssKey >> vchPubKey;
            if (!vchPubKey.IsValid())
            {
                strErr = "Error reading wallet database: CPubKey corrupt";
                return false;
            }
            std::vector<unsigned char> vchPrivKey;
            ssValue >> vchPrivKey;
            wss.nCKeys++;

            if (!pwallet->LoadCryptedKey(vchPubKey, vchPrivKey))
            {
                strErr = "Error reading wallet database: LoadCryptedKey failed";
                return false;
            }
            wss.fIsEncrypted = true;
        }
        else if (strType == "keymeta")
        {
            CPubKey vchPubKey;
            ssKey >> vchPubKey;
            CKeyMetadata keyMeta;
            ssValue >> keyMeta;
            wss.nKeyMeta++;
            pwallet->LoadKeyMetadata(vchPubKey.GetID(), keyMeta);
        }
        else if (strType == "watchmeta")
        {
            CScript script;
            ssKey >> script;
            CKeyMetadata keyMeta;
            ssValue >> keyMeta;
            wss.nKeyMeta++;
            pwallet->LoadScriptMetadata(CScriptID(script), keyMeta);
        }
        else if (strType == "defaultkey")
        {
            // We don't want or need the default key, but if there is one set,
            // we want to make sure that it is valid so that we can detect corruption
            CPubKey vchPubKey;
            ssValue >> vchPubKey;
            if (!vchPubKey.IsValid()) {
                strErr = "Error reading wallet database: Default Key corrupt";
                return false;
            }
        }
        else if (strType == "pool")
        {
            int64_t nIndex;
            ssKey >> nIndex;
            CKeyPool keypool;
            ssValue >> keypool;
            pwallet->LoadKeyPool(nIndex, keypool);
        }
        else if (strType == "version")
        {
            ssValue >> wss.nFileVersion;
            if (wss.nFileVersion == 10300)
                wss.nFileVersion = 300;
        }
        else if (strType == "cscript")
        {
            uint160 hash;
            ssKey >> hash;
            CScript script;
            ssValue >> script;
            if (!pwallet->LoadCScript(script))
            {
                strErr = "Error reading wallet database: LoadCScript failed";
                return false;
            }
        }
        else if (strType == "orderposnext")
        {
            ssValue >> pwallet->nOrderPosNext;
        }
        else if (strType == "destdata")
        {
            std::string strAddress, strKey, strValue;
            ssKey >> strAddress;
            ssKey >> strKey;
            ssValue >> strValue;
            if (!pwallet->LoadDestData(DecodeDestination(strAddress), strKey, strValue))
            {
                strErr = "Error reading wallet database: LoadDestData failed";
                return false;
            }
        }
        else if (strType == "hdchain")
        {
            CHDChain chain;
            ssValue >> chain;
            if (!pwallet->SetHDChainSingle(chain, true))
            {
                strErr = "Error reading wallet database: SetHDChain failed";
                return false;
            }
        }
        else if (strType == "chdchain")
        {
            CHDChain chain;
            ssValue >> chain;
            if (!pwallet->SetCryptedHDChainSingle(chain, true))
            {
                strErr = "Error reading wallet database: SetHDCryptedChain failed";
                return false;
            }
        }
        else if (strType == "hdpubkey")
        {
            wss.nHDPubKeys++;
            CPubKey vchPubKey;
            ssKey >> vchPubKey;

            CHDPubKey hdPubKey;
            ssValue >> hdPubKey;

            if(vchPubKey != hdPubKey.extPubKey.pubkey)
            {
                strErr = "Error reading wallet database: CHDPubKey corrupt";
                return false;
            }
            if (!pwallet->LoadHDPubKey(hdPubKey))
            {
                strErr = "Error reading wallet database: LoadHDPubKey failed";
                return false;
            }
        } else if (strType == "gobject") {
            uint256 nObjectHash;
            CGovernanceObject obj;
            ssKey >> nObjectHash;
            ssValue >> obj;

            if (obj.GetHash() != nObjectHash) {
                strErr = "Invalid governance object: Hash mismatch";
                return false;
            }

            if (!pwallet->LoadGovernanceObject(obj)) {
                strErr = "Invalid governance object: LoadGovernanceObject";
                return false;
            }
        }
        else if (strType != "bestblock" && strType != "bestblock_nomerkle"){
            wss.m_unknown_records++;
        }
    } catch (...)
    {
        return false;
    }
    return true;
}

bool WalletBatch::IsKeyType(const std::string& strType)
{
    return (strType== "key" || strType == "wkey" ||
            strType == "mkey" || strType == "ckey" ||
            strType == "hdchain" || strType == "chdchain");
}

DBErrors WalletBatch::LoadWallet(CWallet* pwallet)
{
    CWalletScanState wss;
    bool fNoncriticalErrors = false;
    DBErrors result = DBErrors::LOAD_OK;

    LOCK2(cs_main, pwallet->cs_wallet);
    try {
        int nMinVersion = 0;
        if (m_batch.Read((std::string)"minversion", nMinVersion))
        {
            if (nMinVersion > FEATURE_LATEST)
                return DBErrors::TOO_NEW;
            pwallet->LoadMinVersion(nMinVersion);
        }

        // Get cursor
        Dbc* pcursor = m_batch.GetCursor();
        if (!pcursor)
        {
            LogPrintf("Error getting wallet database cursor\n");
            return DBErrors::CORRUPT;
        }

        while (true)
        {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = m_batch.ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
            {
                LogPrintf("Error reading next record from wallet database\n");
                return DBErrors::CORRUPT;
            }

            // Try to be tolerant of single corrupt records:
            std::string strType, strErr;
            if (!ReadKeyValue(pwallet, ssKey, ssValue, wss, strType, strErr))
            {
                // losing keys is considered a catastrophic error, anything else
                // we assume the user can live with:
                if (IsKeyType(strType) || strType == "defaultkey")
                    result = DBErrors::CORRUPT;
                else
                {
                    // Leave other errors alone, if we try to fix them we might make things worse.
                    fNoncriticalErrors = true; // ... but do warn the user there is something wrong.
                    if (strType == "tx")
                        // Rescan if there is a bad transaction record:
                        gArgs.SoftSetBoolArg("-rescan", true);
                }
            }
            if (!strErr.empty())
                LogPrintf("%s\n", strErr);
        }
        pcursor->close();

        // Store initial external keypool size since we mostly use external keys in mixing
        pwallet->nKeysLeftSinceAutoBackup = pwallet->KeypoolCountExternalKeys();
        LogPrintf("nKeysLeftSinceAutoBackup: %d\n", pwallet->nKeysLeftSinceAutoBackup);
    }
    catch (const boost::thread_interrupted&) {
        throw;
    }
    catch (...) {
        result = DBErrors::CORRUPT;
    }

    if (fNoncriticalErrors && result == DBErrors::LOAD_OK)
        result = DBErrors::NONCRITICAL_ERROR;

    // Any wallet corruption at all: skip any rewriting or
    // upgrading, we don't want to make it worse.
    if (result != DBErrors::LOAD_OK)
        return result;

    LogPrintf("nFileVersion = %d\n", wss.nFileVersion);

    LogPrintf("Keys: %u plaintext, %u encrypted, %u total; Watch scripts: %u; HD PubKeys: %u; Metadata: %u; Unknown wallet records: %u\n",
           wss.nKeys, wss.nCKeys, wss.nKeys + wss.nCKeys,
           wss.nWatchKeys, wss.nHDPubKeys, wss.nKeyMeta, wss.m_unknown_records);

    // nTimeFirstKey is only reliable if all keys have metadata
    if ((wss.nKeys + wss.nCKeys + wss.nWatchKeys + wss.nHDPubKeys) != wss.nKeyMeta)
        pwallet->UpdateTimeFirstKey(1);

    for (uint256 hash : wss.vWalletUpgrade)
        WriteTx(pwallet->mapWallet.at(hash));

    // Rewrite encrypted wallets of versions 0.4.0 and 0.5.0rc:
    if (wss.fIsEncrypted && (wss.nFileVersion == 40000 || wss.nFileVersion == 50000))
        return DBErrors::NEED_REWRITE;

    if (wss.nFileVersion < CLIENT_VERSION) // Update
        WriteVersion(CLIENT_VERSION);

    if (wss.fAnyUnordered)
        result = pwallet->ReorderTransactions();

    pwallet->laccentries.clear();
    ListAccountCreditDebit("*", pwallet->laccentries);
    for (CAccountingEntry& entry : pwallet->laccentries) {
        pwallet->wtxOrdered.insert(make_pair(entry.nOrderPos, CWallet::TxPair(nullptr, &entry)));
    }

    return result;
}

DBErrors WalletBatch::FindWalletTx(std::vector<uint256>& vTxHash, std::vector<CWalletTx>& vWtx)
{
    DBErrors result = DBErrors::LOAD_OK;

    try {
        int nMinVersion = 0;
        if (m_batch.Read((std::string)"minversion", nMinVersion))
        {
            if (nMinVersion > FEATURE_LATEST)
                return DBErrors::TOO_NEW;
        }

        // Get cursor
        Dbc* pcursor = m_batch.GetCursor();
        if (!pcursor)
        {
            LogPrintf("Error getting wallet database cursor\n");
            return DBErrors::CORRUPT;
        }

        while (true)
        {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = m_batch.ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
            {
                LogPrintf("Error reading next record from wallet database\n");
                return DBErrors::CORRUPT;
            }

            std::string strType;
            ssKey >> strType;
            if (strType == "tx") {
                uint256 hash;
                ssKey >> hash;

                CWalletTx wtx(nullptr /* pwallet */, MakeTransactionRef());
                ssValue >> wtx;

                vTxHash.push_back(hash);
                vWtx.push_back(wtx);
            }
        }
        pcursor->close();
    }
    catch (const boost::thread_interrupted&) {
        throw;
    }
    catch (...) {
        result = DBErrors::CORRUPT;
    }

    return result;
}

DBErrors WalletBatch::ZapSelectTx(std::vector<uint256>& vTxHashIn, std::vector<uint256>& vTxHashOut)
{
    // build list of wallet TXs and hashes
    std::vector<uint256> vTxHash;
    std::vector<CWalletTx> vWtx;
    DBErrors err = FindWalletTx(vTxHash, vWtx);
    if (err != DBErrors::LOAD_OK) {
        return err;
    }

    std::sort(vTxHash.begin(), vTxHash.end());
    std::sort(vTxHashIn.begin(), vTxHashIn.end());

    // erase each matching wallet TX
    bool delerror = false;
    std::vector<uint256>::iterator it = vTxHashIn.begin();
    for (uint256 hash : vTxHash) {
        while (it < vTxHashIn.end() && (*it) < hash) {
            it++;
        }
        if (it == vTxHashIn.end()) {
            break;
        }
        else if ((*it) == hash) {
            if(!EraseTx(hash)) {
                LogPrint(BCLog::DB, "Transaction was found for deletion but returned database error: %s\n", hash.GetHex());
                delerror = true;
            }
            vTxHashOut.push_back(hash);
        }
    }

    if (delerror) {
        return DBErrors::CORRUPT;
    }
    return DBErrors::LOAD_OK;
}

DBErrors WalletBatch::ZapWalletTx(std::vector<CWalletTx>& vWtx)
{
    // build list of wallet TXs
    std::vector<uint256> vTxHash;
    DBErrors err = FindWalletTx(vTxHash, vWtx);
    if (err != DBErrors::LOAD_OK)
        return err;

    // erase each wallet TX
    for (uint256& hash : vTxHash) {
        if (!EraseTx(hash))
            return DBErrors::CORRUPT;
    }

    return DBErrors::LOAD_OK;
}

void MaybeCompactWalletDB()
{
    static std::atomic<bool> fOneThread(false);
    if (fOneThread.exchange(true)) {
        return;
    }
    if (!gArgs.GetBoolArg("-flushwallet", DEFAULT_FLUSHWALLET)) {
        return;
    }

    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        WalletDatabase& dbh = pwallet->GetDBHandle();

        unsigned int nUpdateCounter = dbh.nUpdateCounter;

        if (dbh.nLastSeen != nUpdateCounter) {
            dbh.nLastSeen = nUpdateCounter;
            dbh.nLastWalletUpdate = GetTime();
        }

        if (dbh.nLastFlushed != nUpdateCounter && GetTime() - dbh.nLastWalletUpdate >= 2) {
            if (BerkeleyBatch::PeriodicFlush(dbh)) {
                dbh.nLastFlushed = nUpdateCounter;
            }
        }
    }

    fOneThread = false;
}

//
// Try to (very carefully!) recover wallet file if there is a problem.
//
bool WalletBatch::Recover(const fs::path& wallet_path, void *callbackDataIn, bool (*recoverKVcallback)(void* callbackData, CDataStream ssKey, CDataStream ssValue), std::string& out_backup_filename)
{
    return BerkeleyBatch::Recover(wallet_path, callbackDataIn, recoverKVcallback, out_backup_filename);
}

bool WalletBatch::Recover(const fs::path& wallet_path, std::string& out_backup_filename)
{
    // recover without a key filter callback
    // results in recovering all record types
    return WalletBatch::Recover(wallet_path, nullptr, nullptr, out_backup_filename);
}

bool WalletBatch::RecoverKeysOnlyFilter(void *callbackData, CDataStream ssKey, CDataStream ssValue)
{
    CWallet *dummyWallet = reinterpret_cast<CWallet*>(callbackData);
    CWalletScanState dummyWss;
    std::string strType, strErr;
    bool fReadOK;
    {
        // Required in LoadKeyMetadata():
        LOCK(dummyWallet->cs_wallet);
        fReadOK = ReadKeyValue(dummyWallet, ssKey, ssValue,
                               dummyWss, strType, strErr);
    }
    if (!IsKeyType(strType) && strType != "hdpubkey")
        return false;
    if (!fReadOK)
    {
        LogPrintf("WARNING: WalletBatch::Recover skipping %s: %s\n", strType, strErr);
        return false;
    }

    return true;
}

bool WalletBatch::VerifyEnvironment(const fs::path& wallet_path, std::string& errorStr)
{
    return BerkeleyBatch::VerifyEnvironment(wallet_path, errorStr);
}

bool WalletBatch::VerifyDatabaseFile(const fs::path& wallet_path, std::string& warningStr, std::string& errorStr)
{
    return BerkeleyBatch::VerifyDatabaseFile(wallet_path, warningStr, errorStr, WalletBatch::Recover);
}

bool WalletBatch::WriteDestData(const std::string &address, const std::string &key, const std::string &value)
{
    return WriteIC(std::make_pair(std::string("destdata"), std::make_pair(address, key)), value);
}

bool WalletBatch::EraseDestData(const std::string &address, const std::string &key)
{
    return EraseIC(std::make_pair(std::string("destdata"), std::make_pair(address, key)));
}

bool WalletBatch::WriteHDChain(const CHDChain& chain)
{
    return WriteIC(std::string("hdchain"), chain);
}

bool WalletBatch::WriteCryptedHDChain(const CHDChain& chain)
{
    if (!WriteIC(std::string("chdchain"), chain))
        return false;

    EraseIC(std::string("hdchain"));

    return true;
}

bool WalletBatch::WriteHDPubKey(const CHDPubKey& hdPubKey, const CKeyMetadata& keyMeta)
{
    if (!WriteIC(std::make_pair(std::string("keymeta"), hdPubKey.extPubKey.pubkey), keyMeta, false))
        return false;

    return WriteIC(std::make_pair(std::string("hdpubkey"), hdPubKey.extPubKey.pubkey), hdPubKey, false);
}

bool WalletBatch::TxnBegin()
{
    return m_batch.TxnBegin();
}

bool WalletBatch::TxnCommit()
{
    return m_batch.TxnCommit();
}

bool WalletBatch::TxnAbort()
{
    return m_batch.TxnAbort();
}

bool WalletBatch::ReadVersion(int& nVersion)
{
    return m_batch.ReadVersion(nVersion);
}

bool WalletBatch::WriteVersion(int nVersion)
{
    return m_batch.WriteVersion(nVersion);
}
