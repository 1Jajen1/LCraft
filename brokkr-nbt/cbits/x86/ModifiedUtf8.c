#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <immintrin.h>

#include <stdio.h>

static const uint8_t INVALID_FORMAT = 1<<0;
static const uint8_t TOO_LONG       = 1<<1;
static const uint8_t CONT           = 1<<2;
static const uint8_t TOO_SHORT      = 1<<3;
static const uint8_t OVERLONG_2     = 1<<4;
static const uint8_t OVERLONG_3     = 1<<5;
static const uint8_t LONE_SURROGATE = 1<<6;
static const uint8_t TOO_LARGE      = 1<<0; // TODO Is this possible?

static const uint8_t LOW_ALL = TOO_LONG | TOO_SHORT | CONT | INVALID_FORMAT;

/* 256 bit */

static inline void printVec_256(const __m256i in) {
  uint8_t val[32];
  __m128i var = _mm256_extracti128_si256(in, 0);
  _mm_storeu_si128((__m128i *)val, var);
  var = _mm256_extracti128_si256(in, 1);
  _mm_storeu_si128(((__m128i *)val) + 1, var);
  printf("[%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i] \n", 
          val[0],  val[1],  val[2],  val[3],  val[4],  val[5],  val[6],  val[7],
          val[8],  val[9],  val[10], val[11], val[12], val[13], val[14], val[15],
          val[16], val[17], val[18], val[19], val[20], val[21], val[22], val[23],
          val[24], val[25], val[26], val[27], val[28], val[29], val[30], val[31]);
}

__attribute__((target("avx2")))
static inline __m256i _mm256_through128_set_epi8_rev(
  const uint8_t a0, const uint8_t a1, const uint8_t a2, const uint8_t a3, const uint8_t a4, const uint8_t a5, const uint8_t a6, const uint8_t a7,
  const uint8_t a8, const uint8_t a9, const uint8_t a10, const uint8_t a11, const uint8_t a12, const uint8_t a13, const uint8_t a14, const uint8_t a15
  ) {
    return _mm256_broadcastsi128_si256(_mm_set_epi8(a15, a14, a13, a12, a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, a1, a0));
  }

__attribute__((target("avx2")))
static __inline__ __m256i checkTwoBytes_256(const __m256i inp, const __m256i prev) {
  const __m256i b1H = _mm256_through128_set_epi8_rev(
    // 0000
    TOO_LONG,
    // 0XXX
    TOO_LONG, TOO_LONG, TOO_LONG,
    TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
    // 10XX
    CONT, CONT, CONT, CONT,
    // 1100
    TOO_SHORT | OVERLONG_2,
    // 1101
    TOO_SHORT,
    // 1110
    TOO_SHORT | OVERLONG_3,
    // 1111
    INVALID_FORMAT
  );
  const __m256i resB1H = _mm256_shuffle_epi8(b1H, _mm256_and_si256(_mm256_srli_epi16(prev, 4), _mm256_set1_epi8(0x0F)));

  const __m256i b1L = _mm256_through128_set_epi8_rev(
    // 0000
    LOW_ALL | OVERLONG_2 | OVERLONG_3,
    // 0001
    LOW_ALL | OVERLONG_2,
    // 001X
    LOW_ALL, LOW_ALL,
    // 01XX
    LOW_ALL, LOW_ALL, LOW_ALL, LOW_ALL,
    // 10XX
    LOW_ALL, LOW_ALL, LOW_ALL, LOW_ALL,
    // 1100
    LOW_ALL,
    // 1101
    LOW_ALL,
    // 111X
    LOW_ALL,
    LOW_ALL
  );
  const __m256i resB1L = _mm256_shuffle_epi8(b1L, _mm256_and_si256(prev, _mm256_set1_epi8(0x0F)));

  const __m256i b2H = _mm256_through128_set_epi8_rev(
    // 0000
    TOO_SHORT,
    // 0XXX
    TOO_SHORT, TOO_SHORT, TOO_SHORT,
    TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
    // 100X
    CONT | TOO_LONG | OVERLONG_2 | OVERLONG_3,
    CONT | TOO_LONG | OVERLONG_2 | OVERLONG_3,
    // 101X
    CONT | TOO_LONG | OVERLONG_2,
    CONT | TOO_LONG | OVERLONG_2,
    // 11XX
    TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT
  );
  const __m256i resB2H = _mm256_shuffle_epi8(b2H, _mm256_and_si256(_mm256_srli_epi16(inp, 4), _mm256_set1_epi8(0x0F)));

  // printf("Cases\n");
  // printVec_256(_mm256_and_si256(_mm256_srli_epi16(prev, 4), _mm256_set1_epi8(0x0F)));
  // printVec_256(_mm256_and_si256(prev, _mm256_set1_epi8(0x0F)));
  // printVec_256(_mm256_and_si256(_mm256_srli_epi16(inp, 4), _mm256_set1_epi8(0x0F)));
  // printf("Res\n");
  // printVec_256(resB1H);
  // printVec_256(resB1L);
  // printVec_256(resB2H);
  // printf("EndCases\n");

  return _mm256_and_si256(resB1H, _mm256_and_si256(resB1L, resB2H));
}

