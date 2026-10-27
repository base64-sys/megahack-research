// Item 3 — own SHA-256, no third-party dependency.
//
// Vectors are FIPS 180-4 / NIST CAVP spellings, pinned as literals so the test does not
// depend on the implementation it is checking.

#include <string>

#include "mh/sha256.hpp"
#include "test_runner.hpp"

MH_TEST(sha256_empty_message) {
  MH_CHECK_EQ(mh::sha256_hex(""),
              std::string{"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"});
}

MH_TEST(sha256_abc) {
  MH_CHECK_EQ(mh::sha256_hex("abc"),
              std::string{"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"});
}

MH_TEST(sha256_two_block_message) {
  // 56 bytes: exercises the padding path that needs an extra block.
  MH_CHECK_EQ(mh::sha256_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
              std::string{"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"});
}

MH_TEST(sha256_one_million_a) {
  MH_CHECK_EQ(mh::sha256_hex(std::string(1000000, 'a')),
              std::string{"cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"});
}

MH_TEST(sha256_digest_text_is_lower_case_64_hex) {
  const std::string digest = mh::sha256_hex("megahack.trace");
  MH_CHECK_EQ(digest.size(), std::size_t{64});
  for (const char c : digest) {
    const bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    MH_CHECK(hex);
  }
}
