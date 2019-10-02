#include <stdio.h>
#include "poly.h"
#include "ntt.h"
#include "polyvec.h"
#include "reduce.h"
#include "cbd.h"
#include "fips202.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "stm32wrapper.h"
#include "randombytes.h"
#include <string.h>

static unsigned long long overflowcnt = 0;
unsigned int t0, t1;

void sys_tick_handler(void)
{
  ++overflowcnt;
}

static void printcycles(const char *s, unsigned long long c)
{
  char outs[32];
  snprintf(outs,sizeof(outs),"%llu\n",c);
  send_USART_str(outs);
}

uint8_t ordered_array_256[256] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};

/*************************************************
* Name:        poly_compress
*
* Description: Compression and subsequent serialization of a polynomial
*
* Arguments:   - unsigned char *r: pointer to output byte array
*              - const poly *a:    pointer to input polynomial
**************************************************/
void poly_compress(unsigned char *r, const poly *a)
{
  uint32_t t[8];
  unsigned int i,j,k=0;

  for(i=0;i<KYBER_N;i+=8)
  {
    for(j=0;j<8;j++)
      t[j] = (((freeze(a->coeffs[i+j]) << 3) + KYBER_Q/2)/KYBER_Q) & 7;

    r[k]   =  t[0]       | (t[1] << 3) | (t[2] << 6);
    r[k+1] = (t[2] >> 2) | (t[3] << 1) | (t[4] << 4) | (t[5] << 7);
    r[k+2] = (t[5] >> 1) | (t[6] << 2) | (t[7] << 5);
    k += 3;
  }
}

/*************************************************
* Name:        poly_decompress
*
* Description: De-serialization and subsequent decompression of a polynomial;
*              approximate inverse of poly_compress
*
* Arguments:   - poly *r:                pointer to output polynomial
*              - const unsigned char *a: pointer to input byte array
**************************************************/
void poly_decompress(poly *r, const unsigned char *a)
{
  unsigned int i;
  for(i=0;i<KYBER_N;i+=8)
  {
    r->coeffs[i+0] =  (((a[0] & 7) * KYBER_Q) + 4)>> 3;
    r->coeffs[i+1] = ((((a[0] >> 3) & 7) * KYBER_Q)+ 4) >> 3;
    r->coeffs[i+2] = ((((a[0] >> 6) | ((a[1] << 2) & 4)) * KYBER_Q) + 4)>> 3;
    r->coeffs[i+3] = ((((a[1] >> 1) & 7) * KYBER_Q) + 4)>> 3;
    r->coeffs[i+4] = ((((a[1] >> 4) & 7) * KYBER_Q) + 4)>> 3;
    r->coeffs[i+5] = ((((a[1] >> 7) | ((a[2] << 1) & 6)) * KYBER_Q) + 4)>> 3;
    r->coeffs[i+6] = ((((a[2] >> 2) & 7) * KYBER_Q) + 4)>> 3;
    r->coeffs[i+7] = ((((a[2] >> 5)) * KYBER_Q) + 4)>> 3;
    a += 3;
  }
}

/*************************************************
* Name:        poly_tobytes
*
* Description: Serialization of a polynomial
*
* Arguments:   - unsigned char *r: pointer to output byte array
*              - const poly *a:    pointer to input polynomial
**************************************************/
void poly_tobytes(unsigned char *r, const poly *a)
{
  int i,j;
  uint16_t t[8];

  for(i=0;i<KYBER_N/8;i++)
  {
    for(j=0;j<8;j++)
      t[j] = freeze(a->coeffs[8*i+j]);

    r[13*i+ 0] =  t[0]        & 0xff;
    r[13*i+ 1] = (t[0] >>  8) | ((t[1] & 0x07) << 5);
    r[13*i+ 2] = (t[1] >>  3) & 0xff;
    r[13*i+ 3] = (t[1] >> 11) | ((t[2] & 0x3f) << 2);
    r[13*i+ 4] = (t[2] >>  6) | ((t[3] & 0x01) << 7);
    r[13*i+ 5] = (t[3] >>  1) & 0xff;
    r[13*i+ 6] = (t[3] >>  9) | ((t[4] & 0x0f) << 4);
    r[13*i+ 7] = (t[4] >>  4) & 0xff;
    r[13*i+ 8] = (t[4] >> 12) | ((t[5] & 0x7f) << 1);
    r[13*i+ 9] = (t[5] >>  7) | ((t[6] & 0x03) << 6);
    r[13*i+10] = (t[6] >>  2) & 0xff;
    r[13*i+11] = (t[6] >> 10) | ((t[7] & 0x1f) << 3);
    r[13*i+12] = (t[7] >>  5);
  }
}

