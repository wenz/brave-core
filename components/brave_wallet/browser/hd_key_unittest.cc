/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/hd_key.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
bool IsPublicKeyEmpty(const std::vector<uint8_t> public_key) {
  for (const uint8_t& byte : public_key) {
    if (byte != 0x00)
      return false;
  }
  return true;
}
}  // namespace

TEST(HDKeyUnitTest, GenerateFromSeed) {
  for (size_t i = 16; i <= 64; ++i) {
    EXPECT_NE(HDKey::GenerateFromSeed(std::vector<uint8_t>(i)), nullptr);
  }
  EXPECT_EQ(HDKey::GenerateFromSeed(std::vector<uint8_t>(15)), nullptr);
  EXPECT_EQ(HDKey::GenerateFromSeed(std::vector<uint8_t>(65)), nullptr);
}

TEST(HDKeyUnitTest, TestVector1) {
  const struct {
    const char* path;
    const char* ext_pub;
    const char* ext_pri;
  } cases[] = {
      {"m",
       "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ2"
       "9ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8",
       "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChk"
       "VvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi"},
      {"m/0'",
       "xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LH"
       "hwBZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw",
       "xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ngLNv1TxvUxt4cV1"
       "rGL5hj6KCesnDYUhd7oWgT11eZG7XnxHrnYeSvkzY7d2bhkJ7"},
      {"m/0'/1",
       "xpub6ASuArnXKPbfEwhqN6e3mwBcDTgzisQN1wXN9BJcM47sSikHjJf3UFHKkN"
       "AWbWMiGj7Wf5uMash7SyYq527Hqck2AxYysAA7xmALppuCkwQ",
       "xprv9wTYmMFdV23N2TdNG573QoEsfRrWKQgWeibmLntzniatZvR9BmLnvSxqu5"
       "3Kw1UmYPxLgboyZQaXwTCg8MSY3H2EU4pWcQDnRnrVA1xe8fs"},
      {"m/0'/1/2'",
       "xpub6D4BDPcP2GT577Vvch3R8wDkScZWzQzMMUm3PWbmWvVJrZwQY4VUNgqFJP"
       "MM3No2dFDFGTsxxpG5uJh7n7epu4trkrX7x7DogT5Uv6fcLW5",
       "xprv9z4pot5VBttmtdRTWfWQmoH1taj2axGVzFqSb8C9xaxKymcFzXBDptWmT7"
       "FwuEzG3ryjH4ktypQSAewRiNMjANTtpgP4mLTj34bhnZX7UiM"},
      {"m/0'/1/2'/2",
       "xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyi"
       "LjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV",
       "xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8R"
       "fQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334"},
      {"m/0'/1/2'/2/1000000000",
       "xpub6H1LXWLaKsWFhvm6RVpEL9P4KfRZSW7abD2ttkWP3SSQvnyA8FSVqNTEcY"
       "FgJS2UaFcxupHiYkro49S8yGasTvXEYBVPamhGW6cFJodrTHy",
       "xprvA41z7zogVVwxVSgdKUHDy1SKmdb533PjDz7J6N6mV6uS3ze1ai8FHa8kmH"
       "ScGpWmj4WggLyQjgPie1rFSruoUihUZREPSL39UNdE3BBDu76"},
  };

  std::vector<uint8_t> bytes;
  // path: m
  EXPECT_TRUE(
      base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &bytes));

  std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(bytes);

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    std::unique_ptr<HDKey> key = m_key->DeriveChildFromPath(cases[i].path);
    EXPECT_EQ(key->GetPublicExtendedKey(), cases[i].ext_pub);
    EXPECT_EQ(key->GetPrivateExtendedKey(), cases[i].ext_pri);
  }
}

