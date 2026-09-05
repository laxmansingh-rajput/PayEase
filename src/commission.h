#pragma once

#include "models.h"
#include <memory>

// ─── Strategy Interface ──────────────────────────────────────────────────────

class ICommissionStrategy {
public:
    virtual ~ICommissionStrategy() = default;
    // Calculate commission for a given sale volume on this batch.
    // prevVolume = cumulative monthly volume BEFORE this batch's sales.
    virtual double calculate(double saleVolume, double prevVolume, const Merchant& m) const = 0;
};

// ─── Flat Percentage ─────────────────────────────────────────────────────────

class FlatPercentageStrategy : public ICommissionStrategy {
public:
    double calculate(double saleVolume, double /*prevVolume*/, const Merchant& m) const override {
        return saleVolume * m.flatRate;
    }
};

// ─── Tiered-by-Volume (Marginal) ─────────────────────────────────────────────

class TieredVolumeStrategy : public ICommissionStrategy {
public:
    double calculate(double saleVolume, double prevVolume, const Merchant& m) const override {
        // Marginal tiering: commission is calculated on the cumulative volume
        // range [prevVolume, prevVolume + saleVolume], applying the appropriate
        // tier rate to each slice.
        double commission = 0.0;
        double remaining = saleVolume;
        double cursor = prevVolume;

        for (const auto& tier : m.tiers) {
            if (remaining <= 0.0) break;

            double tierCeiling = (tier.upperLimit < 0) ? 1e18 : tier.upperLimit;

            if (cursor >= tierCeiling) continue; // already past this tier

            double sliceTop = std::min(cursor + remaining, tierCeiling);
            double slice = sliceTop - cursor;

            commission += slice * tier.rate;
            remaining -= slice;
            cursor = sliceTop;
        }
        return commission;
    }
};

// ─── Flat Fee + Percentage ───────────────────────────────────────────────────

class FlatFeePlusPercentageStrategy : public ICommissionStrategy {
public:
    double calculate(double saleVolume, double /*prevVolume*/, const Merchant& m) const override {
        return m.flatFee + (saleVolume * m.feeRate);
    }
};

// ─── Factory ─────────────────────────────────────────────────────────────────

class CommissionPlanFactory {
public:
    static std::unique_ptr<ICommissionStrategy> create(CommissionPlanType type) {
        switch (type) {
            case CommissionPlanType::FLAT_PERCENTAGE:
                return std::make_unique<FlatPercentageStrategy>();
            case CommissionPlanType::TIERED_BY_VOLUME:
                return std::make_unique<TieredVolumeStrategy>();
            case CommissionPlanType::FLAT_FEE_PLUS_PERCENTAGE:
                return std::make_unique<FlatFeePlusPercentageStrategy>();
        }
        return nullptr;
    }
};