__attribute__((target("avx2")))
static __inline__ __m256i check_valid_block_256(const __m256i inp, const __m256i prevInput) {
  const __m256i prev  = _mm256_alignr_epi8(inp, _mm256_permute2x128_si256(prevInput, inp, 0x21), 15);
  const __m256i prev2 = _mm256_alignr_epi8(inp, _mm256_permute2x128_si256(prevInput, inp, 0x21), 14);
  const __m256i prev3 = _mm256_alignr_epi8(inp, _mm256_permute2x128_si256(prevInput, inp, 0x21), 13);
  const __m256i prev4 = _mm256_alignr_epi8(inp, _mm256_permute2x128_si256(prevInput, inp, 0x21), 12);
  // INVALID_FORMAT is bit 0, so its set if the comparison is true anywhere
  const __m256i noNull = _mm256_and_si256(_mm256_cmpeq_epi8(inp, _mm256_set1_epi8(0)), _mm256_set1_epi8(INVALID_FORMAT));
  // Set the OVERLONG_2 bit if we have an embedded null. We XOR this with the other results to unset it from there
  const __m256i embedded =
    _mm256_and_si256(
      _mm256_and_si256(
        _mm256_cmpeq_epi8(prev, _mm256_set1_epi8(0xC0)),
        _mm256_cmpeq_epi8(inp, _mm256_set1_epi8(0x80))
      ), _mm256_set1_epi8(OVERLONG_2)
    );
  const __m256i twos = checkTwoBytes_256(inp, prev);

  const __m256i valid3Cont =
    _mm256_and_si256(
      _mm256_cmpgt_epi8(
        _mm256_subs_epu8(prev2, _mm256_set1_epi8(0xDF)),
        _mm256_set1_epi8(0)
      ), _mm256_set1_epi8(CONT)
    );
  
  const __m256i inValidSurrogates =
    _mm256_and_si256(
      _mm256_xor_si256(
        _mm256_cmpeq_epi8(
          _mm256_and_si256(
            _mm256_cmpeq_epi8(
              _mm256_and_si256(prev3, _mm256_set1_epi8(0xF0)),
              _mm256_set1_epi8(0xA0)
            ), _mm256_cmpeq_epi8(prev4, _mm256_set1_epi8(0xED))
          ), _mm256_and_si256(
            _mm256_cmpeq_epi8(
              _mm256_and_si256(inp, _mm256_set1_epi8(0xF0)),
              _mm256_set1_epi8(0xB0)
            ), _mm256_cmpeq_epi8(prev, _mm256_set1_epi8(0xED))
          )
        ), _mm256_set1_epi8(0xFF)
      ), _mm256_set1_epi8(LONE_SURROGATE)
    );

  // printf("Start\n");
  // printVec_256(inp);
  // printVec_256(prevInput);
  // printVec_256(prev);
  // printVec_256(prev2);
  // printVec_256(prev3);
  // printf("Res\n");
  // printVec_256(noNull);
  // printVec_256(embedded);
  // printVec_256(twos);
  // printVec_256(valid3Cont);
  // printVec_256(inValidSurrogates);
  // printf("End\n");

  return _mm256_or_si256(inValidSurrogates, _mm256_xor_si256(valid3Cont, _mm256_andnot_si256(embedded, _mm256_or_si256(noNull, twos))));
}

struct Checker256 {
  __m256i error;
  __m256i prevInput;
  __m256i prevComplete;
};

