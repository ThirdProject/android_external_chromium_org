// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/ct_test_util.h"

#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "net/cert/ct_serialization.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/x509_certificate.h"

namespace net {

namespace ct {

namespace {

std::string HexToBytes(const char* hex_data) {
  std::vector<uint8> output;
  std::string result;
  if (base::HexStringToBytes(hex_data, &output))
    result.assign(reinterpret_cast<const char*>(&output[0]), output.size());
  return result;
}

// The following test vectors are from
// http://code.google.com/p/certificate-transparency

const char kDefaultDerCert[] =
    "308202ca30820233a003020102020106300d06092a864886f70d01010505003055310b3009"
    "06035504061302474231243022060355040a131b4365727469666963617465205472616e73"
    "706172656e6379204341310e300c0603550408130557616c65733110300e06035504071307"
    "4572772057656e301e170d3132303630313030303030305a170d3232303630313030303030"
    "305a3052310b30090603550406130247423121301f060355040a1318436572746966696361"
    "7465205472616e73706172656e6379310e300c0603550408130557616c65733110300e0603"
    "55040713074572772057656e30819f300d06092a864886f70d010101050003818d00308189"
    "02818100b1fa37936111f8792da2081c3fe41925008531dc7f2c657bd9e1de4704160b4c9f"
    "19d54ada4470404c1c51341b8f1f7538dddd28d9aca48369fc5646ddcc7617f8168aae5b41"
    "d43331fca2dadfc804d57208949061f9eef902ca47ce88c644e000f06eeeccabdc9dd2f68a"
    "22ccb09dc76e0dbc73527765b1a37a8c676253dcc10203010001a381ac3081a9301d060355"
    "1d0e041604146a0d982a3b62c44b6d2ef4e9bb7a01aa9cb798e2307d0603551d2304763074"
    "80145f9d880dc873e654d4f80dd8e6b0c124b447c355a159a4573055310b30090603550406"
    "1302474231243022060355040a131b4365727469666963617465205472616e73706172656e"
    "6379204341310e300c0603550408130557616c65733110300e060355040713074572772057"
    "656e82010030090603551d1304023000300d06092a864886f70d010105050003818100171c"
    "d84aac414a9a030f22aac8f688b081b2709b848b4e5511406cd707fed028597a9faefc2eee"
    "2978d633aaac14ed3235197da87e0f71b8875f1ac9e78b281749ddedd007e3ecf50645f8cb"
    "f667256cd6a1647b5e13203bb8582de7d6696f656d1c60b95f456b7fcf338571908f1c6972"
    "7d24c4fccd249295795814d1dac0e6";

const char kDefaultIssuerKeyHash[] =
    "02adddca08b8bf9861f035940c940156d8350fdff899a6239c6bd77255b8f8fc";

const char kDefaultDerTbsCert[] =
    "30820233a003020102020107300d06092a864886f70d01010505003055310b300906035504"
    "061302474231243022060355040a131b4365727469666963617465205472616e7370617265"
    "6e6379204341310e300c0603550408130557616c65733110300e0603550407130745727720"
    "57656e301e170d3132303630313030303030305a170d3232303630313030303030305a3052"
    "310b30090603550406130247423121301f060355040a131843657274696669636174652054"
    "72616e73706172656e6379310e300c0603550408130557616c65733110300e060355040713"
    "074572772057656e30819f300d06092a864886f70d010101050003818d0030818902818100"
    "beef98e7c26877ae385f75325a0c1d329bedf18faaf4d796bf047eb7e1ce15c95ba2f80ee4"
    "58bd7db86f8a4b252191a79bd700c38e9c0389b45cd4dc9a120ab21e0cb41cd0e72805a410"
    "cd9c5bdb5d4927726daf1710f60187377ea25b1a1e39eed0b88119dc154dc68f7da8e30caf"
    "158a33e6c9509f4a05b01409ff5dd87eb50203010001a381ac3081a9301d0603551d0e0416"
    "04142031541af25c05ffd8658b6843794f5e9036f7b4307d0603551d230476307480145f9d"
    "880dc873e654d4f80dd8e6b0c124b447c355a159a4573055310b3009060355040613024742"
    "31243022060355040a131b4365727469666963617465205472616e73706172656e63792043"
    "41310e300c0603550408130557616c65733110300e060355040713074572772057656e8201"
    "0030090603551d1304023000";

const char kTestDigitallySigned[] =
    "0403004730450220606e10ae5c2d5a1b0aed49dc4937f48de71a4e9784e9c208dfbfe9ef53"
    "6cf7f2022100beb29c72d7d06d61d06bdb38a069469aa86fe12e18bb7cc45689a2c0187ef5"
    "a5";

const char kTestSignedCertificateTimestamp[] =
    "00df1c2ec11500945247a96168325ddc5c7959e8f7c6d388fc002e0bbd3f74d7640000013d"
    "db27ded900000403004730450220606e10ae5c2d5a1b0aed49dc4937f48de71a4e9784e9c2"
    "08dfbfe9ef536cf7f2022100beb29c72d7d06d61d06bdb38a069469aa86fe12e18bb7cc456"
    "89a2c0187ef5a5";

const char kEcP256PublicKey[] =
    "3059301306072a8648ce3d020106082a8648ce3d0301070342000499783cb14533c0161a5a"
    "b45bf95d08a29cd0ea8dd4c84274e2be59ad15c676960cf0afa1074a57ac644b23479e5b3f"
    "b7b245eb4b420ef370210371a944beaceb";

const char kTestKeyId[] =
    "df1c2ec11500945247a96168325ddc5c7959e8f7c6d388fc002e0bbd3f74d764";

const char kTestSCTSignatureData[] =
    "30450220606e10ae5c2d5a1b0aed49dc4937f48de71a4e9784e9c208dfbfe9ef536cf7f202"
    "2100beb29c72d7d06d61d06bdb38a069469aa86fe12e18bb7cc45689a2c0187ef5a5";

const char kTestSCTPrecertSignatureData[] =
    "30450220482f6751af35dba65436be1fd6640f3dbf9a41429495924530288fa3e5e23e0602"
    "2100e4edc0db3ac572b1e2f5e8ab6a680653987dcf41027dfeffa105519d89edbf08";

}  // namespace

void GetX509CertLogEntry(LogEntry* entry) {
  entry->type = ct::LogEntry::LOG_ENTRY_TYPE_X509;
  entry->leaf_certificate = HexToBytes(kDefaultDerCert);
}

std::string GetDerEncodedX509Cert() { return HexToBytes(kDefaultDerCert); }

void GetPrecertLogEntry(LogEntry* entry) {
  entry->type = ct::LogEntry::LOG_ENTRY_TYPE_PRECERT;
  std::string issuer_hash(HexToBytes(kDefaultIssuerKeyHash));
  memcpy(entry->issuer_key_hash.data, issuer_hash.data(), issuer_hash.size());
  entry->tbs_certificate = HexToBytes(kDefaultDerTbsCert);
}

std::string GetTestDigitallySigned() {
  return HexToBytes(kTestDigitallySigned);
}

std::string GetTestSignedCertificateTimestamp() {
  return HexToBytes(kTestSignedCertificateTimestamp);
}

std::string GetTestPublicKey() {
  return HexToBytes(kEcP256PublicKey);
}

std::string GetTestPublicKeyId() {
  return HexToBytes(kTestKeyId);
}

void GetX509CertSCT(scoped_refptr<SignedCertificateTimestamp>* sct_ref) {
  CHECK(sct_ref != NULL);
  *sct_ref = new SignedCertificateTimestamp();
  SignedCertificateTimestamp *const sct(sct_ref->get());
  sct->log_id = HexToBytes(kTestKeyId);
  // Time the log issued a SCT for this certificate, which is
  // Fri Apr  5 10:04:16.089 2013
  sct->timestamp = base::Time::UnixEpoch() +
      base::TimeDelta::FromMilliseconds(GG_INT64_C(1365181456089));
  sct->extensions.clear();

  sct->signature.hash_algorithm = ct::DigitallySigned::HASH_ALGO_SHA256;
  sct->signature.signature_algorithm = ct::DigitallySigned::SIG_ALGO_ECDSA;
  sct->signature.signature_data = HexToBytes(kTestSCTSignatureData);
}

void GetPrecertSCT(scoped_refptr<SignedCertificateTimestamp>* sct_ref) {
  CHECK(sct_ref != NULL);
  *sct_ref = new SignedCertificateTimestamp();
  SignedCertificateTimestamp *const sct(sct_ref->get());
  sct->log_id = HexToBytes(kTestKeyId);
  // Time the log issued a SCT for this Precertificate, which is
  // Fri Apr  5 10:04:16.275 2013
  sct->timestamp = base::Time::UnixEpoch() +
    base::TimeDelta::FromMilliseconds(GG_INT64_C(1365181456275));
  sct->extensions.clear();

  sct->signature.hash_algorithm = ct::DigitallySigned::HASH_ALGO_SHA256;
  sct->signature.signature_algorithm = ct::DigitallySigned::SIG_ALGO_ECDSA;
  sct->signature.signature_data = HexToBytes(kTestSCTPrecertSignatureData);
}

std::string GetDefaultIssuerKeyHash() {
  return HexToBytes(kDefaultIssuerKeyHash);
}

}  // namespace ct

}  // namespace net
