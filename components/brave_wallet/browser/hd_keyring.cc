/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/hd_keyring.h"

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"

namespace brave_wallet {

HDKeyring::HDKeyring(const std::string& hd_path) : hd_path_(hd_path) {}
HDKeyring::~HDKeyring() = default;

void HDKeyring::ConstructRootHDKey(const std::vector<uint8_t>& seed) {
  if (!seed.empty()) {
    master_key_ = HDKey::GenerateFromSeed(seed);
    root_ = master_key_->DeriveChildFromPath(hd_path_);
  }
}

void HDKeyring::AddAccounts(size_t number) {
  size_t cur_accounts_number = accounts_.size();
  for (size_t i = cur_accounts_number; i < cur_accounts_number + number; ++i) {
    accounts_[i] = root_->DeriveChild(i);
  }
}

std::vector<std::string> HDKeyring::GetAccounts() {
  std::vector<std::string> addresses;
  for (size_t i = 0; i < accounts_.size(); ++i) {
    addresses.push_back(GetAddress(i));
  }
  return addresses;
}

std::string HDKeyring::GetAddress(size_t index) {
  if (accounts_.empty() || index < 0 || index >= accounts_.size())
    return std::string();
  const std::vector<uint8_t> public_key = accounts_[index]->public_key();
  EthAddress addr = EthAddress::FromPublicKey(public_key);

  return addr.ToHex();
}

void HDKeyring::SignTransaction(const std::string& address,
                                EthTransaction* tx) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key || !tx)
    return;

  // TODO(darkdh): chain id
  const std::vector<uint8_t> message = tx->GetMessageToSign();
  int recid;
  const std::vector<uint8_t> signature = hd_key->Sign(message, &recid);
  tx->ProcessSignature(signature, recid);
}

std::vector<uint8_t> HDKeyring::SignMessage(
    const std::string& address,
    const std::vector<uint8_t>& message) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key)
    return std::vector<uint8_t>();

  const std::string prefix = "\u0019Ethereum Signed Message:\n";
  std::vector<uint8_t> hash_input(prefix.begin(), prefix.end());
  hash_input.insert(hash_input.end(), message.begin(), message.end());
  const std::vector<uint8_t> hash = KeccakHash(hash_input);

  return hd_key->Sign(hash);
}

HDKey* HDKeyring::GetHDKeyFromAddress(const std::string& address) {
  for (size_t i = 0; i < accounts_.size(); ++i) {
    if (GetAddress(i) == address)
      return accounts_[i].get();
  }
  return nullptr;
}

}  // namespace brave_wallet