__attribute__((target("avx2")))
static __inline__ int isAscii_256(__m256i *inputPtr) {
  __m256i ascii = _mm256_set1_epi8(0);
  for (size_t j = 0; j < 2; j++) {
    // printVec(_mm256_loadu_si256(inputPtr));
    // ascii = _mm256_or_si256(ascii, _mm256_loadu_si256(inputPtr++));
    // check range 1..127
    ascii = _mm256_or_si256(ascii, _mm256_subs_epu8(_mm256_sub_epi8(_mm256_loadu_si256(inputPtr++), _mm256_set1_epi8(1)), _mm256_set1_epi8(0x7E)));
  }
  // return _mm256_testz_si256(ascii, _mm256_set1_epi8(0x80));
  return _mm256_testz_si256(ascii, ascii);
}

__attribute__((target("avx2")))
static __inline__ __m256i isComplete_256(const __m256i inp, const __m256i prevInput) {
  const __m256i prev = _mm256_alignr_epi8(inp, _mm256_permute2x128_si256(prevInput, inp, 0x21), 15);

  const __m256i maxLast = _mm256_through128_set_epi8_rev(
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF, 0xBF
  );
  const __m256i trailing23 =
    _mm256_and_si256(
      _mm256_xor_si256(_mm256_cmpeq_epi8(_mm256_subs_epu8(inp, maxLast), _mm256_set1_epi8(0)), _mm256_set1_epi8(0xFF)),
      _mm256_set_epi8(
        TOO_SHORT,TOO_SHORT,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0
      )
    );
  // Now look for low surrogates
  const __m256i trailingSurrogates =
    _mm256_and_si256(
      _mm256_and_si256(
        _mm256_cmpeq_epi8(
          _mm256_and_si256(inp, _mm256_set1_epi8(0xF0)),
          _mm256_set1_epi8(0xA0)
        ), _mm256_cmpeq_epi8(prev, _mm256_set1_epi8(0xED))
      ), _mm256_set_epi8(
        LONE_SURROGATE,LONE_SURROGATE,LONE_SURROGATE,LONE_SURROGATE,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0
      )
    );


  // printf("IsComplete-\n");
  // printVec_256(inp);
  // printVec_256(trailing23);
  // printVec_256(trailingSurrogates);
  // printf("-IsComplete\n");

  return _mm256_or_si256(trailing23, trailingSurrogates);
}

__attribute__((target("avx2")))
static __inline__ void checkNextBlock_256(struct Checker256 *checker, __m256i *inputPtr) {
  // check if all ascii
  if (isAscii_256(inputPtr)) {
    if (_mm256_testz_si256(checker->prevComplete, checker->prevComplete)) {
      checker->error = _mm256_or_si256(checker->error, checker->prevComplete);
      // printVec_256(checker->error);
      return;
    }
    // printf("ASCII\n");
  } else {
    __m256i prevInput = checker->prevInput;
    __m256i input = _mm256_loadu_si256(inputPtr++);
    // Not all ascii
    checker->error = _mm256_or_si256(checker->error, check_valid_block_256(input, prevInput));
    prevInput = input;
    input = _mm256_loadu_si256(inputPtr++);
    checker->error = _mm256_or_si256(checker->error, check_valid_block_256(input, prevInput));
    // printf("Error\n");
    // printVec_256(checker->error);

    // printf("Block done\n");
    // check incomplete
    checker->prevComplete = isComplete_256(input, prevInput);
    // printVec_256(checker->prevComplete);
    checker->prevInput = input;
  }
}

__attribute__((target("avx2")))
static inline int is_valid_modified_utf8_m256(uint8_t const *const src, size_t len) {
  struct Checker256 checker = {_mm256_set1_epi8(0),_mm256_set1_epi8(0),_mm256_set1_epi8(0)};

  size_t simdSz = len >> 6;
  __m256i *inputPtr = (__m256i *)src;
  for (size_t i = 0; i < simdSz; i++) {
    checkNextBlock_256(&checker, inputPtr);
    inputPtr += 2;
  }

  size_t tailSz = len & 63;
  if (tailSz > 0) {
    __m256i last[2];
    memset((uint8_t*)last, 0x20, 64);
    memcpy(last, (uint8_t *) inputPtr, tailSz);
    checkNextBlock_256(&checker, (__m256i *)last);
  }

  // printf("Test %i %i %i\n", len, simdSz, tailSz);

  // printVec(checker.error);
  // printVec(checker.prevComplete);

  checker.error = _mm256_or_si256(checker.error, checker.prevComplete);

  return _mm256_testz_si256(checker.error, checker.error);
}

