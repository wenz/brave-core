/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/hd_key.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthTransactionUnitTest, GetMessageToSign) {
  std::vector<uint8_t> data;
  EXPECT_TRUE(base::HexStringToBytes(
      "00000000000000000000000000000000000000000000000000000000000000ad0000"
      "00000000000000000000000000000000000000000000000000000000fafa00000000"
      "00000000000000000000000000000000000000000000000000000dfa000000000000"
      "0000000000000000000000000000000000000000000000000dfa0000000000000000"
      "0000000000000000000000000000000000000000000000ad00000000000000000000"
      "0000000000000000000000000000000000000000000f000000000000000000000000"
      "000000000000000000000000000000000000000a0000000000000000000000000000"
      "0000000000000000000000000000000000df00000000000000000000000000000000"
      "0000000000000000000000000000000a000000000000000000000000000000000000"
      "00000000000000000000000000df0000000000000000000000000000000000000000"
      "00000000000000000000000a00000000000000000000000000000000000000000000"
      "0000000000000000000d",
      &data));
  EthTransaction tx1(
      0x06, 0x09184e72a000, 0x0974,
      EthAddress::FromHex("0xbe862ad9abfe6f22bcb087716c7d89a26051f74c"),
      0x016345785d8a0000, data);

  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx1.GetMessageToSign())),
            "61e1ec33764304dddb55348e7883d4437426f44ab3ef65e6da1e025734c03ff0");

  data.clear();
  EthTransaction tx2(
      0x0b, 0x051f4d5c00, 0x5208,
      EthAddress::FromHex("0x656e929d6fc0cac52d3d9526d288fe02dcd56fbd"),
      0x2386f26fc10000, data);

  // with chain id (mainnet)
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx2.GetMessageToSign(1))),
            "f97c73fdca079da7652dbc61a46cd5aeef804008e057be3e712c43eac389aaf0");

  // EIP 155 test vectors
  const struct {
    uint256_t nonce;
    uint256_t gas_price;
    uint256_t gas_limit;
    const char* to;
    uint256_t value;
    const char* hash;
  } cases[] = {
      {0x00, 0x04a817c800, 0x5208, "0x3535353535353535353535353535353535353535",
       0x00,
       "e0be81f8d506dbe3a5549e720b51eb79492378d6638087740824f168667e5239"},
      {0x08, 0x04a817c808, 0x02e248,
       "0x3535353535353535353535353535353535353535", 0x0200,
       "50b6e7b58320c885ab7b2ee0d0b5813a697268bd2494a06de792790b13668c08"},
      {0x09, 0x04a817c809, 0x033450,
       "0x3535353535353535353535353535353535353535", 0x02d9,
       "24fd18c70146a2b002254810473fa26b744f7899258a1f32924cc73e7a8f4d4f"},
      {0x01, 0x04a817c801, 0xa410, "0x3535353535353535353535353535353535353535",
       0x01,
       "42973b488dbb3aa237db3d1a3bad18a8d2148af795fb6cdbbbeef5c736df97b9"},
      {0x02, 0x04a817c802, 0xf618, "0x3535353535353535353535353535353535353535",
       0x08,
       "e68afed5d359c7e60a0408093da23c57b63e84acb2e368ac7c47630518d6f4d9"},
      {0x03, 0x04a817c803, 0x014820,
       "0x3535353535353535353535353535353535353535", 0x1b,
       "bcb6f653e06c276a080e9d68e5a967847a896cf52a6dc81917dc2c57ae0a31ef"},
      {0x04, 0x04a817c804, 0x019a28,
       "0x3535353535353535353535353535353535353535", 0x40,
       "244e4b57522352c3e9f93ad8ac8a47d1b46c3dcda6da2522caedad009ac9afb7"},
      {0x05, 0x04a817c805, 0x01ec30,
       "0x3535353535353535353535353535353535353535", 0x7d,
       "581c0b79498b1cf1b8fa4f69bc5f21c0c60371cd08d4682b15c4334aac1cccfd"},
      {0x06, 0x04a817c806, 0x023e38,
       "0x3535353535353535353535353535353535353535", 0xd8,
       "678ae2053a840f5fe550a63d724d1c85420d2955a0ccc4f868dd59e27afdf279"},
      {0x07, 0x04a817c807, 0x029040,
       "0x3535353535353535353535353535353535353535", 0x0157,
       "81aa03ada1474ff3ca4b86afb8e8c0f8b22791e156e706231a695ef8c51515ab"},
  };

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    EthTransaction tx(cases[i].nonce, cases[i].gas_price, cases[i].gas_limit,
                      EthAddress::FromHex(cases[i].to), cases[i].value,
                      std::vector<uint8_t>());
    // with chain id (mainnet)
    EXPECT_EQ(base::ToLowerASCII(base::HexEncode(tx.GetMessageToSign(1))),
              cases[i].hash);
  }
}

TEST(EthTransactionUnitTest, GetSignedTransaction) {
  std::vector<uint8_t> private_key;
  EXPECT_TRUE(base::HexStringToBytes(
      "4646464646464646464646464646464646464646464646464646464646464646",
      &private_key));

  HDKey key;
  key.SetPrivateKey(private_key);
  EthTransaction tx(
      0x09, 0x4a817c800, 0x5208,
      EthAddress::FromHex("0x3535353535353535353535353535353535353535"),
      0x0de0b6b3a7640000, std::vector<uint8_t>());
  const std::vector<uint8_t> message = tx.GetMessageToSign(1);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(message)),
            "daf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53");

  int recid;
  const std::vector<uint8_t> signature = key.Sign(message, &recid);

  // invalid
  tx.ProcessSignature(std::vector<uint8_t>(63), recid, 1);
  EXPECT_EQ(tx.v_, 0);
  EXPECT_TRUE(tx.r_.empty());
  EXPECT_TRUE(tx.s_.empty());
  tx.ProcessSignature(std::vector<uint8_t>(65), recid, 1);
  EXPECT_EQ(tx.v_, 0);
  EXPECT_TRUE(tx.r_.empty());
  EXPECT_TRUE(tx.s_.empty());
  tx.ProcessSignature(signature, -1, 1);
  EXPECT_EQ(tx.v_, 0);
  EXPECT_TRUE(tx.r_.empty());
  EXPECT_TRUE(tx.s_.empty());
  tx.ProcessSignature(signature, 4, 1);
  EXPECT_EQ(tx.v_, 0);
  EXPECT_TRUE(tx.r_.empty());
  EXPECT_TRUE(tx.s_.empty());

  tx.ProcessSignature(signature, recid, 1);
  EXPECT_EQ(tx.GetSignedTransaction(),
            "0xf86c098504a817c8008252089435353535353535353535353535353535353535"
            "35880de0b6b3a76400008025a028ef61340bd939bc2195fe537567866003e1a15d"
            "3c71ff63e1590620aa636276a067cbe9d8997f761aecb703304b3800ccf555c9f3"
            "dc64214b297fb1966a3b6d83");

  EXPECT_EQ(tx.v_, 37);
  // 18515461264373351373200002665853028612451056578545711640558177340181847433846
  EXPECT_EQ(base::HexEncode(tx.r_),
            "28EF61340BD939BC2195FE537567866003E1A15D3C71FF63E1590620AA636276");
  // 46948507304638947509940763649030358759909902576025900602547168820602576006531
  EXPECT_EQ(base::HexEncode(tx.s_),
            "67CBE9D8997F761AECB703304B3800CCF555C9F3DC64214B297FB1966A3B6D83");
}

}  // namespace brave_wallet