TEST(HDKeyUnitTest, TestVector2) {
  const struct {
    const char* path;
    const char* ext_pub;
    const char* ext_pri;
  } cases[] = {
      {"m",
       "xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMS"
       "gv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB",
       "xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4"
       "PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U"},
      {"m/0",
       "xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfv"
       "rnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH",
       "xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJ"
       "D9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt"},
      {"m/0/2147483647'",
       "xpub6ASAVgeehLbnwdqV6UKMHVzgqAG8Gr6riv3Fxxpj8ksbH9ebxaEyBLZ85ySDhKiLDBr"
       "QSARLq1uNRts8RuJiHjaDMBU4Zn9h8LZNnBC5y4a",
       "xprv9wSp6B7kry3Vj9m1zSnLvN3xH8RdsPP1Mh7fAaR7aRLcQMKTR2vidYEeEg2mUCTAwCd"
       "6vnxVrcjfy2kRgVsFawNzmjuHc2YmYRmagcEPdU9"},
      {"m/0/2147483647'/1",
       "xpub6DF8uhdarytz3FWdA8TvFSvvAh8dP3283MY7p2V4SeE2wyWmG5mg5EwVvmdMVCQcoNJ"
       "xGoWaU9DCWh89LojfZ537wTfunKau47EL2dhHKon",
       "xprv9zFnWC6h2cLgpmSA46vutJzBcfJ8yaJGg8cX1e5StJh45BBciYTRXSd25UEPVuesF9y"
       "og62tGAQtHjXajPPdbRCHuWS6T8XA2ECKADdw4Ef"},
      {"m/0/2147483647'/1/2147483646'",
       "xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4kox"
       "b5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL",
       "xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njG"
       "VyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc"},
      {"m/0/2147483647'/1/2147483646'/2",
       "xpub6FnCn6nSzZAw5Tw7cgR9bi15UV96gLZhjDstkXXxvCLsUXBGXPdSnLFbdpq8p9HmGsA"
       "pME5hQTZ3emM2rnY5agb9rXpVGyy3bdW6EEgAtqt",
       "xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38"
       "EGfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j"},
  };
  std::vector<uint8_t> bytes;
  // path: m
  EXPECT_TRUE(base::HexStringToBytes(
      "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c9996"
      "93908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542",
      &bytes));

  std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(bytes);
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    std::unique_ptr<HDKey> key = m_key->DeriveChildFromPath(cases[i].path);
    EXPECT_EQ(key->GetPublicExtendedKey(), cases[i].ext_pub);
    EXPECT_EQ(key->GetPrivateExtendedKey(), cases[i].ext_pri);
  }
}

TEST(HDKeyUnitTest, TestVector3) {
  const struct {
    const char* path;
    const char* ext_pub;
    const char* ext_pri;
  } cases[] = {
      {"m",
       "xpub661MyMwAqRbcEZVB4dScxMAdx6d4nFc9nvyvH3v4gJL378CSRZiYmhRoP7mBy6gSPSC"
       "Yk6SzXPTf3ND1cZAceL7SfJ1Z3GC8vBgp2epUt13",
       "xprv9s21ZrQH143K25QhxbucbDDuQ4naNntJRi4KUfWT7xo4EKsHt2QJDu7KXp1A3u7Bi1j"
       "8ph3EGsZ9Xvz9dGuVrtHHs7pXeTzjuxBrCmmhgC6"},
      {"m/0'",
       "xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaoh"
       "PX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y",
       "xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AAN"
       "Yqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L"},
  };
  std::vector<uint8_t> bytes;
  // path: m
  EXPECT_TRUE(base::HexStringToBytes(
      "4b381541583be4423346c643850da4b320e46a87ae3d2a4e6da11eba819cd4acba45d239"
      "319ac14f863b8d5ab5a0d0c64d2e8a1e7d1457df2e5a3c51c73235be",
      &bytes));

  std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(bytes);
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    std::unique_ptr<HDKey> key = m_key->DeriveChildFromPath(cases[i].path);
    EXPECT_EQ(key->GetPublicExtendedKey(), cases[i].ext_pub);
    EXPECT_EQ(key->GetPrivateExtendedKey(), cases[i].ext_pri);
  }
}