/* 128 bit */

static inline void printVec_128(const __m128i var) {
  uint8_t val[16];
  // memcpy(val, &var, sizeof(val));
  _mm_storeu_si128((__m128i *)val, var);
  printf("[%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i] \n", 
          val[0], val[1], val[2] , val[3] , val[4] , val[5] , val[6] , val[7],
          val[8], val[9], val[10], val[11], val[12], val[13], val[14], val[15]);
}

static inline __m128i _mm_set_epi8_rev(
  const uint8_t a0, const uint8_t a1, const uint8_t a2, const uint8_t a3, const uint8_t a4, const uint8_t a5, const uint8_t a6, const uint8_t a7,
  const uint8_t a8, const uint8_t a9, const uint8_t a10, const uint8_t a11, const uint8_t a12, const uint8_t a13, const uint8_t a14, const uint8_t a15
  ) {
    return _mm_set_epi8(a15, a14, a13, a12, a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, a1, a0);
  }

__attribute__((target("ssse3")))
static __inline__ __m128i checkTwoBytes_128(const __m128i inp, const __m128i prev) {
  const __m128i b1H = _mm_set_epi8_rev(
    // 0000
    TOO_LONG,
    // 0XXX
    TOO_LONG, TOO_LONG, TOO_LONG,
    TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
    // 10XX
    CONT, CONT, CONT, CONT,
    // 1100
    TOO_SHORT | OVERLONG_2,
    // 1101
    TOO_SHORT,
    // 1110
    TOO_SHORT | OVERLONG_3,
    // 1111
    INVALID_FORMAT
  );
  const __m128i resB1H = _mm_shuffle_epi8(b1H, _mm_and_si128(_mm_srli_epi16(prev, 4), _mm_set1_epi8(0x0F)));

  const __m128i b1L = _mm_set_epi8_rev(
    // 0000
    LOW_ALL | OVERLONG_2 | OVERLONG_3,
    // 0001
    LOW_ALL | OVERLONG_2,
    // 001X
    LOW_ALL, LOW_ALL,
    // 01XX
    LOW_ALL, LOW_ALL, LOW_ALL, LOW_ALL,
    // 10XX
    LOW_ALL, LOW_ALL, LOW_ALL, LOW_ALL,
    // 1100
    LOW_ALL,
    // 1101
    LOW_ALL,
    // 111X
    LOW_ALL,
    LOW_ALL
  );
  const __m128i resB1L = _mm_shuffle_epi8(b1L, _mm_and_si128(prev, _mm_set1_epi8(0x0F)));

  const __m128i b2H = _mm_set_epi8_rev(
    // 0000
    TOO_SHORT,
    // 0XXX
    TOO_SHORT, TOO_SHORT, TOO_SHORT,
    TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
    // 100X
    CONT | TOO_LONG | OVERLONG_2 | OVERLONG_3,
    CONT | TOO_LONG | OVERLONG_2 | OVERLONG_3,
    // 101X
    CONT | TOO_LONG | OVERLONG_2,
    CONT | TOO_LONG | OVERLONG_2,
    // 11XX
    TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT
  );
  const __m128i resB2H = _mm_shuffle_epi8(b2H, _mm_and_si128(_mm_srli_epi16(inp, 4), _mm_set1_epi8(0x0F)));

  // printf("Cases\n");
  // printVec_128(_mm_and_si128(_mm_srli_epi16(prev, 4), _mm_set1_epi8(0x0F)));
  // printVec_128(_mm_and_si128(prev, _mm_set1_epi8(0x0F)));
  // printVec_128(_mm_and_si128(_mm_srli_epi16(inp, 4), _mm_set1_epi8(0x0F)));
  // printf("Res\n");
  // printVec_128(resB1H);
  // printVec_128(resB1L);
  // printVec_128(resB2H);
  // printf("EndCases\n");

  return _mm_and_si128(resB1H, _mm_and_si128(resB1L, resB2H));
}

