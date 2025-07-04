// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2020-2023 The Fapalien developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TRANSACTIONDESC_H
#define BITCOIN_QT_TRANSACTIONDESC_H

#include <memory>

#include <QObject>
#include <QString>

class TransactionRecord;

namespace interfaces {
class Node;
class Wallet;
struct WalletTx;
struct WalletTxStatus;
}
class CFutureTx;


/** Provide a human-readable extended HTML description of a transaction.
 */
class TransactionDesc: public QObject
{
    Q_OBJECT

public:
    static QString toHTML(interfaces::Node& node, interfaces::Wallet& wallet, TransactionRecord *rec, int unit);

private:
    TransactionDesc() {}

    static QString FormatTxStatus(std::shared_ptr<const interfaces::WalletTx> wtx, const interfaces::WalletTxStatus& status, bool inMempool, int numBlocks, int64_t adjustedTime);
    static QString FutureTxDescToHTML(std::shared_ptr<const interfaces::WalletTx> wtx, const interfaces::WalletTxStatus& status, CFutureTx& ftx, int unit);

};

#endif // BITCOIN_QT_TRANSACTIONDESC_H
