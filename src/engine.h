#pragma once

#include "models.h"
#include "commission.h"
#include "store.h"
#include "audit.h"

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <set>
#include <memory>

using namespace std;

// ─── Settlement Engine ──────────────────────────────────────────────────────

class SettlementEngine {
public:
    // ── Register a merchant ──
    bool registerMerchant(const Merchant& m) {
        if (store_.getMerchant(m.id)) {
            cerr << "ERROR: Merchant " << m.id << " already exists.\n";
            return false;
        }

        store_.addMerchant(m);

        audit_.log("MERCHANT", m.id, "REGISTERED",
                   "Plan=" + planName(m.planType));

        return true;
    }

    // ── Record a transaction ──
    bool recordTransaction(const Transaction& t) {
        Merchant* m = store_.getMerchant(t.merchantId);

        if (!m) {
            cerr << "ERROR: Merchant " << t.merchantId << " not found.\n";
            return false;
        }

        // Validate refund/chargeback references
        if (t.type == TxnType::REFUND || t.type == TxnType::CHARGEBACK) {
            if (t.refTxnId.empty()) {
                cerr << "ERROR: " << txnTypeName(t.type)
                     << " must reference an original transaction.\n";
                return false;
            }

            Transaction* orig = store_.getTransaction(t.refTxnId);

            if (!orig) {
                cerr << "ERROR: Referenced transaction " << t.refTxnId
                     << " does not exist. " << txnTypeName(t.type)
                     << " rejected.\n";

                audit_.log("TRANSACTION", t.id, "REJECTED",
                           "Invalid ref txn " + t.refTxnId);

                return false;
            }
        }

        store_.addTransaction(t);

        audit_.log("TRANSACTION", t.id, "RECORDED",
                   txnTypeName(t.type) + " $" + fmtAmt(t.amount) +
                   " merchant=" + t.merchantId);

        return true;
    }