__attribute__((target("ssse3")))
static __inline__ __m128i check_valid_block_128(const __m128i inp, const __m128i prevInput) {
  const __m128i prev = _mm_alignr_epi8(inp, prevInput, 15);
  const __m128i prev2 = _mm_alignr_epi8(inp, prevInput, 14);
  const __m128i prev3 = _mm_alignr_epi8(inp, prevInput, 13);
  const __m128i prev4 = _mm_alignr_epi8(inp, prevInput, 12);
  // INVALID_FORMAT is bit 0, so its set if the comparison is true anywhere
  const __m128i noNull = _mm_and_si128(_mm_cmpeq_epi8(inp, _mm_set1_epi8(0)), _mm_set1_epi8(INVALID_FORMAT));
  // Set the OVERLONG_2 bit if we have an embedded null. We XOR this with the other results to unset it from there
  const __m128i embedded =
    _mm_and_si128(
      _mm_and_si128(
        _mm_cmpeq_epi8(prev, _mm_set1_epi8(0xC0)),
        _mm_cmpeq_epi8(inp, _mm_set1_epi8(0x80))
      ), _mm_set1_epi8(OVERLONG_2)
    );
  const __m128i twos = checkTwoBytes_128(inp, prev);

  const __m128i valid3Cont =
    _mm_and_si128(
      _mm_cmpgt_epi8(
        _mm_subs_epu8(prev2, _mm_set1_epi8(0xDF)),
        _mm_set1_epi8(0)
      ), _mm_set1_epi8(CONT)
    );
  
  const __m128i inValidSurrogates =
    _mm_and_si128(
      _mm_xor_si128(
        _mm_cmpeq_epi8(
          _mm_and_si128(
            _mm_cmpeq_epi8(
              _mm_and_si128(prev3, _mm_set1_epi8(0xF0)),
              _mm_set1_epi8(0xA0)
            ), _mm_cmpeq_epi8(prev4, _mm_set1_epi8(0xED))
          ), _mm_and_si128(
            _mm_cmpeq_epi8(
              _mm_and_si128(inp, _mm_set1_epi8(0xF0)),
              _mm_set1_epi8(0xB0)
            ), _mm_cmpeq_epi8(prev, _mm_set1_epi8(0xED))
          )
        ), _mm_set1_epi8(0xFF)
      ), _mm_set1_epi8(LONE_SURROGATE)
    );

  // printf("Start\n");
  // printVec_128(inp);
  // printVec_128(prevInput);
  // printVec_128(prev);
  // printVec_128(prev2);
  // printVec_128(prev3);
  // printf("Res\n");
  // printVec_128(noNull);
  // printVec_128(embedded);
  // printVec_128(twos);
  // printVec_128(valid3Cont);
  // printVec_128(inValidSurrogates);
  // printf("End\n");

  return _mm_or_si128(inValidSurrogates, _mm_xor_si128(valid3Cont, _mm_andnot_si128(embedded, _mm_or_si128(noNull, twos))));
}

struct Checker128 {
  __m128i error;
  __m128i prevInput;
  __m128i prevComplete;
};

__attribute__((target("sse4.2")))
static __inline__ int isAscii_128(__m128i *inputPtr) {
  __m128i ascii = _mm_set1_epi8(0);
  for (size_t j = 0; j < 4; j++) {
    // printVec(_mm_loadu_si128(inputPtr));
    // ascii = _mm_or_si128(ascii, _mm_loadu_si128(inputPtr++));
    // check range 1..127
    ascii = _mm_or_si128(ascii, _mm_subs_epu8(_mm_sub_epi8(_mm_loadu_si128(inputPtr++), _mm_set1_epi8(1)), _mm_set1_epi8(0x7E)));
  }
  // return _mm_testz_si128(ascii, _mm_set1_epi8(0x80));
  return _mm_testz_si128(ascii, ascii);
}

__attribute__((target("sse4.2")))
static __inline__ __m128i isComplete_128(const __m128i inp, const __m128i prevInput) {
  const __m128i prev = _mm_alignr_epi8(inp, prevInput, 15);

  const __m128i maxLast = _mm_set_epi8_rev(
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF, 0xBF
  );
  const __m128i trailing23 =
    _mm_and_si128(
      _mm_xor_si128(_mm_cmpeq_epi8(_mm_subs_epu8(inp, maxLast), _mm_set1_epi8(0)), _mm_set1_epi8(0xFF)),
      _mm_set_epi8_rev(
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,TOO_SHORT,TOO_SHORT
      )
    );
  // Now look for low surrogates
  const __m128i trailingSurrogates =
    _mm_and_si128(
      _mm_and_si128(
        _mm_cmpeq_epi8(
          _mm_and_si128(inp, _mm_set1_epi8(0xF0)),
          _mm_set1_epi8(0xA0)
        ), _mm_cmpeq_epi8(prev, _mm_set1_epi8(0xED))
      ), _mm_set_epi8_rev(
        0,0,0,0,0,0,0,0,
        0,0,0,0,LONE_SURROGATE,LONE_SURROGATE,LONE_SURROGATE,LONE_SURROGATE
      )
    );


  // printf("IsComplete-\n");
  // printVec_128(inp);
  // printVec_128(trailing23);
  // printVec_128(trailingSurrogates);
  // printf("-IsComplete\n");

  return _mm_or_si128(trailing23, trailingSurrogates);
}