TEST(HDKeyUnitTest, GenerateFromExtendedKey) {
  // m/0/2147483647'/1/2147483646'/2
  std::unique_ptr<HDKey> hdkey_from_pri = HDKey::GenerateFromExtendedKey(
      "xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38E"
      "GfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j");
  EXPECT_EQ(hdkey_from_pri->depth_, 5u);
  EXPECT_EQ(hdkey_from_pri->parent_fingerprint_, 0x31a507b8u);
  EXPECT_EQ(hdkey_from_pri->index_, 2u);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pri->chain_code_)),
            "9452b549be8cea3ecb7a84bec10dcfd94afe4d129ebfd3b3cb58eedf394ed271");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pri->private_key_)),
            "bb7d39bdb83ecf58f2fd82b6d918341cbef428661ef01ab97c28a4842125ac23");
  EXPECT_EQ(
      base::ToLowerASCII(base::HexEncode(hdkey_from_pri->public_key_)),
      "024d902e1a2fc7a8755ab5b694c575fce742c48d9ff192e63df5193e4c7afe1f9c");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pri->identifier_)),
            "26132fdbe7bf89cbc64cf8dafa3f9f88b8666220");

  // m/0/2147483647'/1/2147483646'/2
  std::unique_ptr<HDKey> hdkey_from_pub = HDKey::GenerateFromExtendedKey(
      "xpub6FnCn6nSzZAw5Tw7cgR9bi15UV96gLZhjDstkXXxvCLsUXBGXPdSnLFbdpq8p9HmGsAp"
      "ME5hQTZ3emM2rnY5agb9rXpVGyy3bdW6EEgAtqt");
  EXPECT_EQ(hdkey_from_pub->depth_, 5u);
  EXPECT_EQ(hdkey_from_pub->parent_fingerprint_, 0x31a507b8u);
  EXPECT_EQ(hdkey_from_pub->index_, 2u);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pub->chain_code_)),
            "9452b549be8cea3ecb7a84bec10dcfd94afe4d129ebfd3b3cb58eedf394ed271");
  EXPECT_TRUE(hdkey_from_pub->private_key_.empty());
  EXPECT_EQ(
      base::ToLowerASCII(base::HexEncode(hdkey_from_pub->public_key_)),
      "024d902e1a2fc7a8755ab5b694c575fce742c48d9ff192e63df5193e4c7afe1f9c");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pub->identifier_)),
            "26132fdbe7bf89cbc64cf8dafa3f9f88b8666220");
}

TEST(HDKeyUnitTest, SignAndVerifyAndRecover) {
  std::unique_ptr<HDKey> key = HDKey::GenerateFromExtendedKey(
      "xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38E"
      "GfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j");

  const std::vector<uint8_t> msg_a(32, 0x00);
  const std::vector<uint8_t> msg_b(32, 0x08);
  int recid_a = -1;
  int recid_b = -1;
  const std::vector<uint8_t> sig_a = key->Sign(msg_a, &recid_a);
  const std::vector<uint8_t> sig_b = key->Sign(msg_b, &recid_b);
  EXPECT_NE(recid_a, -1);
  EXPECT_NE(recid_b, -1);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig_a)),
            "6ba4e554457ce5c1f1d7dbd10459465e39219eb9084ee23270688cbe0d49b52b79"
            "05d5beb28492be439a3250e9359e0390f844321b65f1a88ce07960dd85da06");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig_b)),
            "dfae85d39b73c9d143403ce472f7c4c8a5032c13d9546030044050e7d39355e47a"
            "532e5c0ae2a25392d97f5e55ab1288ef1e08d5c034bad3b0956fbbab73b381");
  EXPECT_TRUE(key->Verify(msg_a, sig_a));
  EXPECT_TRUE(key->Verify(msg_b, sig_b));
  const std::vector<uint8_t> public_key_a = key->Recover(msg_a, sig_a, recid_a);
  const std::vector<uint8_t> public_key_b = key->Recover(msg_b, sig_b, recid_b);
  EXPECT_EQ(base::HexEncode(public_key_a), base::HexEncode(key->public_key()));
  EXPECT_EQ(base::HexEncode(public_key_b), base::HexEncode(key->public_key()));

  EXPECT_FALSE(key->Verify(std::vector<uint8_t>(32), std::vector<uint8_t>(64)));
  EXPECT_FALSE(key->Verify(msg_a, sig_b));
  EXPECT_FALSE(key->Verify(msg_b, sig_a));

  EXPECT_FALSE(key->Verify(std::vector<uint8_t>(31), sig_a));
  EXPECT_FALSE(key->Verify(std::vector<uint8_t>(33), sig_a));

  EXPECT_FALSE(key->Verify(msg_a, std::vector<uint8_t>(63)));
  EXPECT_FALSE(key->Verify(msg_a, std::vector<uint8_t>(65)));

  EXPECT_TRUE(
      IsPublicKeyEmpty(key->Recover(std::vector<uint8_t>(31), sig_a, recid_a)));
  EXPECT_TRUE(
      IsPublicKeyEmpty(key->Recover(std::vector<uint8_t>(33), sig_a, recid_a)));
  EXPECT_TRUE(
      IsPublicKeyEmpty(key->Recover(msg_a, std::vector<uint8_t>(31), recid_a)));
  EXPECT_TRUE(
      IsPublicKeyEmpty(key->Recover(msg_a, std::vector<uint8_t>(33), recid_a)));
  EXPECT_TRUE(IsPublicKeyEmpty(key->Recover(msg_a, sig_a, -1)));
  EXPECT_TRUE(IsPublicKeyEmpty(key->Recover(msg_a, sig_a, 4)));
}

