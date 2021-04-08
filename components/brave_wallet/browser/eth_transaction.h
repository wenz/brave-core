/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TRANSACTION_H_

#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "brave/components/brave_wallet/browser/eth_address.h"

namespace brave_wallet {
FORWARD_DECLARE_TEST(EthTransactionTest, GetSignedTransaction);

class EthTransaction {
 public:
  EthTransaction(const uint256_t& nonce,
                 const uint256_t& gas_price,
                 const uint256_t& gas_limit,
                 const EthAddress& to,
                 const uint256_t& value,
                 const std::vector<uint8_t> data);
  ~EthTransaction();

  uint256_t nonce() const { return nonce_; }
  uint256_t gas_price() const { return gas_price_; }
  uint256_t gas_limit() const { return gas_limit_; }
  EthAddress to() const { return to_; }
  uint256_t value() const { return value_; }
  std::vector<uint8_t> data() const { return data_; }

  // return
  // keccack(rlp([nonce, gasPrice, gasLimit, to, value, data, chainID, 0, 0])
  // Support EIP-155 chain id
  std::vector<uint8_t> GetMessageToSign(uint64_t chain_id = 0) const;

  // return rlp([nonce, gasPrice, gasLimit, to, value, data, v, r, s])
  std::string GetSignedTransaction() const;

  // signature and recid will be used to produce v, r, s
  // Support EIP-155 chain id
  void ProcessSignature(const std::vector<uint8_t> signature,
                        int recid,
                        uint64_t chain_id = 0);

 private:
  FRIEND_TEST_ALL_PREFIXES(EthTransactionUnitTest, GetSignedTransaction);

  uint256_t nonce_;
  uint256_t gas_price_;
  uint256_t gas_limit_;
  EthAddress to_;
  uint256_t value_;
  std::vector<uint8_t> data_;

  uint8_t v_;
  std::vector<uint8_t> r_;
  std::vector<uint8_t> s_;
};

}  // namespace brave_wallet
#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TRANSACTION_H_