__attribute__((target("sse4.2")))
static __inline__ void checkNextBlock_128(struct Checker128 *checker, __m128i *inputPtr) {
  // check if all ascii
  if (isAscii_128(inputPtr)) {
    if (_mm_testz_si128(checker->prevComplete, checker->prevComplete)) {
      checker->error = _mm_or_si128(checker->error, checker->prevComplete);
      // printVec(checker->error);
      return;
    }
    // printf("ASCII\n");
  } else {
    __m128i prevInput = checker->prevInput;
    __m128i input = _mm_loadu_si128(inputPtr++);
    // Not all ascii
    checker->error = _mm_or_si128(checker->error, check_valid_block_128(input, prevInput));
    for (size_t j = 0; j < 3; j++) {
      prevInput = input;
      input = _mm_loadu_si128(inputPtr++);
      checker->error = _mm_or_si128(checker->error, check_valid_block_128(input, prevInput));
      // printf("Error\n");
      // printVec(checker->error);
    }
    // printf("Block done\n");
    // check incomplete
    checker->prevComplete = isComplete_128(input, prevInput);
    // printVec(checker->prevComplete);
    checker->prevInput = input;
  }
}

__attribute__((target("sse4.2")))
static inline int is_valid_modified_utf8_m128(uint8_t const *const src, size_t len) {
  struct Checker128 checker = {_mm_set1_epi8(0),_mm_set1_epi8(0),_mm_set1_epi8(0)};

  size_t simdSz = len >> 6;
  __m128i *inputPtr = (__m128i *)src;
  for (size_t i = 0; i < simdSz; i++) {
    checkNextBlock_128(&checker, inputPtr);
    inputPtr += 4;
  }

  size_t tailSz = len & 63;
  if (tailSz > 0) {
    __m128i last[4];
    memset((uint8_t*)last, 0x20, 64);
    memcpy(last, (uint8_t *) inputPtr, tailSz);
    checkNextBlock_128(&checker, (__m128i *)last);
  }

  // printf("Test %i %i %i\n", len, simdSz, tailSz);

  // printVec(checker.error);
  // printVec(checker.prevComplete);

  checker.error = _mm_or_si128(checker.error, checker.prevComplete);

  return _mm_testz_si128(checker.error, checker.error);
}


/*

Rules for valid modified utf-8 compared to utf-8

- Null is overlong 2 byte sequence
- No 4 byte format. Uses surrogate pairs

Heavily inspired by Koz Ross and the fallback utf-8 validation in https://github.com/haskell/bytestring

TODO State machine based version and simd version
*/

#include <MachDeps.h>

#ifdef WORDS_BIGENDIAN
#define to_little_endian(x) __builtin_bswap64(x)
#else
#define to_little_endian(x) (x)
#endif

// 0x80 in every 'lane'.
static uint64_t const high_bits_mask = 0x8080808080808080ULL;
static uint64_t const low_bits_mask  = 0x7F7F7F7F7F7F7F7FULL;

// unaligned 64 bit reads
static inline uint64_t read_uint64(const uint64_t *p) {
  uint64_t r;
  memcpy(&r, p, 8);
  return r;
}