    // ── Settle a batch for a merchant on a given date ──
    bool settleBatch(const string& merchantId, const string& date) {
        Merchant* m = store_.getMerchant(merchantId);

        if (!m) {
            cerr << "ERROR: Merchant " << merchantId << " not found.\n";
            return false;
        }

        string monthKey = date.substr(0, 7); // "YYYY-MM"

        // Gather day's transactions
        auto allTxns = store_.getTransactionsForMerchant(merchantId);

        double grossSales = 0.0;
        double sameBatchRefunds = 0.0;

        vector<string> saleTxnIds;
        vector<string> sameBatchRefundIds;

        for (const auto& t : allTxns) {
            if (t.date != date)
                continue;

            if (store_.isTxnSettled(t.id))
                continue;

            if (store_.isAdjustmentApplied(t.id))
                continue;

            if (t.type == TxnType::SALE) {
                grossSales += t.amount;
                saleTxnIds.push_back(t.id);
            }
            else if (t.type == TxnType::REFUND) {

                // Same-day refund referencing unsettled sale
                // → same-batch deduction
                if (!t.refTxnId.empty() &&
                    !store_.isTxnSettled(t.refTxnId)) {

                    sameBatchRefunds += t.amount;
                    sameBatchRefundIds.push_back(t.id);
                }
            }
        }

        // Cross-day unsettled refunds
        set<string> saleTxnIdSet(
            saleTxnIds.begin(),
            saleTxnIds.end()
        );

        for (const auto& t : allTxns) {
            if (t.date == date)
                continue;

            if (store_.isAdjustmentApplied(t.id))
                continue;

            if (t.type != TxnType::REFUND)
                continue;

            if (t.refTxnId.empty())
                continue;

            // Only net against sales being settled in this batch
            if (saleTxnIdSet.count(t.refTxnId) &&
                !store_.isTxnSettled(t.refTxnId)) {

                sameBatchRefunds += t.amount;
                sameBatchRefundIds.push_back(t.id);
            }
        }

        // Compute clawbacks:
        // refunds/chargebacks referencing SETTLED transactions
        double clawback = 0.0;
        vector<string> clawbackTxnIds;

        for (const auto& t : allTxns) {
            if (store_.isAdjustmentApplied(t.id))
                continue;

            if (t.type == TxnType::REFUND ||
                t.type == TxnType::CHARGEBACK) {

                if (!t.refTxnId.empty() &&
                    store_.isTxnSettled(t.refTxnId)) {

                    // This is a clawback
                    bool isSameBatch = false;

                    for (const auto& sid : sameBatchRefundIds) {
                        if (sid == t.id) {
                            isSameBatch = true;
                            break;
                        }
                    }

                    if (!isSameBatch) {
                        clawback += t.amount;
                        clawbackTxnIds.push_back(t.id);
                    }
                }
            }
        }

        // Add carry-forward balance
        double totalClawback =
            clawback + m->carryForwardBalance;

        // If no sales and no clawbacks, skip batch creation
        if (grossSales == 0.0 &&
            sameBatchRefunds == 0.0 &&
            totalClawback == 0.0) {

            return true;
        }

        // Net sales after same-batch refunds
        double netSales =
            grossSales - sameBatchRefunds;

        if (netSales < 0.0)
            netSales = 0.0;

        // Commission on net sales
        double prevVolume =
            m->monthlyVolume[monthKey];

        auto strategy =
            CommissionPlanFactory::create(m->planType);

        double commission = 0.0;

        if (netSales > 0.0) {
            commission =
                strategy->calculate(
                    netSales,
                    prevVolume,
                    *m
                );
        }

        // Update cumulative monthly volume
        m->monthlyVolume[monthKey] =
            prevVolume + netSales;

        // Net payable
        double netPayable =
            netSales - commission - totalClawback;

        // Handle negative payable → carry forward
        double newCarryForward = 0.0;

        if (netPayable < 0.0) {
            newCarryForward = -netPayable;
            netPayable = 0.0;
        }

        m->carryForwardBalance =
            newCarryForward;

        // Create batch
        string batchId =
            "B" + to_string(++batchCounter_);

        Batch batch;

        batch.id = batchId;
        batch.merchantId = merchantId;
        batch.date = date;
        batch.grossSales = grossSales;
        batch.totalRefunds = sameBatchRefunds;
        batch.totalChargebacks = 0.0;
        batch.commission = commission;
        batch.clawbackApplied = totalClawback;
        batch.netPayable = netPayable;
        batch.settled = true;
        batch.disputed = false;

        // Separate chargebacks vs refunds in batch reporting
        for (const auto& tid : clawbackTxnIds) {
            Transaction* ct =
                store_.getTransaction(tid);

            if (ct && ct->type == TxnType::CHARGEBACK) {
                batch.totalChargebacks += ct->amount;
            }
            else if (ct && ct->type == TxnType::REFUND) {
                batch.totalRefunds += ct->amount;
            }
        }

        store_.addBatch(batch);

        // Mark sale transactions as settled
        for (const auto& sid : saleTxnIds) {
            store_.markTxnSettled(sid);

            audit_.log(
                "TRANSACTION",
                sid,
                "SETTLED",
                "batch=" + batchId
            );
        }

        // Mark same-batch refunds as applied
        for (const auto& rid : sameBatchRefundIds) {
            store_.markAdjustmentApplied(rid);

            audit_.log(
                "TRANSACTION",
                rid,
                "APPLIED_SAME_BATCH",
                "batch=" + batchId
            );
        }

        // Mark clawback transactions as applied
        for (const auto& cid : clawbackTxnIds) {
            store_.markAdjustmentApplied(cid);

            audit_.log(
                "TRANSACTION",
                cid,
                "CLAWBACK_APPLIED",
                "batch=" + batchId
            );
        }

        // Audit the batch
        audit_.log(
            "BATCH",
            batchId,
            "SETTLED",
            "merchant=" + merchantId +
            " date=" + date +
            " gross=" + fmtAmt(grossSales) +
            " commission=" + fmtAmt(commission) +
            " clawback=" + fmtAmt(totalClawback) +
            " net=" + fmtAmt(netPayable)
        );

        if (newCarryForward > 0.0) {
            audit_.log(
                "MERCHANT",
                merchantId,
                "CARRY_FORWARD",
                "$" + fmtAmt(newCarryForward) +
                " deferred to next batch"
            );
        }

        return true;
    }

