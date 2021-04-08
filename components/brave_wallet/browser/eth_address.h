/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_ADDRESS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_ADDRESS_H_

#include <string>
#include <vector>

namespace brave_wallet {

class EthAddress {
 public:
  static EthAddress FromPublicKey(const std::vector<uint8_t>& public_key);
  // input should be a valid address with 20 bytes hex representation starting
  // with 0x
  static EthAddress FromHex(const std::string& input);
  EthAddress(const EthAddress& other);
  ~EthAddress();

  bool IsEmpty() const { return bytes_.empty(); }
  std::vector<uint8_t> bytes() const { return bytes_; }

  std::string ToHex() const;
  // EIP55 + EIP1191
  std::string ToChecksumAddress(uint8_t eip1191_chaincode) const;

 private:
  EthAddress();
  explicit EthAddress(const std::vector<uint8_t>& bytes);

  std::vector<uint8_t> bytes_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_ADDRESS_H_