static inline int is_valid_modified_utf8_fallback(uint8_t const *const src, size_t len) {
  uint8_t const *ptr = (uint8_t const *)src;

  uint8_t const *const end = ptr + len;
  while (ptr < end) {
    uint8_t const byte = *ptr;
    // 1 byte. Highest bit == 0
    if (byte > 0 && byte < 0b10000000) {
      ptr++;

      // verify a block of bytes. Validates up to the first non-ascii byte
      if (ptr + 8 < end && true) {
        uint64_t const *big_ptr = (uint64_t const *)ptr;
        uint64_t word = to_little_endian(read_uint64(big_ptr));

        // First look for non-ascii
        uint64_t tmp = word & high_bits_mask;
        if (tmp == 0) {
          // all ascii. next look for null bytes
          // Hackers Delight Chapter 6 but without the initial & low_bits_mask as word & low_bits_mask == word here
          tmp = word + low_bits_mask;
          tmp = ~(tmp | word | low_bits_mask);
          if (tmp == 0) {
            ptr += 8;
          } else {
            // printf("Invalid null");
            return 0;
          }
        } else {
          ptr += (__builtin_ctzl(tmp) >> 3);
        }
      }
    }
    // maybe 2 bytes?
    // First byte 0b110XXXXX
    // Range of 0b11000000 to 0b11011111
    // Tho technically the range starts at 0b11000010 because utf-8 disallows overlong encodings
    // While this is true for utf-8, it isn't true for modified-utf, because null is encoded as a
    //  two byte overlong sequence, so we also have to check for that. The null byte is [192,128]
    // Second byte 0b10XXXXXX
    // Range of 0b10000000 to 0b10111111
    // Here we can simply cast to signed and use that (int8_t)128 = -128 < (int8_t) 191 = -64 < 0
    else if (ptr + 1 < end &&
        // Check if we have the null byte
        ((byte == 0b11000000 && *(ptr + 1) == 0b10000000) ||
        // Check if both bytes are valid utf-8
        (byte >= 0b11000010 && byte <= 0b11011111 && ((int8_t) * (ptr + 1)) <= (int8_t) 0b10111111))) {
      ptr += 2;
    }
    // maybe 3 bytes without surrogates?
    // First byte is 0b1110XXXX
    // Range of 0b11100000 to 0b11101111
    // Second and third bytes are 0b10XXXXXX
    else if (ptr + 2 < end && byte >= 0b11100000 && byte <= 0b11101111) {
      uint8_t const byte2 = *(ptr + 1);
      bool byte2Valid = (int8_t) byte2 <= (int8_t) 0b10111111;
      bool byte3Valid = ((int8_t) * (ptr + 2)) <= (int8_t) 0b10111111;
      // first accept all 3 byte sequences that are not surrogates
      if (byte2Valid && byte3Valid &&
          // check if overlong
          (byte == 0b11100000 && byte2 >= 0b10100000) ||
          // first byte below the surrogate range and not overlong
          (byte >= 0b11100001 && byte  <= 0b11101100) ||
          // first byte indicates surrogates, check next byte
          // surrogates will be validated later, so only accept
          // non-surrogates here
          (byte == 0b11101101 && byte2 <= 0b10011111) ||
          // above the surrogate range
          (byte >= 0b11101110 && byte  <= 0b11101111)) {
        ptr += 3;
      }
      // check for surrogate pairs
      else if (ptr + 5 < end && byte == 0b11101101) {
        bool byte2Valid = 0b10100000 <= byte2 && byte2 <= 0b10101111;
        bool byte4Valid = *(ptr + 3) == 0b11101101;
        uint8_t const byte5 = *(ptr + 4);
        bool byte5Valid = 0b10110000 <= byte5 && byte5 <= 0b10111111;
        bool byte6Valid = ((int8_t) * (ptr + 5)) <= (int8_t) 0b10111111;
        if (byte2Valid && byte3Valid && byte4Valid && byte5Valid) {
          ptr += 6;
        } else {
          // invalid surrogate pair
          // printf("Invalid surrogate");
          return 0;
        }
      }
      // invalid 3 byte sequence or missing the second half of a surrogate pair
      else {
        // printf("Invalid 3 byte or missing second half surrogate");
        return 0;
      }
    }
    // everything here is invalid
    else {
      // printf("Invalid else %i", byte);
      return 0;
    }
  }
  // we are done and valid
  return 1;
}

int is_valid_modified_utf8(uint8_t const *const src, size_t len) {
  return is_valid_modified_utf8_fallback(src, len);
}

int is_valid_modified_utf8_test(uint8_t const *const src, size_t len) {
  __builtin_cpu_init();

  if (len >= 64) {
    if (__builtin_cpu_supports("avx2")) {
      return is_valid_modified_utf8_m256(src, len);
    } else {
      return is_valid_modified_utf8_m128(src, len);
    }
  } else {
    return is_valid_modified_utf8_fallback(src, len);
  }
}