/*************************************************
* Name:        poly_frombytes
*
* Description: De-serialization of a polynomial;
*              inverse of poly_tobytes
*
* Arguments:   - poly *r:                pointer to output polynomial
*              - const unsigned char *a: pointer to input byte array
**************************************************/
void poly_frombytes(poly *r, const unsigned char *a)
{
  int i;
  for(i=0;i<KYBER_N/8;i++)
  {
    r->coeffs[8*i+0] =  a[13*i+ 0]       | (((uint16_t)a[13*i+ 1] & 0x1f) << 8);
    r->coeffs[8*i+1] = (a[13*i+ 1] >> 5) | (((uint16_t)a[13*i+ 2]       ) << 3) | (((uint16_t)a[13*i+ 3] & 0x03) << 11);
    r->coeffs[8*i+2] = (a[13*i+ 3] >> 2) | (((uint16_t)a[13*i+ 4] & 0x7f) << 6);
    r->coeffs[8*i+3] = (a[13*i+ 4] >> 7) | (((uint16_t)a[13*i+ 5]       ) << 1) | (((uint16_t)a[13*i+ 6] & 0x0f) <<  9);
    r->coeffs[8*i+4] = (a[13*i+ 6] >> 4) | (((uint16_t)a[13*i+ 7]       ) << 4) | (((uint16_t)a[13*i+ 8] & 0x01) << 12);
    r->coeffs[8*i+5] = (a[13*i+ 8] >> 1) | (((uint16_t)a[13*i+ 9] & 0x3f) << 7);
    r->coeffs[8*i+6] = (a[13*i+ 9] >> 6) | (((uint16_t)a[13*i+10]       ) << 2) | (((uint16_t)a[13*i+11] & 0x07) << 10);
    r->coeffs[8*i+7] = (a[13*i+11] >> 3) | (((uint16_t)a[13*i+12]       ) << 5);
  }
}

/*************************************************
* Name:        poly_getnoise
*
* Description: Sample a polynomial deterministically from a seed and a nonce,
*              with output polynomial close to centered binomial distribution
*              with parameter KYBER_ETA
*
* Arguments:   - poly *r:                   pointer to output polynomial
*              - const unsigned char *seed: pointer to input seed
*              - unsigned char nonce:       one-byte input nonce
**************************************************/
void poly_getnoise(poly *r,const unsigned char *seed, unsigned char nonce)
{
  unsigned char buf[KYBER_ETA*KYBER_N/4];
  unsigned char extseed[KYBER_SYMBYTES+1];
  int i;

  for(i=0;i<KYBER_SYMBYTES;i++)
    extseed[i] = seed[i];

  // gpio_set(GPIOA, GPIO7);
  // gpio_clear(GPIOA, GPIO7);
  extseed[KYBER_SYMBYTES] = nonce;
  // asm volatile(
  //     "nop" "\n\t"
  //     "nop" "\n\t"
  //     "nop" "\n\t"
  //     "nop" "\n\t"
  // );
  // send_USART_bytes(extseed+KYBER_SYMBYTES,1);
  shake256(buf,KYBER_ETA*KYBER_N/4,extseed,KYBER_SYMBYTES+1);

  cbd(r, buf);
}

/*************************************************
* Name:        poly_ntt
*
* Description: Computes negacyclic number-theoretic transform (NTT) of
*              a polynomial in place;
*              inputs assumed to be in normal order, output in bitreversed order
*
* Arguments:   - uint16_t *r: pointer to in/output polynomial
**************************************************/
void poly_ntt(poly *r, int protection_level)
{
  ntt(r->coeffs, protection_level);
}

/*************************************************
* Name:        poly_invntt
*
* Description: Computes inverse of negacyclic number-theoretic transform (NTT) of
*              a polynomial in place;
*              inputs assumed to be in bitreversed order, output in normal order
*
* Arguments:   - uint16_t *a: pointer to in/output polynomial
**************************************************/
void poly_invntt(poly *r, int protection_level)
{
  invntt(r->coeffs, protection_level);
}