TEST(HDKeyUnitTest, SetPrivateKey) {
  HDKey key;
  key.SetPrivateKey(std::vector<uint8_t>(31));
  ASSERT_TRUE(key.private_key_.empty());
  key.SetPrivateKey(std::vector<uint8_t>(33));
  ASSERT_TRUE(key.private_key_.empty());
  key.SetPrivateKey(std::vector<uint8_t>(32, 0x1));
  EXPECT_FALSE(key.private_key_.empty());
  EXPECT_TRUE(!IsPublicKeyEmpty(key.public_key_));
}

TEST(HDKeyUnitTest, SetPublicKey) {
  HDKey key;
  key.SetPublicKey(std::vector<uint8_t>(31));
  EXPECT_TRUE(IsPublicKeyEmpty(key.public_key_));
  key.SetPublicKey(std::vector<uint8_t>(34));
  EXPECT_TRUE(IsPublicKeyEmpty(key.public_key_));
  key.SetPublicKey(std::vector<uint8_t>(33, 0x1));
  EXPECT_TRUE(IsPublicKeyEmpty(key.public_key_));

  std::vector<uint8_t> bytes;
  const std::string valid_pubkey =
      "024d902e1a2fc7a8755ab5b694c575fce742c48d9ff192e63df5193e4c7afe1f9c";
  ASSERT_TRUE(base::HexStringToBytes(valid_pubkey, &bytes));
  key.SetPublicKey(bytes);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(key.public_key_)), valid_pubkey);
}

TEST(HDKeyUnitTest, DeriveChildFromPath) {
  std::unique_ptr<HDKey> m_key =
      HDKey::GenerateFromSeed(std::vector<uint8_t>(32));

  const char* cases[] = {
      "1/2/3/4",      "a/b/1/2",       "////",           "m1234",
      "m'/1/2/3'",    "m/1'''/12",     "m/1/a'/3",       "m/-4",
      "m/2147483648", "m/2147483648'", "m/2/2147483649",
  };

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    std::unique_ptr<HDKey> key;
    key = m_key->DeriveChildFromPath(cases[i]);
    EXPECT_EQ(key, nullptr);
  }

  {
    // public parent derives public child
    std::unique_ptr<HDKey> key = HDKey::GenerateFromExtendedKey(
        "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJo"
        "Cu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8");
    std::unique_ptr<HDKey> derived_key =
        key->DeriveChildFromPath("m/3353535/2223/0/99424/4/33");
    EXPECT_EQ(
        derived_key->GetPublicExtendedKey(),
        "xpub6JdKdVJtdx6sC3nh87pDvnGhotXuU5Kz6Qy7Piy84vUAwWSYShsUGULE8u6gCi"
        "vTHgz7cCKJHiXaaMeieB4YnoFVAsNgHHKXJ2mN6jCMbH1");
  }
  {
    // private key has two bytes of leading zeros
    std::vector<uint8_t> bytes;
    EXPECT_TRUE(
        base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &bytes));

    std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(bytes);
    std::unique_ptr<HDKey> key = m_key->DeriveChildFromPath("m/44'/6'/4'");
    EXPECT_EQ(key->GetPrivateExtendedKey(),
              "xprv9ymoag6W7cR6KBcJzhCM6qqTrb3rRVVwXKzwNqp1tDWcwierEv3BA9if3ARH"
              "MhMPh9u2jNoutcgpUBLMfq3kADDo7LzfoCnhhXMRGX3PXDx");
  }
  {
    // private key has many leading zeros
    std::unique_ptr<HDKey> key = HDKey::GenerateFromExtendedKey(
        "xprv9s21ZrQH143K3ckY9DgU79uMTJkQRLdbCCVDh81SnxTgPzLLGax6uHeBULTtaEtcAv"
        "KjXfT7ZWtHzKjTpujMkUd9dDb8msDeAfnJxrgAYhr");
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(key->private_key_)),
        "00000055378cf5fafb56c711c674143f9b0ee82ab0ba2924f19b64f5ae7cdbfd");
    std::unique_ptr<HDKey> derived_key =
        key->DeriveChildFromPath("m/44'/0'/0'/0/0'");
    EXPECT_EQ(
        base::ToLowerASCII(base::HexEncode(derived_key->private_key_)),
        "3348069561d2a0fb925e74bf198762acc47dce7db27372257d2d959a9e6f8aeb");
  }
}

}  // namespace brave_wallet
