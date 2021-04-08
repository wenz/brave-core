/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_

#include "brave/components/brave_wallet/browser/hd_key.h"

#include <memory>
#include <string>
#include <vector>

namespace brave_wallet {

class EthTransaction;

class HDKeyring {
 public:
  explicit HDKeyring(const std::string& hd_path);
  virtual ~HDKeyring();

  // seed is optional, ex. for hardware wallet, we will generate root key
  // directly from the key produced in hardware
  virtual void ConstructRootHDKey(const std::vector<uint8_t>& seed);

  virtual void AddAccounts(size_t number = 1);
  virtual std::vector<std::string> GetAccounts();
  virtual void RemoveAccount(const std::string& address);

  // Bitcoin keyring can override this for different address calculation
  virtual std::string GetAddress(size_t index);

  // TODO(darkdh): Abstract Transacation class
  virtual void SignTransaction(const std::string& address, EthTransaction* tx);
  virtual std::vector<uint8_t> SignMessage(const std::string& address,
                                           const std::vector<uint8_t>& message);

 protected:
  std::string hd_path_;
  std::unique_ptr<HDKey> root_;
  std::unique_ptr<HDKey> master_key_;
  std::vector<std::unique_ptr<HDKey>> accounts_;

 private:
  HDKey* GetHDKeyFromAddress(const std::string& address);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_HD_KEYRING_H_
