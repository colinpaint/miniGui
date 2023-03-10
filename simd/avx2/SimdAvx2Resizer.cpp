//{{{
/*
* Simd Library (http://ermig1979.github.io/Simd).
*
* Copyright (c) 2011-2022 Yermalayeu Ihar.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
//}}}
//{{{  includes
#include "SimdMemory.h"
#include "SimdResizer.h"
#include "SimdResizerCommon.h"
#include "SimdStore.h"
#include "SimdSet.h"
#include "SimdUpdate.h"
#include "SimdExtract.h"
#include "SimdUnpack.h"
//}}}

namespace Simd {
  #ifdef SIMD_AVX2_ENABLE
    namespace Avx2 {
      //{{{  area
      //{{{
      ResizerByteArea1x1::ResizerByteArea1x1 (const ResParam & param)
          : Sse41::ResizerByteArea1x1(param)
      {
      }
      //}}}

      //{{{
      SIMD_INLINE __m256i SaveLoadTail (const uint8_t * ptr, size_t tail)
      {
          uint8_t buffer[DA];
          _mm256_storeu_si256((__m256i*)(buffer), _mm256_loadu_si256((__m256i*)(ptr + tail - A)));
          return _mm256_loadu_si256((__m256i*)(buffer + A - tail));
      }
      //}}}
      //{{{
      template<UpdateType update> SIMD_INLINE void ResizerByteArea1x1RowUpdate (const uint8_t * src0, size_t size, int32_t a, int32_t * dst)
      {
          __m256i alpha = SetInt16(a, a);
          size_t sizeA = AlignLo(size, A);
          size_t i = 0;
          for (; i < sizeA; i += A, dst += A)
          {
              __m256i s0 = _mm256_permutevar8x32_epi32(_mm256_loadu_si256((__m256i*)(src0 + i)), K32_TWO_UNPACK_PERMUTE);
              __m256i i0 = UnpackU8<0>(s0);
              __m256i i1 = UnpackU8<1>(s0);
              Update<update, true>(dst + 0 * F, _mm256_madd_epi16(alpha, UnpackU8<0>(i0)));
              Update<update, true>(dst + 1 * F, _mm256_madd_epi16(alpha, UnpackU8<1>(i0)));
              Update<update, true>(dst + 2 * F, _mm256_madd_epi16(alpha, UnpackU8<0>(i1)));
              Update<update, true>(dst + 3 * F, _mm256_madd_epi16(alpha, UnpackU8<1>(i1)));
          }
          if (i < size)
          {
              __m256i s0 = _mm256_permutevar8x32_epi32(SaveLoadTail(src0 + i, size - i), K32_TWO_UNPACK_PERMUTE);
              __m256i i0 = UnpackU8<0>(s0);
              __m256i i1 = UnpackU8<1>(s0);
              Update<update, true>(dst + 0 * F, _mm256_madd_epi16(alpha, UnpackU8<0>(i0)));
              Update<update, true>(dst + 1 * F, _mm256_madd_epi16(alpha, UnpackU8<1>(i0)));
              Update<update, true>(dst + 2 * F, _mm256_madd_epi16(alpha, UnpackU8<0>(i1)));
              Update<update, true>(dst + 3 * F, _mm256_madd_epi16(alpha, UnpackU8<1>(i1)));
          }
      }
      //}}}
      //{{{
      template<UpdateType update> SIMD_INLINE void ResizerByteArea1x1RowUpdate (const uint8_t * src0, size_t stride, size_t size, int32_t a0, int32_t a1, int32_t * dst)
      {
          __m256i alpha = SetInt16(a0, a1);
          const uint8_t * src1 = src0 + stride;
          size_t sizeA = AlignLo(size, A);
          size_t i = 0;
          for (; i < sizeA; i += A, dst += A)
          {
              __m256i s0 = _mm256_permutevar8x32_epi32(_mm256_loadu_si256((__m256i*)(src0 + i)), K32_TWO_UNPACK_PERMUTE);
              __m256i s1 = _mm256_permutevar8x32_epi32(_mm256_loadu_si256((__m256i*)(src1 + i)), K32_TWO_UNPACK_PERMUTE);
              __m256i i0 = UnpackU8<0>(s0, s1);
              __m256i i1 = UnpackU8<1>(s0, s1);
              Update<update, true>(dst + 0 * F, _mm256_madd_epi16(alpha, UnpackU8<0>(i0)));
              Update<update, true>(dst + 1 * F, _mm256_madd_epi16(alpha, UnpackU8<1>(i0)));
              Update<update, true>(dst + 2 * F, _mm256_madd_epi16(alpha, UnpackU8<0>(i1)));
              Update<update, true>(dst + 3 * F, _mm256_madd_epi16(alpha, UnpackU8<1>(i1)));
          }
          if (i < size)
          {
              __m256i s0 = _mm256_permutevar8x32_epi32(_mm256_loadu_si256((__m256i*)(src0 + i)), K32_TWO_UNPACK_PERMUTE);
              __m256i s1 = _mm256_permutevar8x32_epi32(SaveLoadTail(src1 + i, size - i), K32_TWO_UNPACK_PERMUTE);
              __m256i i0 = UnpackU8<0>(s0, s1);
              __m256i i1 = UnpackU8<1>(s0, s1);
              Update<update, true>(dst + 0 * F, _mm256_madd_epi16(alpha, UnpackU8<0>(i0)));
              Update<update, true>(dst + 1 * F, _mm256_madd_epi16(alpha, UnpackU8<1>(i0)));
              Update<update, true>(dst + 2 * F, _mm256_madd_epi16(alpha, UnpackU8<0>(i1)));
              Update<update, true>(dst + 3 * F, _mm256_madd_epi16(alpha, UnpackU8<1>(i1)));
          }
      }

      //}}}
      //{{{
      SIMD_INLINE void ResizerByteArea1x1RowSum (const uint8_t * src, size_t stride, size_t count, size_t size, int32_t curr, int32_t zero, int32_t next, int32_t * dst)
      {
          if (count)
          {
              size_t i = 0;
              ResizerByteArea1x1RowUpdate<UpdateSet>(src, stride, size, curr, count == 1 ? zero - next : zero, dst), src += 2 * stride, i += 2;
              for (; i < count; i += 2, src += 2 * stride)
                  ResizerByteArea1x1RowUpdate<UpdateAdd>(src, stride, size, zero, i == count - 1 ? zero - next : zero, dst);
              if (i == count)
                  ResizerByteArea1x1RowUpdate<UpdateAdd>(src, size, zero - next, dst);
          }
          else
              ResizerByteArea1x1RowUpdate<UpdateSet>(src, size, curr - next, dst);
      }
      //}}}
      //{{{
      template<size_t N> void ResizerByteArea1x1::Run (const uint8_t * src, size_t srcStride, uint8_t * dst, size_t dstStride)
      {
          size_t bodyW = _param.dstW - (N == 3 ? 1 : 0), rowSize = _param.srcW * N, rowRest = dstStride - _param.dstW * N;
          const int32_t * iy = _iy.data, *ix = _ix.data, *ay = _ay.data, *ax = _ax.data;
          int32_t ay0 = ay[0], ax0 = ax[0];
          for (size_t dy = 0; dy < _param.dstH; dy++, dst += rowRest)
          {
              int32_t * buf = _by.data;
              size_t yn = iy[dy + 1] - iy[dy];
              ResizerByteArea1x1RowSum(src, srcStride, yn, rowSize, ay[dy], ay0, ay[dy + 1], buf), src += yn * srcStride;
              size_t dx = 0;
              for (; dx < bodyW; dx++, dst += N)
              {
                  size_t xn = ix[dx + 1] - ix[dx];
                  Sse41::ResizerByteAreaResult<N>(buf, xn, ax[dx], ax0, ax[dx + 1], dst), buf += xn * N;
              }
              for (; dx < _param.dstW; dx++, dst += N)
              {
                  size_t xn = ix[dx + 1] - ix[dx];
                  Base::ResizerByteAreaResult<N>(buf, xn, ax[dx], ax0, ax[dx + 1], dst), buf += xn * N;
              }
          }
      }
      //}}}

      //{{{
      void ResizerByteArea1x1::Run (const uint8_t * src, size_t srcStride, uint8_t * dst, size_t dstStride)
      {
          switch (_param.channels)
          {
          case 1: Run<1>(src, srcStride, dst, dstStride); return;
          case 2: Run<2>(src, srcStride, dst, dstStride); return;
          case 3: Run<3>(src, srcStride, dst, dstStride); return;
          case 4: Run<4>(src, srcStride, dst, dstStride); return;
          default:
              assert(0);
          }
      }
      //}}}
      //{{{
      ResizerByteArea2x2::ResizerByteArea2x2 (const ResParam& param)
          : Sse41::ResizerByteArea2x2(param)
      {
      }
      //}}}

      //{{{
      template <size_t N> SIMD_INLINE __m256i ShuffleColor (__m256i val)
      {
          return val;
      }
      //}}}
      //{{{
      template<> SIMD_INLINE __m256i ShuffleColor <2> (__m256i val)
      {
          static const __m256i IDX = SIMD_MM256_SETR_EPI8(
              0x0, 0x2, 0x1, 0x3, 0x4, 0x6, 0x5, 0x7, 0x8, 0xA, 0x9, 0xB, 0xC, 0xE, 0xD, 0xF,
              0x0, 0x2, 0x1, 0x3, 0x4, 0x6, 0x5, 0x7, 0x8, 0xA, 0x9, 0xB, 0xC, 0xE, 0xD, 0xF);
          return _mm256_shuffle_epi8(val, IDX);
      }
      //}}}
      //{{{
      template<> SIMD_INLINE __m256i ShuffleColor <4> (__m256i val)
      {
          static const __m256i IDX = SIMD_MM256_SETR_EPI8(
              0x0, 0x4, 0x1, 0x5, 0x2, 0x6, 0x3, 0x7, 0x8, 0xC, 0x9, 0xD, 0xA, 0xE, 0xB, 0xF,
              0x0, 0x4, 0x1, 0x5, 0x2, 0x6, 0x3, 0x7, 0x8, 0xC, 0x9, 0xD, 0xA, 0xE, 0xB, 0xF);
          return _mm256_shuffle_epi8(val, IDX);
      }
      //}}}
      //{{{
      template <size_t N> SIMD_INLINE __m256i SaveLoadTail2x2 (const uint8_t* ptr, size_t tail)
      {
          uint8_t buffer[DA];
          _mm256_storeu_si256((__m256i*)(buffer), LoadAfterLast<false, N>(ptr + tail - A));
          return LoadPermuted<false>((__m256i*)(buffer + A - tail - N));
      }
      //}}}
      //{{{
      template <size_t N, UpdateType update> SIMD_INLINE void ResizerByteArea2x2RowUpdateColor (const uint8_t* src0, const uint8_t* src1, size_t size, int32_t val, int32_t* dst)
      {
          if (update == UpdateAdd && val == 0)
              return;
          size_t i = 0;
          size_t size4F = AlignLoAny(size, 4 * F);
          __m256i _val = _mm256_set1_epi16(val);
          for (; i < size4F; i += 4 * F, dst += 2 * F)
          {
              __m256i s0 = _mm256_maddubs_epi16(ShuffleColor<N>(LoadPermuted<false>((__m256i*)(src0 + i))), K8_01);
              __m256i s1 = _mm256_maddubs_epi16(ShuffleColor<N>(LoadPermuted<false>((__m256i*)(src1 + i))), K8_01);
              Update<update, false>(dst + 0, _mm256_madd_epi16(_mm256_unpacklo_epi16(s0, s1), _val));
              Update<update, false>(dst + F, _mm256_madd_epi16(_mm256_unpackhi_epi16(s0, s1), _val));
          }
          if ( i < size)
          {
              size_t tail = size - i;
              __m256i s0 = _mm256_maddubs_epi16(ShuffleColor<N>(SaveLoadTail2x2<N>(src0 + i, tail)), K8_01);
              __m256i s1 = _mm256_maddubs_epi16(ShuffleColor<N>(SaveLoadTail2x2<N>(src1 + i, tail)), K8_01);
              Update<update, false>(dst + 0, _mm256_madd_epi16(_mm256_unpacklo_epi16(s0, s1), _val));
              Update<update, false>(dst + F, _mm256_madd_epi16(_mm256_unpackhi_epi16(s0, s1), _val));
          }
       }
      //}}}
      //{{{
      template <size_t N> SIMD_INLINE void ResizerByteArea2x2RowSum (const uint8_t* src, size_t stride, size_t count, size_t size, int32_t curr, int32_t zero, int32_t next, bool tail, int32_t* dst)
      {
          size_t c = 0;
          if (count)
          {
              ResizerByteArea2x2RowUpdateColor<N, UpdateSet>(src, src + stride, size, curr, dst), src += 2 * stride, c += 2;
              for (; c < count; c += 2, src += 2 * stride)
                  ResizerByteArea2x2RowUpdateColor<N, UpdateAdd>(src, src + stride, size, zero, dst);
              ResizerByteArea2x2RowUpdateColor<N, UpdateAdd>(src, tail ? src : src + stride, size, zero - next, dst);
          }
          else
              ResizerByteArea2x2RowUpdateColor<N, UpdateSet>(src, tail ? src : src + stride, size, curr - next, dst);
      }
      //}}}
      //{{{
      SIMD_INLINE void SaveLoadTailBgr2x2 (const uint8_t* ptr, size_t tail, __m256i* val)
      {
          uint8_t buffer[3 * A];
          _mm256_storeu_si256((__m256i*)(buffer + 00), _mm256_loadu_si256((__m256i*)(ptr + tail - 48)));
          _mm256_storeu_si256((__m256i*)(buffer + 19), LoadAfterLast<false, 3>(ptr + tail - 32));
          val[0] = _mm256_loadu_si256((__m256i*)(buffer + 48 - tail));
          val[1] = _mm256_loadu_si256((__m256i*)(buffer + 64 - tail));
      }
      //}}}
      //{{{
      template <UpdateType update> SIMD_INLINE void ResizerByteArea2x2RowUpdateBgr (const uint8_t* src0, const uint8_t* src1, size_t size, int32_t val, int32_t* dst)
      {
          if (update == UpdateAdd && val == 0)
              return;
          size_t i = 0;
          static const __m256i K32_PRM0 = SIMD_MM256_SETR_EPI32(0x0, 0x1, 0x2, 0x0, 0x3, 0x4, 0x5, 0x0);
          static const __m256i K32_PRM1 = SIMD_MM256_SETR_EPI32(0x2, 0x3, 0x4, 0x0, 0x5, 0x6, 0x7, 0x0);
          static const __m256i K8_SHFL = SIMD_MM256_SETR_EPI8(
              0x0, 0x3, 0x1, 0x4, 0x2, 0x5, 0x6, 0x9, 0x7, 0xA, 0x8, 0xB, -1, -1, -1, -1,
              0x0, 0x3, 0x1, 0x4, 0x2, 0x5, 0x6, 0x9, 0x7, 0xA, 0x8, 0xB, -1, -1, -1, -1);
          static const __m256i K32_PRM2 = SIMD_MM256_SETR_EPI32(0x0, 0x1, 0x2, 0x4, 0x5, 0x6, 0x3, 0x7);
          static const __m256i K32_PRM3 = SIMD_MM256_SETR_EPI32(0x2, 0x4, 0x5, 0x6, 0x3, 0x7, 0x0, 0x1);
          __m256i _val = _mm256_set1_epi32(val);
          size_t size48 = AlignLoAny(size, 48);
          for (; i < size48; i += 48, dst += 24)
          {
              __m256i s00 = _mm256_shuffle_epi8(_mm256_permutevar8x32_epi32(Load<false>((__m256i*)(src0 + i + 0 * F)), K32_PRM0), K8_SHFL);
              __m256i s01 = _mm256_shuffle_epi8(_mm256_permutevar8x32_epi32(Load<false>((__m256i*)(src0 + i + 2 * F)), K32_PRM1), K8_SHFL);
              __m256i s10 = _mm256_shuffle_epi8(_mm256_permutevar8x32_epi32(Load<false>((__m256i*)(src1 + i + 0 * F)), K32_PRM0), K8_SHFL);
              __m256i s11 = _mm256_shuffle_epi8(_mm256_permutevar8x32_epi32(Load<false>((__m256i*)(src1 + i + 2 * F)), K32_PRM1), K8_SHFL);
              __m256i s0 = _mm256_add_epi16(_mm256_maddubs_epi16(s00, K8_01), _mm256_maddubs_epi16(s10, K8_01));
              __m256i s1 = _mm256_add_epi16(_mm256_maddubs_epi16(s01, K8_01), _mm256_maddubs_epi16(s11, K8_01));
              __m256i d0 = _mm256_permutevar8x32_epi32(s0, K32_PRM2);
              __m256i d2 = _mm256_permutevar8x32_epi32(s1, K32_PRM3);
              Update<update, false>(dst + 0 * F, _mm256_madd_epi16(_mm256_cvtepi16_epi32(_mm256_castsi256_si128(d0)), _val));
              Update<update, false>(dst + 1 * F, _mm256_madd_epi16(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(_mm256_or_si256(d0, d2), 1)), _val));
              Update<update, false>(dst + 2 * F, _mm256_madd_epi16(_mm256_cvtepi16_epi32(_mm256_castsi256_si128(d2)), _val));
          }
          if (i < size)
          {
              size_t tail = size - i;
              __m256i s[4];
              SaveLoadTailBgr2x2(src0 + i, tail, s + 0);
              SaveLoadTailBgr2x2(src1 + i, tail, s + 2);
              __m256i s0 = _mm256_add_epi16(
                  _mm256_maddubs_epi16(_mm256_shuffle_epi8(_mm256_permutevar8x32_epi32(s[0], K32_PRM0), K8_SHFL), K8_01),
                  _mm256_maddubs_epi16(_mm256_shuffle_epi8(_mm256_permutevar8x32_epi32(s[2], K32_PRM0), K8_SHFL), K8_01));
              __m256i s1 = _mm256_add_epi16(
                  _mm256_maddubs_epi16(_mm256_shuffle_epi8(_mm256_permutevar8x32_epi32(s[1], K32_PRM1), K8_SHFL), K8_01),
                  _mm256_maddubs_epi16(_mm256_shuffle_epi8(_mm256_permutevar8x32_epi32(s[3], K32_PRM1), K8_SHFL), K8_01));
              __m256i d0 = _mm256_permutevar8x32_epi32(s0, K32_PRM2);
              __m256i d2 = _mm256_permutevar8x32_epi32(s1, K32_PRM3);
              Update<update, false>(dst + 0 * F, _mm256_madd_epi16(_mm256_cvtepi16_epi32(_mm256_castsi256_si128(d0)), _val));
              Update<update, false>(dst + 1 * F, _mm256_madd_epi16(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(_mm256_or_si256(d0, d2), 1)), _val));
              Update<update, false>(dst + 2 * F, _mm256_madd_epi16(_mm256_cvtepi16_epi32(_mm256_castsi256_si128(d2)), _val));
          }
      }
      //}}}
      //{{{
      template<> SIMD_INLINE void ResizerByteArea2x2RowSum <3> (const uint8_t* src, size_t stride, size_t count, size_t size, int32_t curr, int32_t zero, int32_t next, bool tail, int32_t* dst)
      {
          size_t c = 0;
          if (count)
          {
              ResizerByteArea2x2RowUpdateBgr<UpdateSet>(src, src + stride, size, curr, dst), src += 2 * stride, c += 2;
              for (; c < count; c += 2, src += 2 * stride)
                  ResizerByteArea2x2RowUpdateBgr<UpdateAdd>(src, src + stride, size, zero, dst);
              ResizerByteArea2x2RowUpdateBgr<UpdateAdd>(src, tail ? src : src + stride, size, zero - next, dst);
          }
          else
              ResizerByteArea2x2RowUpdateBgr<UpdateSet>(src, tail ? src : src + stride, size, curr - next, dst);
      }
      //}}}

      //{{{
      template<size_t N> void ResizerByteArea2x2::Run (const uint8_t* src, size_t srcStride, uint8_t* dst, size_t dstStride)
      {
          size_t bodyW = _param.dstW - (N == 3 ? 1 : 0), rowSize = _param.srcW * N, rowRest = dstStride - _param.dstW * N;
          const int32_t* iy = _iy.data, * ix = _ix.data, * ay = _ay.data, * ax = _ax.data;
          int32_t ay0 = ay[0], ax0 = ax[0];

          for (size_t dy = 0; dy < _param.dstH; dy++, dst += rowRest) {
            int32_t* buf = _by.data;
            size_t yn = (iy[dy + 1] - iy[dy]) * 2;
            bool tail = (dy == _param.dstH - 1) && (_param.srcH & 1);
            ResizerByteArea2x2RowSum<N>(src, srcStride, yn, rowSize, ay[dy], ay0, ay[dy + 1], tail, buf), src += yn * srcStride;
            size_t dx = 0;
            for (; dx < bodyW; dx++, dst += N) {
              size_t xn = ix[dx + 1] - ix[dx];
              Sse41::ResizerByteAreaResult<N>(buf, xn, ax[dx], ax0, ax[dx + 1], dst), buf += xn * N;
              }
            for (; dx < _param.dstW; dx++, dst += N) {
              size_t xn = ix[dx + 1] - ix[dx];
              Base::ResizerByteAreaResult<N>(buf, xn, ax[dx], ax0, ax[dx + 1], dst), buf += xn * N;
              }
          }
        }
      //}}}
      //{{{
      void ResizerByteArea2x2::Run (const uint8_t* src, size_t srcStride, uint8_t* dst, size_t dstStride) {

        switch (_param.channels) {
          case 1: Run <1> (src, srcStride, dst, dstStride); return;
          case 2: Run <2> (src, srcStride, dst, dstStride); return;
          case 3: Run <3> (src, srcStride, dst, dstStride); return;
          case 4: Run <4> (src, srcStride, dst, dstStride); return;
          default:
            assert(0);
          }
        }
      //}}}
      //}}}
      //{{{  nearest
      //{{{
      ResizerNearest::ResizerNearest (const ResParam& param)
          : Sse41::ResizerNearest(param)
      {
      }
      //}}}
      //{{{
      void ResizerNearest::EstimateParams()
      {
          if (_pixelSize)
              return;
          size_t pixelSize = _param.PixelSize();
          if (pixelSize == 4 || pixelSize == 8 ||
              (pixelSize == 3 && _param.dstW <= _param.srcW) ||
              (pixelSize == 2 && _param.dstW * 2 <= _param.srcW))
              Base::ResizerNearest::EstimateParams();
      }
      //}}}

      //{{{
      const __m256i K8_SHUFFLE_UVXX_TO_UV = SIMD_MM256_SETR_EPI8(
          0x0, 0x1, 0x4, 0x5, 0x8, 0x9, 0xC, 0xD, -1, -1, -1, -1, -1, -1, -1, -1,
          0x0, 0x1, 0x4, 0x5, 0x8, 0x9, 0xC, 0xD, -1, -1, -1, -1, -1, -1, -1, -1);
      //}}}
      const __m256i K32_PERMUTE_UVXX_TO_UV = SIMD_MM256_SETR_EPI32(0x0, 0x1, 0x4, 0x5, -1, -1, -1, -1);
      //{{{
      SIMD_INLINE void Gather8x2 (const uint8_t* src, const int32_t* idx, uint8_t* dst)
      {
          __m256i _idx = _mm256_loadu_si256((__m256i*)idx);
          __m256i uvxx = _mm256_i32gather_epi32((int32_t*)src, _idx, 1);
          __m256i uv = _mm256_permutevar8x32_epi32(_mm256_shuffle_epi8(uvxx, K8_SHUFFLE_UVXX_TO_UV), K32_PERMUTE_UVXX_TO_UV);
          _mm_storeu_si128((__m128i*)dst, _mm256_castsi256_si128(uv));
      }
      //}}}
      //{{{
      void ResizerNearest::Gather2 (const uint8_t* src, size_t srcStride, uint8_t* dst, size_t dstStride)
      {
          size_t body = AlignLo(_param.dstW, 8);
          size_t tail = _param.dstW - 8;
          for (size_t dy = 0; dy < _param.dstH; dy++)
          {
              const uint8_t* srcRow = src + _iy[dy] * srcStride;
              for (size_t dx = 0, offs = 0; dx < body; dx += 8, offs += 16)
                  Avx2::Gather8x2(srcRow, _ix.data + dx, dst + offs);
              Avx2::Gather8x2(srcRow, _ix.data + tail, dst + tail * 2);
              dst += dstStride;
          }
      }
      //}}}

      //{{{

      SIMD_INLINE __m256i Gather8x3 (const uint8_t* src, const int32_t* idx)
      {
          __m256i _idx = _mm256_loadu_si256((__m256i*)idx);
          __m256i bgrx = _mm256_i32gather_epi32((int32_t*)src, _idx, 1);
          return _mm256_permutevar8x32_epi32(_mm256_shuffle_epi8(bgrx, K8_SHUFFLE_BGRA_TO_BGR), K32_PERMUTE_BGRA_TO_BGR);
      }
      //}}}
      //{{{
      void ResizerNearest::Gather3 (const uint8_t* src, size_t srcStride, uint8_t* dst, size_t dstStride)
      {
          size_t body = AlignLo(_param.dstW - 1, 8);
          size_t tail = _param.dstW - 8;
          for (size_t dy = 0; dy < _param.dstH; dy++)
          {
              const uint8_t* srcRow = src + _iy[dy] * srcStride;
              for (size_t dx = 0, offs = 0; dx < body; dx +=8, offs += 24)
                  _mm256_storeu_si256((__m256i*)(dst + offs), Gather8x3(srcRow, _ix.data + dx));
              Store24<false>(dst + tail * 3, Gather8x3(srcRow, _ix.data + tail));
              dst += dstStride;
          }
      }
      //}}}

      //{{{
      SIMD_INLINE void Gather8x4 (const int32_t * src, const int32_t* idx, int32_t* dst)
      {
          __m256i _idx = _mm256_loadu_si256((__m256i*)idx);
          __m256i val = _mm256_i32gather_epi32(src, _idx, 1);
          _mm256_storeu_si256((__m256i*)dst, val);
      }
      //}}}
      //{{{
      void ResizerNearest::Gather4 (const uint8_t* src, size_t srcStride, uint8_t* dst, size_t dstStride)
      {
          size_t body = AlignLo(_param.dstW, 8);
          size_t tail = _param.dstW - 8;
          for (size_t dy = 0; dy < _param.dstH; dy++)
          {
              const int32_t* srcRow = (int32_t*)(src + _iy[dy] * srcStride);
              for (size_t dx = 0; dx < body; dx += 8)
                  Avx2::Gather8x4(srcRow, _ix.data + dx, (int32_t*)dst + dx);
              Avx2::Gather8x4(srcRow, _ix.data + tail, (int32_t*)dst + tail);
              dst += dstStride;
          }
      }
      //}}}

      //{{{
      SIMD_INLINE void Gather4x8 (const int64_t* src, const int32_t* idx, int64_t* dst)
      {
          __m128i _idx = _mm_loadu_si128((__m128i*)idx);
          __m256i val = _mm256_i32gather_epi64((long long*)src, _idx, 1);
          _mm256_storeu_si256((__m256i*)dst, val);
      }
      //}}}
      //{{{
      void ResizerNearest::Gather8 (const uint8_t* src, size_t srcStride, uint8_t* dst, size_t dstStride)
      {
          size_t body = AlignLo(_param.dstW, 4);
          size_t tail = _param.dstW - 4;
          for (size_t dy = 0; dy < _param.dstH; dy++)
          {
              const int64_t* srcRow = (int64_t*)(src + _iy[dy] * srcStride);
              for (size_t dx = 0; dx < body; dx += 4)
                  Avx2::Gather4x8(srcRow, _ix.data + dx, (int64_t*)dst + dx);
              Avx2::Gather4x8(srcRow, _ix.data + tail, (int64_t*)dst + tail);
              dst += dstStride;
          }
      }
      //}}}

      //{{{
      void ResizerNearest::Run (const uint8_t* src, size_t srcStride, uint8_t* dst, size_t dstStride)
      {
          EstimateParams();
          if (_pixelSize == 2 && _param.dstW * 2 <= _param.srcW)
              Gather2(src, srcStride, dst, dstStride);
          else if (_pixelSize == 3 && _param.dstW <= _param.srcW)
              Gather3(src, srcStride, dst, dstStride);
          else if (_pixelSize == 4)
              Gather4(src, srcStride, dst, dstStride);
          else if (_pixelSize == 8)
              Gather8(src, srcStride, dst, dstStride);
          else
              Sse41::ResizerNearest::Run(src, srcStride, dst, dstStride);
      }
      //}}}
      //}}}
      //{{{  bilinear
      //{{{
      ResizerByteBilinear::ResizerByteBilinear(const ResParam & param)
          : Sse41::ResizerByteBilinear(param)
      {
      }
      //}}}
      //{{{
      void ResizerByteBilinear::EstimateParams()
      {
          if (_ax.data)
              return;
          if (_param.channels == 1 && _param.srcW < 4 * _param.dstW)
              _blocks = BlockCountMax(A);
          float scale = (float)_param.srcW / _param.dstW;
          _ax.Resize(AlignHi(_param.dstW, A) * _param.channels * 2, false, _param.align);
          uint8_t * alphas = _ax.data;
          if (_blocks)
          {
              _ixg.Resize(_blocks);
              int block = 0;
              _ixg[0].src = 0;
              _ixg[0].dst = 0;
              for (int dstIndex = 0; dstIndex < (int)_param.dstW; ++dstIndex)
              {
                  float alpha = (float)((dstIndex + 0.5)*scale - 0.5);
                  int srcIndex = (int)::floor(alpha);
                  alpha -= srcIndex;

                  if (srcIndex < 0)
                  {
                      srcIndex = 0;
                      alpha = 0;
                  }

                  if (srcIndex > (int)_param.srcW - 2)
                  {
                      srcIndex = (int)_param.srcW - 2;
                      alpha = 1;
                  }

                  int dst = 2 * dstIndex - _ixg[block].dst;
                  int src = srcIndex - _ixg[block].src;
                  if (src >= A - 1 || dst >= A)
                  {
                      block++;
                      _ixg[block].src = Simd::Min(srcIndex, int(_param.srcW - A));
                      _ixg[block].dst = 2 * dstIndex;
                      dst = 0;
                      src = srcIndex - _ixg[block].src;
                  }
                  _ixg[block].shuffle[dst] = src;
                  _ixg[block].shuffle[dst + 1] = src + 1;

                  alphas[1] = (uint8_t)(alpha * Base::FRACTION_RANGE + 0.5);
                  alphas[0] = (uint8_t)(Base::FRACTION_RANGE - alphas[1]);
                  alphas += 2;
              }
              _blocks = block + 1;
          }
          else
          {
              _ix.Resize(AlignHi(_param.dstW, _param.align/4), true, _param.align);
              for (size_t i = 0; i < _param.dstW; ++i)
              {
                  float alpha = (float)((i + 0.5)*scale - 0.5);
                  ptrdiff_t index = (ptrdiff_t)::floor(alpha);
                  alpha -= index;

                  if (index < 0)
                  {
                      index = 0;
                      alpha = 0;
                  }

                  if (index >(ptrdiff_t)_param.srcW - 2)
                  {
                      index = _param.srcW - 2;
                      alpha = 1;
                  }

                  _ix[i] = (int)index;
                  alphas[1] = (uint8_t)(alpha * Base::FRACTION_RANGE + 0.5);
                  alphas[0] = (uint8_t)(Base::FRACTION_RANGE - alphas[1]);
                  for (size_t channel = 1; channel < _param.channels; channel++)
                      ((uint16_t*)alphas)[channel] = *(uint16_t*)alphas;
                  alphas += 2 * _param.channels;
              }
          }
          size_t size = AlignHi(_param.dstW, _param.align)*_param.channels * 2 + SIMD_ALIGN;
          _bx[0].Resize(size, false, _param.align);
          _bx[1].Resize(size, false, _param.align);
      }
      //}}}

      template <size_t channelCount> void ResizerByteBilinearInterpolateX(const __m256i * alpha, __m256i * buffer);
      //{{{
      template <> SIMD_INLINE void ResizerByteBilinearInterpolateX<1>(const __m256i * alpha, __m256i * buffer)
      {
      #ifdef SIMD_MADDUBS_ERROR
          __m256i _buffer = _mm256_or_si256(K_ZERO, _mm256_load_si256(buffer));
      #else
          __m256i _buffer = _mm256_load_si256(buffer);
      #endif
          _mm256_store_si256(buffer, _mm256_maddubs_epi16(_buffer, _mm256_load_si256(alpha)));
      }
      //}}}

      //{{{
      const __m256i K8_SHUFFLE_X2 = SIMD_MM256_SETR_EPI8(0x0, 0x2, 0x1, 0x3, 0x4, 0x6, 0x5, 0x7, 0x8, 0xA, 0x9, 0xB, 0xC, 0xE, 0xD, 0xF,
          0x0, 0x2, 0x1, 0x3, 0x4, 0x6, 0x5, 0x7, 0x8, 0xA, 0x9, 0xB, 0xC, 0xE, 0xD, 0xF);
      //}}}
      //{{{
      SIMD_INLINE void ResizerByteBilinearInterpolateX2(const __m256i * alpha, __m256i * buffer)
      {
          __m256i src = _mm256_shuffle_epi8(_mm256_load_si256(buffer), K8_SHUFFLE_X2);
          _mm256_store_si256(buffer, _mm256_maddubs_epi16(src, _mm256_load_si256(alpha)));
      }
      //}}}
      //{{{
      template <> SIMD_INLINE void ResizerByteBilinearInterpolateX<2>(const __m256i * alpha, __m256i * buffer)
      {
          ResizerByteBilinearInterpolateX2(alpha + 0, buffer + 0);
          ResizerByteBilinearInterpolateX2(alpha + 1, buffer + 1);
      }
      //}}}

      //{{{
      const __m256i K8_SHUFFLE_X3_00 = SIMD_MM256_SETR_EPI8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
          0xE, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
      //}}}
      //{{{
      const __m256i K8_SHUFFLE_X3_01 = SIMD_MM256_SETR_EPI8(0x0, 0x3, 0x1, 0x4, 0x2, 0x5, 0x6, 0x9, 0x7, 0xA, 0x8, 0xB, 0xC, 0xF, 0xD, -1,
          -1, 0x1, 0x2, 0x5, 0x3, 0x6, 0x4, 0x7, 0x8, 0xB, 0x9, 0xC, 0xA, 0xD, 0xE, -1);
      //}}}
      //{{{
      const __m256i K8_SHUFFLE_X3_02 = SIMD_MM256_SETR_EPI8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0,
          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x1);
      //}}}
      //{{{
      const __m256i K8_SHUFFLE_X3_10 = SIMD_MM256_SETR_EPI8(0xF, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
      //}}}
      //{{{
      const __m256i K8_SHUFFLE_X3_11 = SIMD_MM256_SETR_EPI8(-1, 0x2, 0x0, 0x3, 0x4, 0x7, 0x5, 0x8, 0x6, 0x9, 0xA, 0xD, 0xB, 0xE, 0xC, 0xF,
          0x0, 0x3, 0x1, 0x4, 0x2, 0x5, 0x6, 0x9, 0x7, 0xA, 0x8, 0xB, 0xC, 0xF, 0xD, -1);
      //}}}
      //{{{
      const __m256i K8_SHUFFLE_X3_12 = SIMD_MM256_SETR_EPI8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x0);
      //}}}
      //{{{
      const __m256i K8_SHUFFLE_X3_20 = SIMD_MM256_SETR_EPI8(0xE, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
          0xF, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
      //}}}
      //{{{
      const __m256i K8_SHUFFLE_X3_21 = SIMD_MM256_SETR_EPI8(-1, 0x1, 0x2, 0x5, 0x3, 0x6, 0x4, 0x7, 0x8, 0xB, 0x9, 0xC, 0xA, 0xD, 0xE, -1,
          -1, 0x2, 0x0, 0x3, 0x4, 0x7, 0x5, 0x8, 0x6, 0x9, 0xA, 0xD, 0xB, 0xE, 0xC, 0xF);
      //}}}
      //{{{
      const __m256i K8_SHUFFLE_X3_22 = SIMD_MM256_SETR_EPI8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0x1,
          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
      //}}}
      //{{{
      template <> SIMD_INLINE void ResizerByteBilinearInterpolateX<3>(const __m256i * alpha, __m256i * buffer)
      {
          __m256i src[3], shuffled;
          src[0] = _mm256_load_si256(buffer + 0);
          src[1] = _mm256_load_si256(buffer + 1);
          src[2] = _mm256_load_si256(buffer + 2);

          shuffled = _mm256_shuffle_epi8(_mm256_permute2x128_si256(src[0], src[0], 0x21), K8_SHUFFLE_X3_00);
          shuffled = _mm256_or_si256(shuffled, _mm256_shuffle_epi8(src[0], K8_SHUFFLE_X3_01));
          shuffled = _mm256_or_si256(shuffled, _mm256_shuffle_epi8(_mm256_permute2x128_si256(src[0], src[1], 0x21), K8_SHUFFLE_X3_02));
          _mm256_store_si256(buffer + 0, _mm256_maddubs_epi16(shuffled, _mm256_load_si256(alpha + 0)));

          shuffled = _mm256_shuffle_epi8(_mm256_permute2x128_si256(src[0], src[1], 0x21), K8_SHUFFLE_X3_10);
          shuffled = _mm256_or_si256(shuffled, _mm256_shuffle_epi8(src[1], K8_SHUFFLE_X3_11));
          shuffled = _mm256_or_si256(shuffled, _mm256_shuffle_epi8(_mm256_permute2x128_si256(src[1], src[2], 0x21), K8_SHUFFLE_X3_12));
          _mm256_store_si256(buffer + 1, _mm256_maddubs_epi16(shuffled, _mm256_load_si256(alpha + 1)));

          shuffled = _mm256_shuffle_epi8(_mm256_permute2x128_si256(src[1], src[2], 0x21), K8_SHUFFLE_X3_20);
          shuffled = _mm256_or_si256(shuffled, _mm256_shuffle_epi8(src[2], K8_SHUFFLE_X3_21));
          shuffled = _mm256_or_si256(shuffled, _mm256_shuffle_epi8(_mm256_permute2x128_si256(src[2], src[2], 0x21), K8_SHUFFLE_X3_22));
          _mm256_store_si256(buffer + 2, _mm256_maddubs_epi16(shuffled, _mm256_load_si256(alpha + 2)));
      }
      //}}}

      //{{{
      const __m256i K8_SHUFFLE_X4 = SIMD_MM256_SETR_EPI8(0x0, 0x4, 0x1, 0x5, 0x2, 0x6, 0x3, 0x7, 0x8, 0xC, 0x9, 0xD, 0xA, 0xE, 0xB, 0xF,
          0x0, 0x4, 0x1, 0x5, 0x2, 0x6, 0x3, 0x7, 0x8, 0xC, 0x9, 0xD, 0xA, 0xE, 0xB, 0xF);
      //}}}
      //{{{
      SIMD_INLINE void ResizerByteBilinearInterpolateX4(const __m256i * alpha, __m256i * buffer)
      {
          __m256i src = _mm256_shuffle_epi8(_mm256_load_si256(buffer), K8_SHUFFLE_X4);
          _mm256_store_si256(buffer, _mm256_maddubs_epi16(src, _mm256_load_si256(alpha)));
      }
      //}}}
      //{{{
      template <> SIMD_INLINE void ResizerByteBilinearInterpolateX<4>(const __m256i * alpha, __m256i * buffer)
      {
          ResizerByteBilinearInterpolateX4(alpha + 0, buffer + 0);
          ResizerByteBilinearInterpolateX4(alpha + 1, buffer + 1);
          ResizerByteBilinearInterpolateX4(alpha + 2, buffer + 2);
          ResizerByteBilinearInterpolateX4(alpha + 3, buffer + 3);
      }
      //}}}

      const __m256i K16_FRACTION_ROUND_TERM = SIMD_MM256_SET1_EPI16(Base::BILINEAR_ROUND_TERM);
      //{{{
      template<bool align> SIMD_INLINE __m256i ResizerByteBilinearInterpolateY(const __m256i * pbx0, const __m256i * pbx1, __m256i alpha[2])
      {
          __m256i sum = _mm256_add_epi16(_mm256_mullo_epi16(Load<align>(pbx0), alpha[0]), _mm256_mullo_epi16(Load<align>(pbx1), alpha[1]));
          return _mm256_srli_epi16(_mm256_add_epi16(sum, K16_FRACTION_ROUND_TERM), Base::BILINEAR_SHIFT);
      }
      //}}}
      //{{{
      template<bool align> SIMD_INLINE void ResizerByteBilinearInterpolateY(const uint8_t * bx0, const uint8_t * bx1, __m256i alpha[2], uint8_t * dst)
      {
          __m256i lo = ResizerByteBilinearInterpolateY<align>((__m256i*)bx0 + 0, (__m256i*)bx1 + 0, alpha);
          __m256i hi = ResizerByteBilinearInterpolateY<align>((__m256i*)bx0 + 1, (__m256i*)bx1 + 1, alpha);
          Store<false>((__m256i*)dst, PackI16ToU8(lo, hi));
      }
      //}}}
      //{{{
      template<size_t N> void ResizerByteBilinear::Run(const uint8_t * src, size_t srcStride, uint8_t * dst, size_t dstStride)
      {
          struct One { uint8_t val[N * 1]; };
          struct Two { uint8_t val[N * 2]; };

          size_t size = 2 * _param.dstW*N;
          size_t aligned = AlignHi(size, DA) - DA;
          const size_t step = A * N;
          size_t dstW = _param.dstW;
          ptrdiff_t previous = -2;
          __m256i a[2];
          uint8_t * bx[2] = { _bx[0].data, _bx[1].data };
          const uint8_t * ax = _ax.data;
          const int32_t * ix = _ix.data;

          for (size_t yDst = 0; yDst < _param.dstH; yDst++, dst += dstStride)
          {
              a[0] = _mm256_set1_epi16(int16_t(Base::FRACTION_RANGE - _ay[yDst]));
              a[1] = _mm256_set1_epi16(int16_t(_ay[yDst]));

              ptrdiff_t sy = _iy[yDst];
              int k = 0;

              if (sy == previous)
                  k = 2;
              else if (sy == previous + 1)
              {
                  Swap(bx[0], bx[1]);
                  k = 1;
              }

              previous = sy;

              for (; k < 2; k++)
              {
                  Two * pb = (Two *)bx[k];
                  const One * psrc = (const One *)(src + (sy + k)*srcStride);
                  for (size_t x = 0; x < dstW; x++)
                      pb[x] = *(Two *)(psrc + ix[x]);

                  uint8_t * pbx = bx[k];
                  for (size_t i = 0; i < size; i += step)
                      ResizerByteBilinearInterpolateX<N>((__m256i*)(ax + i), (__m256i*)(pbx + i));
              }

              for (size_t ib = 0, id = 0; ib < aligned; ib += DA, id += A)
                  ResizerByteBilinearInterpolateY<true>(bx[0] + ib, bx[1] + ib, a, dst + id);
              size_t i = size - DA;
              ResizerByteBilinearInterpolateY<false>(bx[0] + i, bx[1] + i, a, dst + i / 2);
          }
      }
      //}}}
      //{{{
      void ResizerByteBilinear::RunG(const uint8_t * src, size_t srcStride, uint8_t * dst, size_t dstStride)
      {
          size_t bufW = AlignHi(_param.dstW, A) * 2;
          size_t size = 2 * _param.dstW;
          size_t aligned = AlignHi(size, DA) - DA;
          size_t blocks = _blocks;
          ptrdiff_t previous = -2;
          __m256i a[2];
          uint8_t * bx[2] = { _bx[0].data, _bx[1].data };
          const uint8_t * ax = _ax.data;
          const Idx * ixg = _ixg.data;

          for (size_t yDst = 0; yDst < _param.dstH; yDst++, dst += dstStride)
          {
              a[0] = _mm256_set1_epi16(int16_t(Base::FRACTION_RANGE - _ay[yDst]));
              a[1] = _mm256_set1_epi16(int16_t(_ay[yDst]));

              ptrdiff_t sy = _iy[yDst];
              int k = 0;

              if (sy == previous)
                  k = 2;
              else if (sy == previous + 1)
              {
                  Swap(bx[0], bx[1]);
                  k = 1;
              }

              previous = sy;

              for (; k < 2; k++)
              {
                  const uint8_t * psrc = src + (sy + k)*srcStride;
                  uint8_t * pdst = bx[k];
                  for (size_t i = 0; i < blocks; ++i)
                      ResizerByteBilinearLoadGrayInterpolated(psrc, ixg[i], ax, pdst);
              }

              for (size_t ib = 0, id = 0; ib < aligned; ib += DA, id += A)
                  ResizerByteBilinearInterpolateY<true>(bx[0] + ib, bx[1] + ib, a, dst + id);
              size_t i = size - DA;
              ResizerByteBilinearInterpolateY<false>(bx[0] + i, bx[1] + i, a, dst + i / 2);
          }
      }
      //}}}
      //{{{
      void ResizerByteBilinear::Run(const uint8_t * src, size_t srcStride, uint8_t * dst, size_t dstStride)
      {
          assert(_param.dstW >= A);

          EstimateParams();
          switch (_param.channels)
          {
          case 1:
              if (_blocks)
                  RunG(src, srcStride, dst, dstStride);
              else
                  Run<1>(src, srcStride, dst, dstStride);
              break;
          case 2: Run<2>(src, srcStride, dst, dstStride); break;
          case 3: Run<3>(src, srcStride, dst, dstStride); break;
          case 4: Run<4>(src, srcStride, dst, dstStride); break;
          default:
              assert(0);
          }
      }
      //}}}
      //{{{
      ResizerShortBilinear::ResizerShortBilinear(const ResParam& param)
          : Sse41::ResizerShortBilinear(param)
      {
      }
      //}}}

      //{{{
      const __m256i RSB_1_0 = SIMD_MM256_SETR_EPI8(
          0x0, 0x1, -1, -1, 0x4, 0x5, -1, -1, 0x8, 0x9, -1, -1, 0xC, 0xD, -1, -1,
          0x0, 0x1, -1, -1, 0x4, 0x5, -1, -1, 0x8, 0x9, -1, -1, 0xC, 0xD, -1, -1);
      //}}}
      //{{{
      const __m256i RSB_1_1 = SIMD_MM256_SETR_EPI8(
          0x2, 0x3, -1, -1, 0x6, 0x7, -1, -1, 0xA, 0xB, -1, -1, 0xE, 0xF, -1, -1,
          0x2, 0x3, -1, -1, 0x6, 0x7, -1, -1, 0xA, 0xB, -1, -1, 0xE, 0xF, -1, -1);
      //}}}
      //{{{
      SIMD_INLINE __m256 BilColS1(const uint16_t* src, const int32_t* idx, __m256 fx0, __m256 fx1)
      {
          __m256i s = _mm256_i32gather_epi32((int32_t*)src, _mm256_loadu_si256((__m256i*)idx), 2);
          __m256 m0 = _mm256_mul_ps(fx0, _mm256_cvtepi32_ps(_mm256_shuffle_epi8(s, RSB_1_0)));
          __m256 m1 = _mm256_mul_ps(fx1, _mm256_cvtepi32_ps(_mm256_shuffle_epi8(s, RSB_1_1)));
          return _mm256_add_ps(m0, m1);
      }
      //}}}

      //{{{
      const __m256i RSB_2_0 = SIMD_MM256_SETR_EPI8(
          0x0, 0x1, -1, -1, 0x2, 0x3, -1, -1, 0x8, 0x9, -1, -1, 0xA, 0xB, -1, -1,
          0x0, 0x1, -1, -1, 0x2, 0x3, -1, -1, 0x8, 0x9, -1, -1, 0xA, 0xB, -1, -1);
      //}}}
      //{{{
      const __m256i RSB_2_1 = SIMD_MM256_SETR_EPI8(
          0x4, 0x5, -1, -1, 0x6, 0x7, -1, -1, 0xC, 0xD, -1, -1, 0xE, 0xF, -1, -1,
          0x4, 0x5, -1, -1, 0x6, 0x7, -1, -1, 0xC, 0xD, -1, -1, 0xE, 0xF, -1, -1);
      //}}}
      //{{{
      SIMD_INLINE __m256 BilColS2(const uint16_t* src, const int32_t* idx, __m256 fx0, __m256 fx1)
      {
          __m256i s = _mm256_setr_epi64x(
              *(uint64_t*)(src + idx[0]), *(uint64_t*)(src + idx[2]),
              *(uint64_t*)(src + idx[4]), *(uint64_t*)(src + idx[6]));
          //__m256i s = _mm256_i64gather_epi64((long long*)src, _mm256_and_si256(_mm256_loadu_si256((__m256i*)idx), K64_00000000FFFFFFFF), 2);
          __m256 m0 = _mm256_mul_ps(fx0, _mm256_cvtepi32_ps(_mm256_shuffle_epi8(s, RSB_2_0)));
          __m256 m1 = _mm256_mul_ps(fx1, _mm256_cvtepi32_ps(_mm256_shuffle_epi8(s, RSB_2_1)));
          return _mm256_add_ps(m0, m1);
      }
      //}}}

      //{{{
      const __m256i RSB_3_0 = SIMD_MM256_SETR_EPI8(
          0x0, 0x1, -1, -1, 0x2, 0x3, -1, -1, 0x4, 0x5, -1, -1, -1, -1, -1, -1,
          0x0, 0x1, -1, -1, 0x2, 0x3, -1, -1, 0x4, 0x5, -1, -1, -1, -1, -1, -1);
      //}}}
      //{{{
      const __m256i RSB_3_1 = SIMD_MM256_SETR_EPI8(
          0x6, 0x7, -1, -1, 0x8, 0x9, -1, -1, 0xA, 0xB, -1, -1, -1, -1, -1, -1,
          0x6, 0x7, -1, -1, 0x8, 0x9, -1, -1, 0xA, 0xB, -1, -1, -1, -1, -1, -1);
      //}}}
      const __m256i RSB_3_P1 = SIMD_MM256_SETR_EPI32(0, 1, 2, 4, 5, 6, 7, 7);
      //{{{
      SIMD_INLINE __m256 BilColS3(const uint16_t* src, const int32_t* idx, __m256 fx0, __m256 fx1)
      {
          __m256i s = Load<false>((__m128i*)(src + idx[0]), (__m128i*)(src + idx[3]));
          __m256 m0 = _mm256_mul_ps(fx0, _mm256_cvtepi32_ps(_mm256_shuffle_epi8(s, RSB_3_0)));
          __m256 m1 = _mm256_mul_ps(fx1, _mm256_cvtepi32_ps(_mm256_shuffle_epi8(s, RSB_3_1)));
          return _mm256_permutevar8x32_ps(_mm256_add_ps(m0, m1), RSB_3_P1);
      }
      //}}}

      //{{{
      const __m256i RSB_4_0 = SIMD_MM256_SETR_EPI8(
          0x0, 0x1, -1, -1, 0x2, 0x3, -1, -1, 0x4, 0x5, -1, -1, 0x6, 0x7, -1, -1,
          0x0, 0x1, -1, -1, 0x2, 0x3, -1, -1, 0x4, 0x5, -1, -1, 0x6, 0x7, -1, -1);
      //}}}
      //{{{
      const __m256i RSB_4_1 = SIMD_MM256_SETR_EPI8(
          0x8, 0x9, -1, -1, 0xA, 0xB, -1, -1, 0xC, 0xD, -1, -1, 0xE, 0xF, -1, -1,
          0x8, 0x9, -1, -1, 0xA, 0xB, -1, -1, 0xC, 0xD, -1, -1, 0xE, 0xF, -1, -1);
      //}}}
      //{{{
      SIMD_INLINE __m256 BilColS4(const uint16_t* src, const int32_t* idx, __m256 fx0, __m256 fx1)
      {
          __m256i s = Load<false>((__m128i*)(src + idx[0]), (__m128i*)(src + idx[4]));
          __m256 m0 = _mm256_mul_ps(fx0, _mm256_cvtepi32_ps(_mm256_shuffle_epi8(s, RSB_4_0)));
          __m256 m1 = _mm256_mul_ps(fx1, _mm256_cvtepi32_ps(_mm256_shuffle_epi8(s, RSB_4_1)));
          return _mm256_add_ps(m0, m1);
      }
      //}}}
      //{{{
      template<size_t N> void ResizerShortBilinear::RunB(const uint16_t* src, size_t srcStride, uint16_t* dst, size_t dstStride)
      {
          size_t rs = _param.dstW * N;
          float* pbx[2] = { _bx[0].data, _bx[1].data };
          int32_t prev = -2;
          size_t rs3 = AlignLoAny(rs - 1, 3);
          size_t rs6 = AlignLoAny(rs - 1, 6);
          size_t rs4 = AlignLo(rs, 4);
          size_t rs8 = AlignLo(rs, 8);
          size_t rs16 = AlignLo(rs, 16);
          __m256 _1 = _mm256_set1_ps(1.0f);
          for (size_t dy = 0; dy < _param.dstH; dy++, dst += dstStride)
          {
              float fy1 = _ay[dy];
              float fy0 = 1.0f - fy1;
              int32_t sy = _iy[dy];
              int32_t k = 0;

              if (sy == prev)
                  k = 2;
              else if (sy == prev + 1)
              {
                  Swap(pbx[0], pbx[1]);
                  k = 1;
              }

              prev = sy;

              for (; k < 2; k++)
              {
                  float* pb = pbx[k];
                  const uint16_t* ps = src + (sy + k) * srcStride;
                  size_t dx = 0;
                  if (N == 1)
                  {
                      for (; dx < rs8; dx += 8)
                      {
                          __m256 fx1 = _mm256_loadu_ps(_ax.data + dx);
                          __m256 fx0 = _mm256_sub_ps(_1, fx1);
                          _mm256_storeu_ps(pb + dx, BilColS1(ps, _ix.data + dx, fx0, fx1));
                      }
                      for (; dx < rs4; dx += 4)
                      {
                          __m128 fx1 = _mm_loadu_ps(_ax.data + dx);
                          __m128 fx0 = _mm_sub_ps(_mm256_castps256_ps128(_1), fx1);
                          _mm_storeu_ps(pb + dx, Sse41::BilColS1(ps, _ix.data + dx, fx0, fx1));
                      }
                  }
                  if (N == 2)
                  {
                      for (; dx < rs8; dx += 8)
                      {
                          __m256 fx1 = _mm256_loadu_ps(_ax.data + dx);
                          __m256 fx0 = _mm256_sub_ps(_1, fx1);
                          _mm256_storeu_ps(pb + dx, BilColS2(ps, _ix.data + dx, fx0, fx1));
                      }
                      for (; dx < rs4; dx += 4)
                      {
                          __m128 fx1 = _mm_loadu_ps(_ax.data + dx);
                          __m128 fx0 = _mm_sub_ps(_mm256_castps256_ps128(_1), fx1);
                          _mm_storeu_ps(pb + dx, Sse41::BilColS2(ps, _ix.data + dx, fx0, fx1));
                      }
                  }
                  if (N == 3)
                  {
                      for (; dx < rs6; dx += 6)
                      {
                          __m256 fx1 = Avx::Load<false>(_ax.data + dx, _ax.data + dx + 3);
                          __m256 fx0 = _mm256_sub_ps(_1, fx1);
                          _mm256_storeu_ps(pb + dx, BilColS3(ps, _ix.data + dx, fx0, fx1));
                      }
                      for (; dx < rs3; dx += 3)
                      {
                          __m128 fx1 = _mm_loadu_ps(_ax.data + dx);
                          __m128 fx0 = _mm_sub_ps(_mm256_castps256_ps128(_1), fx1);
                          _mm_storeu_ps(pb + dx, Sse41::BilColS3(ps + _ix[dx], fx0, fx1));
                      }
                  }
                  if (N == 4)
                  {
                      for (; dx < rs8; dx += 8)
                      {
                          __m256 fx1 = _mm256_loadu_ps(_ax.data + dx);
                          __m256 fx0 = _mm256_sub_ps(_1, fx1);
                          _mm256_storeu_ps(pb + dx, BilColS4(ps, _ix.data + dx, fx0, fx1));
                      }
                      for (; dx < rs4; dx += 4)
                      {
                          __m128 fx1 = _mm_loadu_ps(_ax.data + dx);
                          __m128 fx0 = _mm_sub_ps(_mm256_castps256_ps128(_1), fx1);
                          _mm_storeu_ps(pb + dx, Sse41::BilColS4(ps + _ix[dx], fx0, fx1));
                      }
                  }
                  for (; dx < rs; dx++)
                  {
                      int32_t sx = _ix[dx];
                      float fx = _ax[dx];
                      pb[dx] = ps[sx] * (1.0f - fx) + ps[sx + N] * fx;
                  }
              }

              size_t dx = 0;
              __m256 _fy0 = _mm256_set1_ps(fy0);
              __m256 _fy1 = _mm256_set1_ps(fy1);
              for (; dx < rs16; dx += 16)
              {
                  __m256 m00 = _mm256_mul_ps(_mm256_loadu_ps(pbx[0] + dx + 0), _fy0);
                  __m256 m01 = _mm256_mul_ps(_mm256_loadu_ps(pbx[1] + dx + 0), _fy1);
                  __m256i i0 = _mm256_cvttps_epi32(_mm256_add_ps(m00, m01));
                  __m256 m10 = _mm256_mul_ps(_mm256_loadu_ps(pbx[0] + dx + 8), _fy0);
                  __m256 m11 = _mm256_mul_ps(_mm256_loadu_ps(pbx[1] + dx + 8), _fy1);
                  __m256i i1 = _mm256_cvttps_epi32(_mm256_add_ps(m10, m11));
                  _mm256_storeu_si256((__m256i*)(dst + dx), PackU32ToI16(i0, i1));
              }
              for (; dx < rs8; dx += 8)
              {
                  __m256 m0 = _mm256_mul_ps(_mm256_loadu_ps(pbx[0] + dx), _fy0);
                  __m256 m1 = _mm256_mul_ps(_mm256_loadu_ps(pbx[1] + dx), _fy1);
                  __m256i i0 = _mm256_cvttps_epi32(_mm256_add_ps(m0, m1));
                  _mm_storeu_si128((__m128i*)(dst + dx), _mm256_castsi256_si128(PackU32ToI16(i0, K_ZERO)));
              }
              for (; dx < rs4; dx += 4)
              {
                  __m128 m0 = _mm_mul_ps(_mm_loadu_ps(pbx[0] + dx), _mm256_castps256_ps128(_fy0));
                  __m128 m1 = _mm_mul_ps(_mm_loadu_ps(pbx[1] + dx), _mm256_castps256_ps128(_fy1));
                  __m128i i0 = _mm_cvttps_epi32(_mm_add_ps(m0, m1));
                  _mm_storel_epi64((__m128i*)(dst + dx), _mm_packus_epi32(i0, Sse41::K_ZERO));
              }
              for (; dx < rs; dx++)
                  dst[dx] = Round(pbx[0][dx] * fy0 + pbx[1][dx] * fy1);
          }
      }
      //}}}

      const __m256i RSB_3_P2 = SIMD_MM256_SETR_EPI32(0, 1, 4, 2, 3, 6, 5, 7);
      //{{{
      SIMD_INLINE __m256i PackU32ToI16Rsb3(__m256i lo, __m256i hi)
      {
          return _mm256_permutevar8x32_epi32(_mm256_packus_epi32(lo, hi), RSB_3_P2);
      }
      //}}}

      //{{{
      template<size_t N> void ResizerShortBilinear::RunS (const uint16_t* src, size_t srcStride, uint16_t* dst, size_t dstStride)
      {
          size_t rs = _param.dstW * N;
          size_t rs3 = AlignLoAny(rs - 1, 3);
          size_t rs6 = AlignLoAny(rs - 1, 6);
          size_t rs12 = AlignLoAny(rs - 1, 12);
          size_t rs4 = AlignLo(rs, 4);
          size_t rs8 = AlignLo(rs, 8);
          size_t rs16 = AlignLo(rs, 16);
          __m256 _1 = _mm256_set1_ps(1.0f);
          for (size_t dy = 0; dy < _param.dstH; dy++, dst += dstStride)
          {
              float fy1 = _ay[dy];
              float fy0 = 1.0f - fy1;
              int32_t sy = _iy[dy];
              const uint16_t* ps0 = src + (sy + 0) * srcStride;
              const uint16_t* ps1 = src + (sy + 1) * srcStride;
              size_t dx = 0;
              __m256 _fy0 = _mm256_set1_ps(fy0);
              __m256 _fy1 = _mm256_set1_ps(fy1);
              if (N == 1)
              {
                  for (; dx < rs16; dx += 16)
                  {
                      __m256 fx01 = _mm256_loadu_ps(_ax.data + dx + 0);
                      __m256 fx00 = _mm256_sub_ps(_1, fx01);
                      __m256 m00 = _mm256_mul_ps(BilColS1(ps0, _ix.data + dx + 0, fx00, fx01), _fy0);
                      __m256 m01 = _mm256_mul_ps(BilColS1(ps1, _ix.data + dx + 0, fx00, fx01), _fy1);
                      __m256i i0 = _mm256_cvttps_epi32(_mm256_add_ps(m00, m01));
                      __m256 fx11 = _mm256_loadu_ps(_ax.data + dx + 8);
                      __m256 fx10 = _mm256_sub_ps(_1, fx11);
                      __m256 m10 = _mm256_mul_ps(BilColS1(ps0, _ix.data + dx + 8, fx10, fx11), _fy0);
                      __m256 m11 = _mm256_mul_ps(BilColS1(ps1, _ix.data + dx + 8, fx10, fx11), _fy1);
                      __m256i i1 = _mm256_cvttps_epi32(_mm256_add_ps(m10, m11));
                      _mm256_storeu_si256((__m256i*)(dst + dx), PackU32ToI16(i0, i1));
                  }
                  for (; dx < rs8; dx += 8)
                  {
                      __m256 fx1 = _mm256_loadu_ps(_ax.data + dx);
                      __m256 fx0 = _mm256_sub_ps(_1, fx1);
                      __m256 m0 = _mm256_mul_ps(BilColS1(ps0, _ix.data + dx, fx0, fx1), _fy0);
                      __m256 m1 = _mm256_mul_ps(BilColS1(ps1, _ix.data + dx, fx0, fx1), _fy1);
                      __m256i i0 = _mm256_cvttps_epi32(_mm256_add_ps(m0, m1));
                      _mm_storeu_si128((__m128i*)(dst + dx), _mm256_castsi256_si128(PackU32ToI16(i0, K_ZERO)));
                  }
                  for (; dx < rs4; dx += 4)
                  {
                      __m128 fx1 = _mm_loadu_ps(_ax.data + dx);
                      __m128 fx0 = _mm_sub_ps(_mm256_castps256_ps128(_1), fx1);
                      __m128 m0 = _mm_mul_ps(Sse41::BilColS1(ps0, _ix.data + dx, fx0, fx1), _mm256_castps256_ps128(_fy0));
                      __m128 m1 = _mm_mul_ps(Sse41::BilColS1(ps1, _ix.data + dx, fx0, fx1), _mm256_castps256_ps128(_fy1));
                      __m128i i0 = _mm_cvttps_epi32(_mm_add_ps(m0, m1));
                      _mm_storel_epi64((__m128i*)(dst + dx), _mm_packus_epi32(i0, Sse41::K_ZERO));
                  }
              }
              if (N == 2)
              {
                  for (; dx < rs16; dx += 16)
                  {
                      __m256 fx01 = _mm256_loadu_ps(_ax.data + dx + 0);
                      __m256 fx00 = _mm256_sub_ps(_1, fx01);
                      __m256 m00 = _mm256_mul_ps(BilColS2(ps0, _ix.data + dx + 0, fx00, fx01), _fy0);
                      __m256 m01 = _mm256_mul_ps(BilColS2(ps1, _ix.data + dx + 0, fx00, fx01), _fy1);
                      __m256i i0 = _mm256_cvttps_epi32(_mm256_add_ps(m00, m01));
                      __m256 fx11 = _mm256_loadu_ps(_ax.data + dx + 8);
                      __m256 fx10 = _mm256_sub_ps(_1, fx11);
                      __m256 m10 = _mm256_mul_ps(BilColS2(ps0, _ix.data + dx + 8, fx10, fx11), _fy0);
                      __m256 m11 = _mm256_mul_ps(BilColS2(ps1, _ix.data + dx + 8, fx10, fx11), _fy1);
                      __m256i i1 = _mm256_cvttps_epi32(_mm256_add_ps(m10, m11));
                      _mm256_storeu_si256((__m256i*)(dst + dx), PackU32ToI16(i0, i1));
                  }
                  for (; dx < rs8; dx += 8)
                  {
                      __m256 fx1 = _mm256_loadu_ps(_ax.data + dx);
                      __m256 fx0 = _mm256_sub_ps(_1, fx1);
                      __m256 m0 = _mm256_mul_ps(BilColS2(ps0, _ix.data + dx, fx0, fx1), _fy0);
                      __m256 m1 = _mm256_mul_ps(BilColS2(ps1, _ix.data + dx, fx0, fx1), _fy1);
                      __m256i i0 = _mm256_cvttps_epi32(_mm256_add_ps(m0, m1));
                      _mm_storeu_si128((__m128i*)(dst + dx), _mm256_castsi256_si128(PackU32ToI16(i0, K_ZERO)));
                  }
                  for (; dx < rs4; dx += 4)
                  {
                      __m128 fx1 = _mm_loadu_ps(_ax.data + dx);
                      __m128 fx0 = _mm_sub_ps(_mm256_castps256_ps128(_1), fx1);
                      __m128 m0 = _mm_mul_ps(Sse41::BilColS2(ps0, _ix.data + dx, fx0, fx1), _mm256_castps256_ps128(_fy0));
                      __m128 m1 = _mm_mul_ps(Sse41::BilColS2(ps1, _ix.data + dx, fx0, fx1), _mm256_castps256_ps128(_fy1));
                      __m128i i0 = _mm_cvttps_epi32(_mm_add_ps(m0, m1));
                      _mm_storel_epi64((__m128i*)(dst + dx), _mm_packus_epi32(i0, Sse41::K_ZERO));
                  }
              }
              if (N == 3)
              {
                  for (; dx < rs12; dx += 12)
                  {
                      __m256 fx01 = Avx::Load<false>(_ax.data + dx + 0, _ax.data + dx + 3);
                      __m256 fx00 = _mm256_sub_ps(_1, fx01);
                      __m256 m00 = _mm256_mul_ps(BilColS3(ps0, _ix.data + dx, fx00, fx01), _fy0);
                      __m256 m01 = _mm256_mul_ps(BilColS3(ps1, _ix.data + dx, fx00, fx01), _fy1);
                      __m256i i0 = _mm256_cvttps_epi32(_mm256_add_ps(m00, m01));
                      __m256 fx11 = Avx::Load<false>(_ax.data + dx + 6, _ax.data + dx + 9);
                      __m256 fx10 = _mm256_sub_ps(_1, fx11);
                      __m256 m10 = _mm256_mul_ps(BilColS3(ps0, _ix.data + dx + 6, fx10, fx11), _fy0);
                      __m256 m11 = _mm256_mul_ps(BilColS3(ps1, _ix.data + dx + 6, fx10, fx11), _fy1);
                      __m256i i1 = _mm256_cvttps_epi32(_mm256_add_ps(m10, m11));
                      _mm256_storeu_si256((__m256i*)(dst + dx), PackU32ToI16Rsb3(i0, i1));
                  }
                  for (; dx < rs6; dx += 6)
                  {
                      __m256 fx1 = Avx::Load<false>(_ax.data + dx, _ax.data + dx + 3);
                      __m256 fx0 = _mm256_sub_ps(_1, fx1);
                      __m256 m0 = _mm256_mul_ps(BilColS3(ps0, _ix.data + dx, fx0, fx1), _fy0);
                      __m256 m1 = _mm256_mul_ps(BilColS3(ps1, _ix.data + dx, fx0, fx1), _fy1);
                      __m256i i0 = _mm256_cvttps_epi32(_mm256_add_ps(m0, m1));
                      _mm_storeu_si128((__m128i*)(dst + dx), _mm256_castsi256_si128(PackU32ToI16(i0, K_ZERO)));
                  }
                  for (; dx < rs3; dx += 3)
                  {
                      __m128 fx1 = _mm_loadu_ps(_ax.data + dx);
                      __m128 fx0 = _mm_sub_ps(_mm256_castps256_ps128(_1), fx1);
                      __m128 m0 = _mm_mul_ps(Sse41::BilColS3(ps0 + _ix[dx], fx0, fx1), _mm256_castps256_ps128(_fy0));
                      __m128 m1 = _mm_mul_ps(Sse41::BilColS3(ps1 + _ix[dx], fx0, fx1), _mm256_castps256_ps128(_fy1));
                      __m128i i0 = _mm_cvttps_epi32(_mm_add_ps(m0, m1));
                      _mm_storel_epi64((__m128i*)(dst + dx), _mm_packus_epi32(i0, Sse41::K_ZERO));
                  }
              }
              if (N == 4)
              {
                  for (; dx < rs16; dx += 16)
                  {
                      __m256 fx01 = _mm256_loadu_ps(_ax.data + dx + 0);
                      __m256 fx00 = _mm256_sub_ps(_1, fx01);
                      __m256 m00 = _mm256_mul_ps(BilColS4(ps0, _ix.data + dx + 0, fx00, fx01), _fy0);
                      __m256 m01 = _mm256_mul_ps(BilColS4(ps1, _ix.data + dx + 0, fx00, fx01), _fy1);
                      __m256i i0 = _mm256_cvttps_epi32(_mm256_add_ps(m00, m01));
                      __m256 fx11 = _mm256_loadu_ps(_ax.data + dx + 8);
                      __m256 fx10 = _mm256_sub_ps(_1, fx11);
                      __m256 m10 = _mm256_mul_ps(BilColS4(ps0, _ix.data + dx + 8, fx10, fx11), _fy0);
                      __m256 m11 = _mm256_mul_ps(BilColS4(ps1, _ix.data + dx + 8, fx10, fx11), _fy1);
                      __m256i i1 = _mm256_cvttps_epi32(_mm256_add_ps(m10, m11));
                      _mm256_storeu_si256((__m256i*)(dst + dx), PackU32ToI16(i0, i1));
                  }
                  for (; dx < rs8; dx += 8)
                  {
                      __m256 fx1 = _mm256_loadu_ps(_ax.data + dx);
                      __m256 fx0 = _mm256_sub_ps(_1, fx1);
                      __m256 m0 = _mm256_mul_ps(BilColS4(ps0, _ix.data + dx, fx0, fx1), _fy0);
                      __m256 m1 = _mm256_mul_ps(BilColS4(ps1, _ix.data + dx, fx0, fx1), _fy1);
                      __m256i i0 = _mm256_cvttps_epi32(_mm256_add_ps(m0, m1));
                      _mm_storeu_si128((__m128i*)(dst + dx), _mm256_castsi256_si128(PackU32ToI16(i0, K_ZERO)));
                  }
                  for (; dx < rs4; dx += 4)
                  {
                      __m128 fx1 = _mm_loadu_ps(_ax.data + dx);
                      __m128 fx0 = _mm_sub_ps(_mm256_castps256_ps128(_1), fx1);
                      __m128 m0 = _mm_mul_ps(Sse41::BilColS4(ps0 + _ix[dx], fx0, fx1), _mm256_castps256_ps128(_fy0));
                      __m128 m1 = _mm_mul_ps(Sse41::BilColS4(ps1 + _ix[dx], fx0, fx1), _mm256_castps256_ps128(_fy1));
                      __m128i i0 = _mm_cvttps_epi32(_mm_add_ps(m0, m1));
                      _mm_storel_epi64((__m128i*)(dst + dx), _mm_packus_epi32(i0, Sse41::K_ZERO));
                  }
              }
              for (; dx < rs; dx++)
              {
                  int32_t sx = _ix[dx];
                  float fx1 = _ax[dx];
                  float fx0 = 1.0f - fx1;
                  float r0 = ps0[sx] * fx0 + ps0[sx + N] * fx1;
                  float r1 = ps1[sx] * fx0 + ps1[sx + N] * fx1;
                  dst[dx] = Round(r0 * fy0 + r1 * fy1);
              }
          }
      }
      //}}}
      //{{{
      void ResizerShortBilinear::Run (const uint16_t* src, size_t srcStride, uint16_t* dst, size_t dstStride)
      {
          bool sparse = _param.dstH * 2.0 <= _param.srcH;
          switch (_param.channels)
          {
          case 1: sparse ? RunS<1>(src, srcStride, dst, dstStride) : RunB<1>(src, srcStride, dst, dstStride); return;
          case 2: sparse ? RunS<2>(src, srcStride, dst, dstStride) : RunB<2>(src, srcStride, dst, dstStride); return;
          case 3: sparse ? RunS<3>(src, srcStride, dst, dstStride) : RunB<3>(src, srcStride, dst, dstStride); return;
          case 4: sparse ? RunS<4>(src, srcStride, dst, dstStride) : RunB<4>(src, srcStride, dst, dstStride); return;
          default:
              assert(0);
          }
      }
      //}}}

      //{{{
      ResizerFloatBilinear::ResizerFloatBilinear (const ResParam & param)
          : Base::ResizerFloatBilinear(param)
      {
      }
      //}}}
      //{{{
      void ResizerFloatBilinear::Run (const float * src, size_t srcStride, float * dst, size_t dstStride)
      {
          size_t cn = _param.channels;
          size_t rs = _param.dstW * cn;
          float * pbx[2] = { _bx[0].data, _bx[1].data };
          int32_t prev = -2;
          size_t rsa = AlignLo(rs, Avx::F);
          size_t rsh = AlignLo(rs, Sse41::F);
          for (size_t dy = 0; dy < _param.dstH; dy++, dst += dstStride)
          {
              float fy1 = _ay[dy];
              float fy0 = 1.0f - fy1;
              int32_t sy = _iy[dy];
              int32_t k = 0;

              if (sy == prev)
                  k = 2;
              else if (sy == prev + 1)
              {
                  Swap(pbx[0], pbx[1]);
                  k = 1;
              }

              prev = sy;

              for (; k < 2; k++)
              {
                  float * pb = pbx[k];
                  const float * ps = src + (sy + k)*srcStride;
                  size_t dx = 0;
                  if (cn == 1)
                  {
                      __m256 _1 = _mm256_set1_ps(1.0f);
                      for (; dx < rsa; dx += Avx::F)
                      {
                          __m256i idx = Avx2::LoadPermuted<true>((__m256i*)(_ix.data + dx));
                          __m256 s0145 = _mm256_castpd_ps(_mm256_i32gather_pd((double*)ps, _mm256_extracti128_si256(idx, 0), 4));
                          __m256 s2367 = _mm256_castpd_ps(_mm256_i32gather_pd((double*)ps, _mm256_extracti128_si256(idx, 1), 4));
                          __m256 fx1 = _mm256_load_ps(_ax.data + dx);
                          __m256 fx0 = _mm256_sub_ps(_1, fx1);
                          __m256 s0 = _mm256_shuffle_ps(s0145, s2367, 0x88);
                          __m256 s1 = _mm256_shuffle_ps(s0145, s2367, 0xDD);
                          _mm256_store_ps(pb + dx, _mm256_fmadd_ps(s0, fx0, _mm256_mul_ps(s1, fx1)));
                      }
                      for (; dx < rsh; dx += Sse41::F)
                      {
                          __m128 s01 = Sse41::Load(ps + _ix[dx + 0], ps + _ix[dx + 1]);
                          __m128 s23 = Sse41::Load(ps + _ix[dx + 2], ps + _ix[dx + 3]);
                          __m128 fx1 = _mm_load_ps(_ax.data + dx);
                          __m128 fx0 = _mm_sub_ps(_mm256_castps256_ps128(_1), fx1);
                          __m128 m0 = _mm_mul_ps(fx0, _mm_shuffle_ps(s01, s23, 0x88));
                          __m128 m1 = _mm_mul_ps(fx1, _mm_shuffle_ps(s01, s23, 0xDD));
                          _mm_store_ps(pb + dx, _mm_add_ps(m0, m1));
                      }
                  }
                  if (cn == 3 && rs > 3)
                  {
                      __m256 _1 = _mm256_set1_ps(1.0f);
                      size_t rs3 = rs - 3;
                      size_t rs6 = AlignLoAny(rs3, 6);
                      for (; dx < rs6; dx += 6)
                      {
                          __m256 s0 = Avx::Load<false>(ps + _ix[dx + 0] + 0, ps + _ix[dx + 3] + 0);
                          __m256 s1 = Avx::Load<false>(ps + _ix[dx + 0] + 3, ps + _ix[dx + 3] + 3);
                          __m256 fx1 = Avx::Load<false>(_ax.data + dx + 0, _ax.data + dx + 3);
                          __m256 fx0 = _mm256_sub_ps(_1, fx1);
                          Avx::Store<false>(pb + dx + 0, pb + dx + 3, _mm256_fmadd_ps(fx0, s0, _mm256_mul_ps(fx1, s1)));
                      }
                      for (; dx < rs3; dx += 3)
                      {
                          __m128 s0 = _mm_loadu_ps(ps + _ix[dx] + 0);
                          __m128 s1 = _mm_loadu_ps(ps + _ix[dx] + 3);
                          __m128 fx1 = _mm_set1_ps(_ax.data[dx]);
                          __m128 fx0 = _mm_sub_ps(_mm256_castps256_ps128(_1), fx1);
                          _mm_storeu_ps(pb + dx, _mm_add_ps(_mm_mul_ps(fx0, s0), _mm_mul_ps(fx1, s1)));
                      }
                  }
                  else
                  {
                      __m256 _1 = _mm256_set1_ps(1.0f);
                      __m256i _cn = _mm256_set1_epi32((int)cn);
                      for (; dx < rsa; dx += Avx::F)
                      {
                          __m256i i0 = _mm256_load_si256((__m256i*)(_ix.data + dx));
                          __m256i i1 = _mm256_add_epi32(i0, _cn);
                          __m256 s0 = _mm256_i32gather_ps(ps, i0, 4);
                          __m256 s1 = _mm256_i32gather_ps(ps, i1, 4);
                          __m256 fx1 = _mm256_load_ps(_ax.data + dx);
                          __m256 fx0 = _mm256_sub_ps(_1, fx1);
                          _mm256_store_ps(pb + dx, _mm256_fmadd_ps(s0, fx0, _mm256_mul_ps(s1, fx1)));
                      }
                  }
                  for (; dx < rs; dx++)
                  {
                      int32_t sx = _ix[dx];
                      float fx = _ax[dx];
                      pb[dx] = ps[sx] * (1.0f - fx) + ps[sx + cn] * fx;
                  }
              }

              size_t dx = 0;
              __m256 _fy0 = _mm256_set1_ps(fy0);
              __m256 _fy1 = _mm256_set1_ps(fy1);
              for (; dx < rsa; dx += Avx::F)
              {
                  __m256 b0 = _mm256_load_ps(pbx[0] + dx);
                  __m256 b1 = _mm256_load_ps(pbx[1] + dx);
                  _mm256_storeu_ps(dst + dx, _mm256_fmadd_ps(b0, _fy0, _mm256_mul_ps(b1, _fy1)));
              }
              for (; dx < rsh; dx += Sse41::F)
              {
                  __m128 m0 = _mm_mul_ps(_mm_load_ps(pbx[0] + dx), _mm256_castps256_ps128(_fy0));
                  __m128 m1 = _mm_mul_ps(_mm_load_ps(pbx[1] + dx), _mm256_castps256_ps128(_fy1));
                  _mm_storeu_ps(dst + dx, _mm_add_ps(m0, m1));
              }
              for (; dx < rs; dx++)
                  dst[dx] = pbx[0][dx] * fy0 + pbx[1][dx] * fy1;
          }
      }
      //}}}
      //}}}
      //{{{  bicubic
      //{{{
      ResizerByteBicubic::ResizerByteBicubic(const ResParam& param)
          : Sse41::ResizerByteBicubic(param)
      {
      }
      //}}}

      template<int N> __m256i LoadAx(const int8_t* ax);
      //{{{
      template<> SIMD_INLINE __m256i LoadAx <1> (const int8_t* ax)
      {
          return _mm256_loadu_si256((__m256i*)ax);
      }
      //}}}
      //{{{
      template<> SIMD_INLINE __m256i LoadAx <2> (const int8_t* ax)
      {
          static const __m256i PERMUTE = SIMD_MM256_SETR_EPI32(0, 0, 1, 1, 2, 2, 3, 3);
          return _mm256_permutevar8x32_epi32(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)ax)), PERMUTE);
      }
      //}}}
      //{{{
      template<> SIMD_INLINE __m256i LoadAx <3> (const int8_t* ax)
      {
          static const __m256i PERMUTE = SIMD_MM256_SETR_EPI32(0, 0, 0, 0, 1, 1, 1, 1);
          return  _mm256_permutevar8x32_epi32(_mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)ax)), PERMUTE);
      }
      //}}}
      //{{{
      template<> SIMD_INLINE __m256i LoadAx <4> (const int8_t* ax)
      {
          static const __m256i PERMUTE = SIMD_MM256_SETR_EPI32(0, 0, 0, 0, 1, 1, 1, 1);
          return  _mm256_permutevar8x32_epi32(_mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)ax)), PERMUTE);
      }
      //}}}

      template<int N> __m256i CubicSumX (const uint8_t* src, const int32_t* ix, __m256i ax, __m256i ay);
      //{{{
      template<> SIMD_INLINE __m256i CubicSumX <1> (const uint8_t* src, const int32_t* ix, __m256i ax, __m256i ay)
      {
          __m256i _src = _mm256_i32gather_epi32((int32_t*)src, _mm256_loadu_si256((__m256i*)ix), 1);
          return _mm256_madd_epi16(_mm256_maddubs_epi16(_src, ax), ay);
      }
      //}}}
      //{{{
      template<> SIMD_INLINE __m256i CubicSumX <2> (const uint8_t* src, const int32_t* ix, __m256i ax, __m256i ay)
      {
          static const __m256i SHUFFLE = SIMD_MM256_SETR_EPI8(
              0x0, 0x2, 0x4, 0x6, 0x1, 0x3, 0x5, 0x7, 0x8, 0xA, 0xC, 0xE, 0x9, 0xB, 0xD, 0xF,
              0x0, 0x2, 0x4, 0x6, 0x1, 0x3, 0x5, 0x7, 0x8, 0xA, 0xC, 0xE, 0x9, 0xB, 0xD, 0xF);
          __m256i _src = _mm256_shuffle_epi8(_mm256_i32gather_epi64((long long*)src, _mm_loadu_si128((__m128i*)ix), 1), SHUFFLE);
          return _mm256_madd_epi16(_mm256_maddubs_epi16(_src, ax), ay);
      }
      //}}}
      //{{{
      template<> SIMD_INLINE __m256i CubicSumX <3> (const uint8_t* src, const int32_t* ix, __m256i ax, __m256i ay)
      {
          static const __m256i SHUFFLE = SIMD_MM256_SETR_EPI8(
              0x0, 0x3, 0x6, 0x9, 0x1, 0x4, 0x7, 0xA, 0x2, 0x5, 0x8, 0xB, -1, -1, -1, -1,
              0x0, 0x3, 0x6, 0x9, 0x1, 0x4, 0x7, 0xA, 0x2, 0x5, 0x8, 0xB, -1, -1, -1, -1);
          __m256i _src = _mm256_shuffle_epi8(Load<false>((__m128i*)(src + ix[0]), (__m128i*)(src + ix[1])), SHUFFLE);
          return _mm256_madd_epi16(_mm256_maddubs_epi16(_src, ax), ay);
      }
      //}}}
      //{{{
      template<> SIMD_INLINE __m256i CubicSumX <4> (const uint8_t* src, const int32_t* ix, __m256i ax, __m256i ay)
      {
          static const __m256i SHUFFLE = SIMD_MM256_SETR_EPI8(
              0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF,
              0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF);
          __m256i _src = _mm256_shuffle_epi8(Load<false>((__m128i*)(src + ix[0]), (__m128i*)(src + ix[1])), SHUFFLE);
          return _mm256_madd_epi16(_mm256_maddubs_epi16(_src, ax), ay);
      }
      //}}}

      //{{{
      template <int N> SIMD_INLINE void StoreBicubicInt (__m256i val, uint8_t* dst)
      {
          *((int64_t*)dst) = Extract64i<0>(PackI16ToU8(PackI32ToI16(val, K_ZERO), K_ZERO));
      }
      //}}}
      //{{{
      template <> SIMD_INLINE void StoreBicubicInt <3> (__m256i val, uint8_t* dst)
      {
          static const __m128i SHUFFLE = SIMD_MM_SETR_EPI8(0x0, 0x1, 0x2, 0x4, 0x5, 0x6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
          __m128i u8 = _mm256_castsi256_si128(PackI16ToU8(PackI32ToI16(val, K_ZERO), K_ZERO));
          *((int64_t*)dst) = Sse41::ExtractInt64<0>(_mm_shuffle_epi8(u8, SHUFFLE));
      }
      //}}}
      //{{{
      template <int N> SIMD_INLINE void BicubicInt (const uint8_t* src0, const uint8_t* src1, const uint8_t* src2, const uint8_t* src3, const int32_t* ix, const int8_t* ax, const __m256i* ay, uint8_t* dst)
      {
          static const __m256i ROUND = SIMD_MM256_SET1_EPI32(Base::BICUBIC_ROUND);
          __m256i _ax = LoadAx<N>(ax);
          __m256i say0 = CubicSumX<N>(src0 - N, ix, _ax, ay[0]);
          __m256i say1 = CubicSumX<N>(src1 - N, ix, _ax, ay[1]);
          __m256i say2 = CubicSumX<N>(src2 - N, ix, _ax, ay[2]);
          __m256i say3 = CubicSumX<N>(src3 - N, ix, _ax, ay[3]);
          __m256i sum = _mm256_add_epi32(_mm256_add_epi32(say0, say1), _mm256_add_epi32(say2, say3));
          __m256i dst0 = _mm256_srai_epi32(_mm256_add_epi32(sum, ROUND), Base::BICUBIC_SHIFT);
          StoreBicubicInt<N>(dst0, dst);
      }
      //}}}
      //{{{
      SIMD_INLINE __m256i CubicSumX1 (const uint8_t* src, __m256i ix, __m256i ax, __m256i ay)
      {
          __m256i _src = _mm256_i32gather_epi32((int32_t*)(src - 1), ix, 1);
          return _mm256_madd_epi16(_mm256_maddubs_epi16(_src, ax), ay);
      }
      //}}}

      //{{{
      template <> SIMD_INLINE void BicubicInt <1> (const uint8_t* src0, const uint8_t* src1, const uint8_t* src2, const uint8_t* src3, const int32_t* ix, const int8_t* ax, const __m256i* ay, uint8_t* dst)
      {
          static const __m256i ROUND = SIMD_MM256_SET1_EPI32(Base::BICUBIC_ROUND);
          __m256i _ix = _mm256_loadu_si256((__m256i*)ix);
          __m256i _ax = LoadAx<1>(ax);
          __m256i say0 = CubicSumX1(src0, _ix, _ax, ay[0]);
          __m256i say1 = CubicSumX1(src1, _ix, _ax, ay[1]);
          __m256i say2 = CubicSumX1(src2, _ix, _ax, ay[2]);
          __m256i say3 = CubicSumX1(src3, _ix, _ax, ay[3]);
          __m256i sum = _mm256_add_epi32(_mm256_add_epi32(say0, say1), _mm256_add_epi32(say2, say3));
          __m256i dst0 = _mm256_srai_epi32(_mm256_add_epi32(sum, ROUND), Base::BICUBIC_SHIFT);
          StoreBicubicInt<1>(dst0, dst);
      }
      //}}}
      //{{{
      template<int N> void ResizerByteBicubic::RunS (const uint8_t* src, size_t srcStride, uint8_t* dst, size_t dstStride)
      {
          assert(_xn == 0 && _xt == _param.dstW);
          size_t step = 4 / N * 2;
          size_t body = AlignLoAny(_param.dstW - (N == 3 ? 1 : 0), step);
          for (size_t dy = 0; dy < _param.dstH; dy++, dst += dstStride)
          {
              size_t sy = _iy[dy];
              const uint8_t* src1 = src + sy * srcStride;
              const uint8_t* src2 = src1 + srcStride;
              const uint8_t* src0 = sy ? src1 - srcStride : src1;
              const uint8_t* src3 = sy < _param.srcH - 2 ? src2 + srcStride : src2;
              const int32_t* ay = _ay.data + dy * 4;
              __m256i ays[4];
              ays[0] = _mm256_set1_epi16(ay[0]);
              ays[1] = _mm256_set1_epi16(ay[1]);
              ays[2] = _mm256_set1_epi16(ay[2]);
              ays[3] = _mm256_set1_epi16(ay[3]);
              size_t dx = 0;
              for (; dx < body; dx += step)
                  BicubicInt<N>(src0, src1, src2, src3, _ix.data + dx, _ax.data + dx * 4, ays, dst + dx * N);
              for (; dx < _param.dstW; dx++)
                  Base::BicubicInt<N, -1, 2>(src0, src1, src2, src3, _ix[dx], _ax.data + dx * 4, ay, dst + dx * N);
          }
      }
      //}}}

      template<int F> SIMD_INLINE void PixelCubicSumX(const uint8_t* src, const int32_t* ix, const int8_t* ax, int32_t* dst);
      //{{{
      template<> SIMD_INLINE void PixelCubicSumX <1> (const uint8_t* src, const int32_t* ix, const int8_t* ax, int32_t* dst)
      {
      #if 1
          __m128i src0 = _mm_setr_epi32(*(int32_t*)(src + ix[0]), *(int32_t*)(src + ix[1]), *(int32_t*)(src + ix[2]), *(int32_t*)(src + ix[3]));
          __m128i src1 = _mm_setr_epi32(*(int32_t*)(src + ix[4]), *(int32_t*)(src + ix[5]), *(int32_t*)(src + ix[6]), *(int32_t*)(src + ix[7]));
          __m256i _src = Set(src0, src1);
      #else
          __m256i _src = _mm256_i32gather_epi32((int32_t*)src, _mm256_loadu_si256((__m256i*)ix), 1);
      #endif
          __m256i _ax = _mm256_loadu_si256((__m256i*)ax);
          _mm256_storeu_si256((__m256i*)dst, _mm256_madd_epi16(_mm256_maddubs_epi16(_src, _ax), K16_0001));
      }
      //}}}
      //{{{
      template<> SIMD_INLINE void PixelCubicSumX <2> (const uint8_t* src, const int32_t* ix, const int8_t* ax, int32_t* dst)
      {
          static const __m256i PERMUTE = SIMD_MM256_SETR_EPI32(0, 0, 1, 1, 2, 2, 3, 3);
          __m256i _ax = _mm256_permutevar8x32_epi32(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)ax)), PERMUTE);
          static const __m256i SHUFFLE = SIMD_MM256_SETR_EPI8(
              0x0, 0x2, 0x4, 0x6, 0x1, 0x3, 0x5, 0x7, 0x8, 0xA, 0xC, 0xE, 0x9, 0xB, 0xD, 0xF,
              0x0, 0x2, 0x4, 0x6, 0x1, 0x3, 0x5, 0x7, 0x8, 0xA, 0xC, 0xE, 0x9, 0xB, 0xD, 0xF);
          __m256i _src = _mm256_shuffle_epi8(_mm256_i32gather_epi64((long long*)src, _mm_loadu_si128((__m128i*)ix), 1), SHUFFLE);
          _mm256_storeu_si256((__m256i*)dst, _mm256_madd_epi16(_mm256_maddubs_epi16(_src, _ax), K16_0001));
      }
      //}}}
      //{{{
      template<> SIMD_INLINE void PixelCubicSumX <3> (const uint8_t* src, const int32_t* ix, const int8_t* ax, int32_t* dst)
      {
          static const __m256i PERM_A = SIMD_MM256_SETR_EPI32(0, 0, 0, 1, 1, 1, 0, 0);
          __m256i _ax = _mm256_permutevar8x32_epi32(_mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)ax)), PERM_A);
          static const __m256i SHUFFLE = SIMD_MM256_SETR_EPI8(
              0x0, 0x3, 0x6, 0x9, 0x1, 0x4, 0x7, 0xA, 0x2, 0x5, 0x8, 0xB, -1, -1, -1, -1,
              0x0, 0x3, 0x6, 0x9, 0x1, 0x4, 0x7, 0xA, 0x2, 0x5, 0x8, 0xB, -1, -1, -1, -1);
          static const __m256i PERM_B = SIMD_MM256_SETR_EPI32(0, 1, 2, 4, 5, 6, 0, 0);
          __m256i _src = _mm256_permutevar8x32_epi32(_mm256_shuffle_epi8(Load<false>((__m128i*)(src + ix[0]), (__m128i*)(src + ix[1])), SHUFFLE), PERM_B);
          _mm256_storeu_si256((__m256i*)dst, _mm256_madd_epi16(_mm256_maddubs_epi16(_src, _ax), K16_0001));
      }
      //}}}
      //{{{
      template<> SIMD_INLINE void PixelCubicSumX <4> (const uint8_t* src, const int32_t* ix, const int8_t* ax, int32_t* dst)
      {
          static const __m256i PERMUTE = SIMD_MM256_SETR_EPI32(0, 0, 0, 0, 1, 1, 1, 1);
          __m256i _ax = _mm256_permutevar8x32_epi32(_mm256_castsi128_si256(_mm_loadl_epi64((__m128i*)ax)), PERMUTE);
          static const __m256i SHUFFLE = SIMD_MM256_SETR_EPI8(
              0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF,
              0x0, 0x4, 0x8, 0xC, 0x1, 0x5, 0x9, 0xD, 0x2, 0x6, 0xA, 0xE, 0x3, 0x7, 0xB, 0xF);
          __m256i _src = _mm256_shuffle_epi8(Load<false>((__m128i*)(src + ix[0]), (__m128i*)(src + ix[1])), SHUFFLE);
          _mm256_storeu_si256((__m256i*)dst, _mm256_madd_epi16(_mm256_maddubs_epi16(_src, _ax), K16_0001));
      }
      //}}}
      //{{{
      template<int N> SIMD_INLINE void RowCubicSumX (const uint8_t* src, size_t nose, size_t body, size_t tail, const int32_t* ix, const int8_t* ax, int32_t* dst)
      {
          size_t step = 4 / N * 2;
          size_t bodyS = nose + AlignLoAny(body - nose, step);

          size_t dx = 0;
          for (; dx < nose; dx++, ax += 4, dst += N)
              Base::PixelCubicSumX<N, 0, 2>(src + ix[dx], ax, dst);
          for (; dx < bodyS; dx += step, ax += 4 * step, dst += N * step)
              PixelCubicSumX<N>(src - N, ix + dx, ax, dst);
          for (; dx < body; dx++, ax += 4, dst += N)
              Base::PixelCubicSumX<N, -1, 2>(src + ix[dx], ax, dst);
          for (; dx < tail; dx++, ax += 4, dst += N)
              Base::PixelCubicSumX<N, -1, 1>(src + ix[dx], ax, dst);
      }
      //}}}
      //{{{
      SIMD_INLINE void BicubicRowInt (const int32_t* src0, const int32_t* src1, const int32_t* src2, const int32_t* src3, size_t n, const int32_t* ay, uint8_t* dst)
      {
          size_t nF = AlignLo(n, F);
          size_t i = 0;
          if (nF)
          {
              static const __m256i ROUND = SIMD_MM256_SET1_EPI32(Base::BICUBIC_ROUND);
              __m256i ay0 = _mm256_set1_epi32(ay[0]);
              __m256i ay1 = _mm256_set1_epi32(ay[1]);
              __m256i ay2 = _mm256_set1_epi32(ay[2]);
              __m256i ay3 = _mm256_set1_epi32(ay[3]);
              for (; i < nF; i += F)
              {
                  __m256i say0 = _mm256_mullo_epi32(_mm256_loadu_si256((__m256i*)(src0 + i)), ay0);
                  __m256i say1 = _mm256_mullo_epi32(_mm256_loadu_si256((__m256i*)(src1 + i)), ay1);
                  __m256i say2 = _mm256_mullo_epi32(_mm256_loadu_si256((__m256i*)(src2 + i)), ay2);
                  __m256i say3 = _mm256_mullo_epi32(_mm256_loadu_si256((__m256i*)(src3 + i)), ay3);
                  __m256i sum = _mm256_add_epi32(_mm256_add_epi32(say0, say1), _mm256_add_epi32(say2, say3));
                  __m256i dst0 = _mm256_srai_epi32(_mm256_add_epi32(sum, ROUND), Base::BICUBIC_SHIFT);
                  *((int64_t*)(dst + i)) = Extract64i<0>(PackI16ToU8(PackI32ToI16(dst0, K_ZERO), K_ZERO));
              }
          }
          for (; i < n; ++i)
          {
              int32_t sum = ay[0] * src0[i] + ay[1] * src1[i] + ay[2] * src2[i] + ay[3] * src3[i];
              dst[i] = Base::RestrictRange((sum + Base::BICUBIC_ROUND) >> Base::BICUBIC_SHIFT, 0, 255);
          }
      }
      //}}}
      //{{{
      template<int N> void ResizerByteBicubic::RunB (const uint8_t* src, size_t srcStride, uint8_t* dst, size_t dstStride)
      {
          int32_t prev = -1;
          for (size_t dy = 0; dy < _param.dstH; dy++, dst += dstStride)
          {
              int32_t sy = _iy[dy], next = prev;
              for (int32_t curr = sy - 1, end = sy + 3; curr < end; ++curr)
              {
                  if (curr < prev)
                      continue;
                  const uint8_t* ps = src + RestrictRange(curr, 0, (int)_param.srcH - 1) * srcStride;
                  int32_t* pb = _bx[(curr + 1) & 3].data;
                  RowCubicSumX<N>(ps, _xn, _xt, _param.dstW, _ix.data, _ax.data, pb);
                  next++;
              }
              prev = next;

              const int32_t* ay = _ay.data + dy * 4;
              int32_t* pb0 = _bx[(sy + 0) & 3].data;
              int32_t* pb1 = _bx[(sy + 1) & 3].data;
              int32_t* pb2 = _bx[(sy + 2) & 3].data;
              int32_t* pb3 = _bx[(sy + 3) & 3].data;
              BicubicRowInt(pb0, pb1, pb2, pb3, _bx[0].size, ay, dst);
          }
      }
      //}}}

      //{{{
      void ResizerByteBicubic::Run (const uint8_t* src, size_t srcStride, uint8_t* dst, size_t dstStride)
      {
          bool sparse = _param.dstH * 3.0 <= _param.srcH;
          Init(sparse);
          switch (_param.channels)
          {
          case 1: sparse ? Sse41::ResizerByteBicubic::RunS<1>(src, srcStride, dst, dstStride) : RunB<1>(src, srcStride, dst, dstStride); return;
          case 2: sparse ? RunS<2>(src, srcStride, dst, dstStride) : RunB<2>(src, srcStride, dst, dstStride); return;
          case 3: sparse ? RunS<3>(src, srcStride, dst, dstStride) : RunB<3>(src, srcStride, dst, dstStride); return;
          case 4: sparse ? RunS<4>(src, srcStride, dst, dstStride) : RunB<4>(src, srcStride, dst, dstStride); return;
          default:
              assert(0);
          }
      }
      //}}}
      //}}}
      //{{{
      void * ResizerInit (size_t srcX, size_t srcY, size_t dstX, size_t dstY,
                          size_t channels, SimdResizeChannelType type, SimdResizeMethodType method) {

        ResParam param (srcX, srcY, dstX, dstY, channels, type, method, sizeof(__m256i));

        if (param.IsNearest() && dstX >= F)
          return new ResizerNearest (param);
        else if (param.IsByteBilinear() && dstX >= A)
          return new ResizerByteBilinear (param);
        else if (param.IsShortBilinear() && dstX >= F)
          return new ResizerShortBilinear (param);
        else if (param.IsFloatBilinear())
          return new ResizerFloatBilinear (param);
        else if (param.IsByteBicubic())
          return new ResizerByteBicubic (param);
        else if (param.IsByteArea2x2())
          return new ResizerByteArea2x2 (param);
        else if (param.IsByteArea1x1())
          return new ResizerByteArea1x1 (param);
        else
          return Avx::ResizerInit (srcX, srcY, dstX, dstY, channels, type, method);
        }
      //}}}
      }
  #endif
  }
