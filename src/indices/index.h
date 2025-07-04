// Copyright (c) 2023 The Fapalien Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FAPA_INDEX_H
#define FAPA_INDEX_H

#include <amount.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>

struct IndexKey {
    uint256 txid;
    unsigned int outputIndex;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(txid);
        READWRITE(outputIndex);
    }

    IndexKey(uint256 hash, unsigned int index) :
       txid(hash),
       outputIndex(index)
    {
    }

    IndexKey()
    {
        SetNull();
    }

    void SetNull()
    {
        txid.SetNull();
        outputIndex = 0;
    }
};

struct CIndexKeyCompare {
    bool operator()(const IndexKey& a, const IndexKey& b) const
    {
        if (a.txid == b.txid) {
            return a.outputIndex < b.outputIndex;
        } else {
            return a.txid < b.txid;
        }
    }
};

#endif