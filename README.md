# PayEase — Fintech Merchant Settlement & Reconciliation Engine

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Design](https://img.shields.io/badge/Design-LLD%20OOP-orange.svg)]()
[![License](https://img.shields.io/badge/license-MIT-green.svg)]()

> A robust, high-performance C++17 Low-Level Design (LLD) prototype for automated daily merchant transaction settlements, dynamic multi-tier commission calculations, asynchronous refund/chargeback clawbacks, and immutable audit trails.

---

## 📑 Table of Contents

- [Overview](#overview)
- [System Architecture & Design Patterns](#system-architecture--design-patterns)
- [Key Features](#key-features)
- [Business Rules & Edge Cases](#business-rules--edge-cases)
- [Project Directory Structure](#project-directory-structure)
- [Building & Running](#building--running)
- [Interactive CLI Reference](#interactive-cli-reference)
- [Test Suite & Scenarios](#test-suite--scenarios)

---

## 💡 Overview

Payment processing platforms settle daily transaction proceeds to merchant partners after deducting processing commissions. In manual or naive settlement workflows, common points of failure include:
- Incorrect commission calculations across volume boundaries.
- Asynchronous refunds or chargebacks arriving after a batch has already been settled and paid out.
- Inconsistent debt tracking when clawbacks exceed the payout of the subsequent batch.
- Destructive data modifications that destroy auditability.

**PayEase** resolves these challenges through an in-memory, event-audited settlement engine that enforces strict financial invariants, supports polymorphic commission plans, gracefully rolls over debt via carry-forward balances, and maintains full transaction-to-batch traceability.

---

## 🏛️ System Architecture & Design Patterns

The system is decoupled using clean Object-Oriented Design (OOD) principles:

```
                  ┌──────────────────────┐
                  │    CLI / Test Runner │
                  └──────────┬───────────┘
                             │
                  ┌──────────▼───────────┐
                  │   SettlementEngine   │
                  └───────┬──────┬───────┘
          ┌───────────────┘      └────────────────┐
          ▼                                       ▼
┌───────────────────┐                   ┌───────────────────┐
│ InMemoryDataStore │                   │   AuditManager    │
├───────────────────┤                   ├───────────────────┤
│ • Merchants       │                   │ • Append-only Log │
│ • Transactions    │                   │ • Entity Trails   │
│ • Batches         │                   └───────────────────┘
└─────────┬─────────┘
          │
          ▼
┌───────────────────────────────────────┐
│ Commission Calculation                │
├───────────────────────────────────────┤
│ «Strategy» ICommissionStrategy        │
│   ├── FlatPercentageStrategy          │
│   ├── TieredVolumeStrategy (Marginal) │
│   └── FlatFeePlusPercentageStrategy   │
│                                       │
│ «Factory» CommissionPlanFactory       │
└───────────────────────────────────────┘
```

1. **Strategy Pattern (`ICommissionStrategy`)**:
   - Isolates commission calculation algorithms. Each plan (`FLAT_PERCENTAGE`, `TIERED_BY_VOLUME`, `FLAT_FEE_PLUS_PERCENTAGE`) implements `calculate()`.
2. **Factory Method Pattern (`CommissionPlanFactory`)**:
   - Decouples strategy instantiation from the merchant configuration.
3. **Repository Pattern (`InMemoryDataStore`)**:
   - Centralizes state management for Merchants, Transactions, and Settlement Batches without polluting the core engine logic.
4. **Audit Log Pattern (`AuditManager`)**:
   - Appends immutable event logs (e.g., `RECORDED`, `SETTLED`, `CLAWBACK`, `DISPUTED`) ensuring complete, tamper-evident financial compliance.

---

## 🚀 Key Features

- **Dynamic Commission Plans**:
  - **Flat Percentage**: Consistent rate on gross sales (e.g., 2%).
  - **Tiered by Volume**: Dynamic commission calculated based on cumulative monthly volume with marginal tier brackets.
  - **Flat Fee + Percentage**: Fixed per-batch fee plus a percentage on gross sales.
- **Asynchronous Clawback Handling**:
  - Automatically captures refunds and chargebacks referencing previous, already-settled batches and deducts them from the merchant's next open settlement batch without altering finalized records.
- **Carry-Forward Balances**:
  - If clawbacks exceed a batch's gross sales minus commissions, the net payout floors at `$0.00` and the remaining negative debt is safely carried forward to subsequent settlement cycles.
- **Dispute Flagging**:
  - Batches can be flagged as disputed without altering underlying financial figures or audit trails.
- **Range Inquiries & Reporting**:
  - Query aggregate net payable amounts for any merchant over custom date windows.
- **Comprehensive Audit Trail**:
  - Query entity-specific chronological logs for merchants, transactions, and settlement batches.

---

## ⚖️ Business Rules & Edge Cases

| Rule / Edge Case | System Behavior |
| :--- | :--- |
| **Marginal Tiering** | When a merchant crosses a volume tier boundary mid-month, sales up to the threshold use the previous tier's rate, and overflow volume is charged at the next tier's rate. |
| **Carry-Forward Balances** | When clawbacks > gross proceeds, `batch.netPayable = 0.00`. Remaining deficit is stored in `Merchant.carryForwardBalance` and docked from subsequent batches until cleared. |
| **Zero-Transaction Settlements** | Settling a day with zero sales but pending clawbacks still yields a valid batch, formally logging the debt and updating carry-forwards. |
| **Unsettled Refunds** | Refunds targeting sales in the current (unsettled) batch are directly applied to reduce the gross payable in the current batch. |
| **Invalid Refund Rejection** | Refunds referencing non-existent transaction IDs are rejected at intake to prevent corruption. |

---

## 📁 Project Directory Structure

```
PayEase/
├── .gitignore               # Multi-platform build & editor ignore rules
├── Makefile                 # Root-level build and test automation
├── Problem Statement.md     # Fintech requirements & problem definition
├── Solution.md              # Technical design & architecture documentation
├── README.md                # Comprehensive project documentation
└── src/
    ├── Makefile             # Source-level Makefile
    ├── audit.h              # AuditEntry & AuditManager event-logging
    ├── commission.h         # Commission strategies & factory implementation
    ├── engine.h             # Core SettlementEngine reconciliation logic
    ├── models.h             # Data entities (Transaction, Merchant, Batch)
    ├── store.h              # InMemoryDataStore repository
    ├── tests.h              # Test suite declarations
    ├── tests.cpp            # Test cases & scenario verifications
    ├── main.cpp             # Interactive CLI & entry point
    └── bits/
        └── stdc++.h         # Cross-platform header support
```

---

## 🛠️ Building & Running

### Prerequisites
- Modern C++ compiler with C++17 support:
  - GCC (`g++` 7.0+)
  - Clang (`clang++` 6.0+)
  - MSVC 2017+
- `make` (optional, for automated builds)

### Compilation

Using `make`:
```bash
make
```

Or manually with `g++`:
```bash
g++ -std=c++17 -Wall -Wextra -O2 -Isrc src/main.cpp src/tests.cpp -o payease
```

### Running Tests
Execute the built-in automated test suite:
```bash
make test
# or directly:
./payease --test
```

### Running Interactive CLI
```bash
./payease
```

---

## 💻 Interactive CLI Reference

| Command | Syntax | Description |
| :--- | :--- | :--- |
| `help` | `help` | Display command syntax and help menu |
| `run_tests` | `run_tests` | Run all automated test suites |
| `register` | `register <id> <name> <planType> [params]` | Register a new merchant (`flat`, `tiered`, or `flatfee`) |
| `sale` | `sale <txnId> <merchantId> <amount> <date>` | Record a new sale transaction |
| `refund` | `refund <txnId> <merchantId> <amount> <date> <refTxnId>` | Record a refund linked to a sale |
| `chargeback` | `chargeback <txnId> <merchantId> <amount> <date> <refTxnId>` | Record a chargeback linked to a sale |
| `settle` | `settle <merchantId> <date>` | Settle transactions for a merchant on given date |
| `dispute` | `dispute <batchId>` | Mark a settlement batch as disputed |
| `query` | `query <merchantId> <startDate> <endDate>` | Calculate aggregate net payable over date range |
| `audit` | `audit <entityId>` | Print complete chronological audit trail |
| `quit` | `quit` or `exit` | Exit the CLI |

### Example CLI Workflow
```text
payease> register M1 "Acme Corp" flat 0.02
Merchant M1 registered.

payease> sale TXN1 M1 1000.00 2026-09-01
Sale TXN1 recorded.

payease> settle M1 2026-09-01
Batch settled for M1 on 2026-09-01.

payease> audit M1
--- Audit Trail for Entity: M1 ---
[2026-09-01] [MERCHANT:M1] REGISTERED: Merchant Acme Corp registered with plan: 0
[2026-09-01] [BATCH:M1_2026-09-01] SETTLED: Gross: $1000.00, Comm: $20.00, Clawback: $0.00, Net: $980.00
```

---

## 🧪 Test Suite & Scenarios

The test suite in [`tests.cpp`](src/tests.cpp) covers 10 critical validation scenarios:

1. **Flat Percentage Settlement**: Verifies single and multi-sale calculations with 2% flat rate.
2. **Tiered Volume Strategy (Marginal)**: Verifies volume threshold transitions within a calendar month.
3. **Flat Fee + Percentage Settlement**: Verifies fixed batch charge added to percentage commission.
4. **Intra-day Refund Handling**: Verifies deduction of refund before daily batch finalization.
5. **Asynchronous Clawback on Settled Batch**: Verifies refund arriving on day 2 against day 1 batch correctly deducted.
6. **Chargeback with Negative Carry-Forward**: Verifies that when clawback exceeds gross sales, net payable is `$0.00` and deficit is carried forward.
7. **Zero-Transaction Settlement**: Verifies settlement with only pending clawbacks and no new sales.
8. **Dispute Flagging**: Verifies batch disputed status update while preserving financial values.
9. **Date-Range Net Payable Query**: Aggregates net payables across multiple batches within a date range.
10. **Audit Trail Completeness**: Verifies event logging across all lifecycle actions.

---

## 📄 License
This project is licensed under the [MIT License](LICENSE).
