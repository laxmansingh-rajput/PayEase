# README: PayEase Settlement & Reconciliation Engine

**Overview**
A command-line based Merchant Settlement & Reconciliation Engine designed to process daily transactions, calculate dynamic commissions, and manage asynchronous chargebacks[span_10](start_span)[span_10](end_span). 

**How to Build and Run**
*(C++17 Environment with g++ / Clang)*
1. From the repository root or `/src`:
   ```bash
   make
   ```
2. Run the interactive CLI:
   ```bash
   ./payease
   ```
3. Run the automated test suite:
   ```bash
   make test
   # Or directly:
   ./payease --test
   ```

**Business Rule Assumptions**
To address explicitly under-specified edge cases[span_12](start_span)[span_12](end_span), the following rules are enforced:
*   **Carry-Forward Balances:** If a chargeback exceeds a merchant's next batch payable amount[span_13](start_span)[span_13](end_span), the batch net payable is set to `$0.00`. The remaining negative balance is stored on the Merchant profile and automatically docked from subsequent daily batches until cleared.
*   **Marginal Tiering:** When a merchant crosses a volume tier boundary mid-month[span_14](start_span)[span_14](end_span), the commission is calculated marginally. The volume up to the threshold uses the prior tier's rate, and the overflow uses the new tier's rate.
*   **Zero-Transaction Settlements:** If a batch is settled on a day with zero new sales but pending clawbacks exist[span_15](start_span)[span_15](end_span), a valid batch is still generated to formally record the carry-forward debt.
*   **Unsettled Transaction Refunds:** Refunds arriving for transactions that have not yet been settled are applied immediately to that same active batch[span_16](start_span)[span_16](end_span).
*   **Invalid Refunds:** Refunds referencing a non-existent transaction ID are rejected at the entry point to prevent data corruption[span_17](start_span)[span_17](end_span).

**Design Patterns Applied**
*   **Strategy Pattern:** Used for calculating commissions[span_18](start_span)[span_18](end_span). `FlatPercentageStrategy`, `TieredVolumeStrategy`, and `FlatFeePlusPercentageStrategy` implement a common interface, satisfying the requirement for polymorphism[span_19](start_span)[span_19](end_span).
*   **Factory Method:** A `CommissionPlanFactory` isolates the instantiation logic for various merchant commission plans.
*   **Audit Log Pattern:** Instead of destructively modifying transaction states, the system appends immutable event records (e.g., "RECORDED", "SETTLED") to a centralized `AuditManager` to guarantee an accurate transaction trail[span_20](start_span)[span_20](end_span).
*   **Repository Pattern:** An `InMemoryDataStore` manages state for Merchants, Transactions, and Batches to prevent the engine from becoming monolithic[span_21](start_span)[span_21](end_span).