/*************************************************
* Name:        poly_add
*
* Description: Add two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_add(poly *r, const poly *a, const poly *b, int protection_level)
{
  int i;
  uint8_t temp,random_index;

    if(protection_level == 1)
    {
        // send_USART_str("Shuffled poly_sub\n");
        // Shuffling for every stage...
        t0 = systick_get_value();
        overflowcnt = 0;
        for (i = 256-1; i >= 1; --i)
        {
              randombytes(&random_index,1);
              random_index = random_index%(i + 1);
              temp = ordered_array_256[i];
              ordered_array_256[i] = ordered_array_256[random_index];
              ordered_array_256[random_index] = temp;
        }
        t1 = systick_get_value();
        printcycles("", (t0+overflowcnt*2400000llu)-t1);

        for(i=0;i<KYBER_N;i++)
        {
            temp = ordered_array_256[i];
            r->coeffs[temp] = barrett_reduce(a->coeffs[temp] + b->coeffs[temp]);
        }

    }
    else
    {
        t0 = systick_get_value();
        overflowcnt = 0;
        for(i=0;i<KYBER_N;i++)
        {
            r->coeffs[i] = barrett_reduce(a->coeffs[i] + b->coeffs[i]);
        }
        t1 = systick_get_value();
        printcycles("", (t0+overflowcnt*2400000llu)-t1);
    }
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_sub(poly *r, const poly *a, const poly *b, int protection_level)
{
    int i;
    uint8_t temp,random_index;

    if(protection_level == 1)
    {
        // send_USART_str("Shuffled poly_sub\n");
        // Shuffling for every stage...
        for (i = 256-1; i >= 1; --i)
        {
            randombytes(&random_index,1);
            random_index = random_index%(i + 1);
            temp = ordered_array_256[i];
            ordered_array_256[i] = ordered_array_256[random_index];
            ordered_array_256[random_index] = temp;
        }

        for(i=0;i<KYBER_N;i++)
        {
            temp = ordered_array_256[i];
            r->coeffs[temp] = barrett_reduce(a->coeffs[temp] + 3*KYBER_Q - b->coeffs[temp]);
        }
    }
    else
    {
        for(i=0;i<KYBER_N;i++)
            r->coeffs[i] = barrett_reduce(a->coeffs[i] + 3*KYBER_Q - b->coeffs[i]);
    }
}

/*************************************************
* Name:        poly_frommsg
*
* Description: Convert 32-byte message to polynomial
*
* Arguments:   - poly *r:                  pointer to output polynomial
*              - const unsigned char *msg: pointer to input message
**************************************************/
void poly_frommsg(poly *r, const unsigned char msg[KYBER_SYMBYTES], int protection_level)
{
  uint16_t i,j,mask;
  uint8_t temp, random_index;
  uint8_t temp_i, temp_j;

    if(protection_level == 1)
    {
        // Shuffling for every stage...
        for (i = 256-1; i >= 1; --i)
        {
            randombytes(&random_index,1);
            random_index = random_index%(i + 1);
            temp = ordered_array_256[i];
            ordered_array_256[i] = ordered_array_256[random_index];
            ordered_array_256[random_index] = temp;
        }

        for(i=0;i<KYBER_SYMBYTES;i++)
        {
            for(j=0;j<8;j++)
            {
                temp = ordered_array_256[8*i+j];
                temp_i = temp>>3;
                temp_j = temp&0x7;
              //gpio_set(GPIOA,GPIO1);
              mask = -((msg[temp_i] >> temp_j)&1);
              //gpio_clear(GPIOA,GPIO1);
              r->coeffs[temp] = mask & ((KYBER_Q+1)/2);
            }
        }
    }
    else
    {
        for(i=0;i<KYBER_SYMBYTES;i++)
        {
            for(j=0;j<8;j++)
            {
              //gpio_set(GPIOA,GPIO1);
              mask = -((msg[i] >> j)&1);
              //gpio_clear(GPIOA,GPIO1);
              r->coeffs[8*i+j] = mask & ((KYBER_Q+1)/2);
            }
        }
    }
}

/*************************************************
* Name:        poly_tomsg
*
* Description: Convert polynomial to 32-byte message
*
* Arguments:   - unsigned char *msg: pointer to output message
*              - const poly *a:      pointer to input polynomial
**************************************************/
void poly_tomsg(unsigned char msg[KYBER_SYMBYTES], const poly *a, int protection_level)
{
  uint16_t t;
  int i,j;
  uint8_t temp, random_index;
  uint8_t temp_i, temp_j;

    if(protection_level == 1)
    {
        // Shuffling for every stage...
        for (i = 256-1; i >= 1; --i)
        {
          randombytes(&random_index,1);
          random_index = random_index%(i + 1);
          temp = ordered_array_256[i];
          ordered_array_256[i] = ordered_array_256[random_index];
          ordered_array_256[random_index] = temp;
        }

        for(i=0;i<KYBER_SYMBYTES;i++)
        {
            msg[i] = 0;
        }

        for(i=0;i<KYBER_SYMBYTES;i++)
        {
            // msg[i] = 0;
            for(j=0;j<8;j++)
            {
                temp = ordered_array_256[8*i+j];
                temp_i = temp>>3;
                temp_j = temp&0x7;
                t = (((freeze(a->coeffs[temp]) << 1) + KYBER_Q/2)/KYBER_Q) & 1;
                msg[temp_i] |= t << temp_j;
            }
        }
    }
    else
    {
        for(i=0;i<KYBER_SYMBYTES;i++)
        {
            msg[i] = 0;
            for(j=0;j<8;j++)
            {
                t = (((freeze(a->coeffs[8*i+j]) << 1) + KYBER_Q/2)/KYBER_Q) & 1;
                msg[i] |= t << j;
            }
        }
    }
}