    // ── Flag a batch as disputed ──
    bool flagDispute(const string& batchId) {
        Batch* b = store_.getBatch(batchId);

        if (!b) {
            cerr << "ERROR: Batch " << batchId
                 << " not found.\n";
            return false;
        }

        b->disputed = true;

        audit_.log(
            "BATCH",
            batchId,
            "DISPUTED",
            "Flagged for dispute review"
        );

        return true;
    }

    // ── Query net payable over date range ──
    double queryNetPayable(
        const string& merchantId,
        const string& startDate,
        const string& endDate
    ) const {

        auto batches =
            store_.getBatchesForMerchant(merchantId);

        double total = 0.0;

        for (const auto& b : batches) {
            if (b.date >= startDate &&
                b.date <= endDate) {

                total += b.netPayable;
            }
        }

        return total;
    }

    // ── Get audit trail ──
    vector<AuditEntry> getAuditTrail(
        const string& entityId
    ) const {

        return audit_.getTrail(entityId);
    }

    // ── Accessors for testing ──
    Batch* getBatch(const string& id) {
        return store_.getBatch(id);
    }

    Merchant* getMerchant(const string& id) {
        return store_.getMerchant(id);
    }

    const InMemoryDataStore& store() const {
        return store_;
    }

    // ── Print batch details ──
    void printBatchDetails(const Batch& b) const {
        cout << "  Batch: " << b.id
             << " | Date: " << b.date
             << " | Gross: $" << fmtAmt(b.grossSales)
             << " | Refunds: $" << fmtAmt(b.totalRefunds)
             << " | Chargebacks: $" << fmtAmt(b.totalChargebacks)
             << " | Commission: $" << fmtAmt(b.commission)
             << " | Clawback: $" << fmtAmt(b.clawbackApplied)
             << " | Net: $" << fmtAmt(b.netPayable)
             << " | Disputed: "
             << (b.disputed ? "YES" : "NO")
             << "\n";
    }

    void printAuditTrail(const string& entityId) const {
        auto trail =
            audit_.getTrail(entityId);

        if (trail.empty()) {
            cout << "  No audit trail for "
                 << entityId << "\n";
            return;
        }

        for (const auto& e : trail) {
            cout << "  [" << e.timestamp << "] "
                 << e.entityType << "/"
                 << e.entityId
                 << " -> " << e.event
                 << ": " << e.details
                 << "\n";
        }
    }

private:
    InMemoryDataStore store_;
    AuditManager audit_;
    int batchCounter_ = 0;

    static string fmtAmt(double v) {
        ostringstream oss;
        oss.precision(2);
        oss << fixed << v;
        return oss.str();
    }

    static string txnTypeName(TxnType t) {
        switch (t) {
            case TxnType::SALE:
                return "SALE";

            case TxnType::REFUND:
                return "REFUND";

            case TxnType::CHARGEBACK:
                return "CHARGEBACK";
        }

        return "UNKNOWN";
    }

    static string planName(CommissionPlanType p) {
        switch (p) {
            case CommissionPlanType::FLAT_PERCENTAGE:
                return "FLAT_PERCENTAGE";

            case CommissionPlanType::TIERED_BY_VOLUME:
                return "TIERED_BY_VOLUME";

            case CommissionPlanType::FLAT_FEE_PLUS_PERCENTAGE:
                return "FLAT_FEE_PLUS_PERCENTAGE";
        }

        return "UNKNOWN";
    }
};
