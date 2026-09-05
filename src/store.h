#pragma once

#include "models.h"
#include <map>
#include <vector>
#include <string>
#include <set>

using namespace std;

// ─── In-Memory Data Store (Repository Pattern) ──────────────────────────────

class InMemoryDataStore {
public:
    // ── Merchants ──
    void addMerchant(const Merchant& m) { merchants_[m.id] = m; }

    Merchant* getMerchant(const string& id) {
        auto it = merchants_.find(id);
        return (it != merchants_.end()) ? &it->second : nullptr;
    }

    // ── Transactions ──
    void addTransaction(const Transaction& t) {
        transactions_[t.id] = t;
        merchantTxns_[t.merchantId].push_back(t.id);
    }

    Transaction* getTransaction(const string& id) {
        auto it = transactions_.find(id);
        return (it != transactions_.end()) ? &it->second : nullptr;
    }

    vector<Transaction> getTransactionsForMerchant(const string& merchantId) const {
        vector<Transaction> result;
        auto it = merchantTxns_.find(merchantId);

        if (it == merchantTxns_.end())
            return result;

        for (const auto& tid : it->second) {
            auto tit = transactions_.find(tid);

            if (tit != transactions_.end())
                result.push_back(tit->second);
        }

        return result;
    }

    // ── Batches ──
    void addBatch(const Batch& b) {
        batches_[b.id] = b;
        merchantBatches_[b.merchantId].push_back(b.id);
    }

    Batch* getBatch(const string& id) {
        auto it = batches_.find(id);
        return (it != batches_.end()) ? &it->second : nullptr;
    }

    vector<Batch> getBatchesForMerchant(const string& merchantId) const {
        vector<Batch> result;
        auto it = merchantBatches_.find(merchantId);

        if (it == merchantBatches_.end())
            return result;

        for (const auto& bid : it->second) {
            auto bit = batches_.find(bid);

            if (bit != batches_.end())
                result.push_back(bit->second);
        }

        return result;
    }

    // ── Track which txns are settled ──
    void markTxnSettled(const string& txnId) {
        settledTxns_.insert(txnId);
    }

    bool isTxnSettled(const string& txnId) const {
        return settledTxns_.count(txnId) > 0;
    }

    // ── Track which refund/chargeback txns have been applied ──
    void markAdjustmentApplied(const string& txnId) {
        appliedAdjustments_.insert(txnId);
    }

    bool isAdjustmentApplied(const string& txnId) const {
        return appliedAdjustments_.count(txnId) > 0;
    }

private:
    map<string, Merchant> merchants_;
    map<string, Transaction> transactions_;
    map<string, vector<string>> merchantTxns_;  // merchantId -> txnIds

    map<string, Batch> batches_;
    map<string, vector<string>> merchantBatches_; // merchantId -> batchIds

    set<string> settledTxns_;
    set<string> appliedAdjustments_;
};
