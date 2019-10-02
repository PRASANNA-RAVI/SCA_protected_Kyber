#include "api.h"
#include "randombytes.h"
#include "stm32wrapper.h"
#include <string.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/vector.h>
// #include "ntt.h"
// #include "../lib/cm3/vector.c"
/* load optional platform dependent initialization routines */
//#include "../lib/dispatch/vector_chipset.c"
/* load the weak symbols for IRQ_HANDLERS */
//#include "../lib/dispatch/vector_nvic.c"

#define NTESTS 100

static uint8_t ordered_array[128] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
    70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
    95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
    116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127};

// static unsigned long long overflowcnt = 0;
// unsigned int t0, t1;
//
// void sys_tick_handler(void)
// {
//   ++overflowcnt;
// }
//
// static void printcycles(const char *s, unsigned long long c)
// {
//   char outs[32];
//   snprintf(outs,sizeof(outs),"%llu\n",c);
//   send_USART_str(outs);
// }

// void extern HardFault_Handler_C(unsigned long * svc_args, unsigned int lr_value);

static int test_keys(void)
{
  unsigned char key_a[CRYPTO_BYTES], key_b[CRYPTO_BYTES];
  unsigned char pk[CRYPTO_PUBLICKEYBYTES];
  unsigned char sendb[CRYPTO_CIPHERTEXTBYTES];
  unsigned char sk_a[CRYPTO_SECRETKEYBYTES];
  int i;
  uint8_t temp, random_index;

  // for(i=0; i<NTESTS; i++)
  // {
    //Alice generates a public key
    // t0 = systick_get_value();
    // overflowcnt = 0;
    crypto_kem_keypair(pk, sk_a);
    // t1 = systick_get_value();
    // printcycles("", (t0+overflowcnt*2400000llu)-t1);
    //send_USART_str("DONE key pair generation!");

    //Bob derives a secret key and creates a response
    // t0 = systick_get_value();
    // overflowcnt = 0;
    crypto_kem_enc(sendb, key_b, pk);
    // t1 = systick_get_value();
    // printcycles("", (t0+overflowcnt*2400000llu)-t1);
    //send_USART_str("DONE encapsulation!");

    //Alice uses Bobs response to get her secret key
    // t0 = systick_get_value();
    // overflowcnt = 0;
    crypto_kem_dec(key_a, sendb, sk_a);
    // t1 = systick_get_value();
    // printcycles("", (t0+overflowcnt*2400000llu)-t1);
    //send_USART_str("DONE decapsulation!");

    // t0 = systick_get_value();
    // overflowcnt = 0;
    // for (int i = 128-1; i >= 1; --i)
    // {
    //     randombytes(&random_index,1);
    //     random_index = random_index%(i + 1);
    //     temp = ordered_array[i];
    //     ordered_array[i] = ordered_array[random_index];
    //     ordered_array[random_index] = temp;
    // }
    // t1 = systick_get_value();
    // printcycles("", (t0+overflowcnt*2400000llu)-t1);

    if(memcmp(key_a, key_b, CRYPTO_BYTES))
    {
      send_USART_str("ERROR KEYS\n");
    }
    else
    {
      send_USART_str("OK KEYS\n");
    }
  // }
      return 0;
}

static int test_invalid_sk_a(void)
{
  unsigned char sk_a[CRYPTO_SECRETKEYBYTES];
  unsigned char key_a[CRYPTO_BYTES], key_b[CRYPTO_BYTES];
  unsigned char pk[CRYPTO_PUBLICKEYBYTES];
  unsigned char sendb[CRYPTO_CIPHERTEXTBYTES];
  int i;

  for(i=0; i<NTESTS; i++)
  {
    //Alice generates a public key
    crypto_kem_keypair(pk, sk_a);

    //Bob derives a secret key and creates a response
    crypto_kem_enc(sendb, key_b, pk);

    //Replace secret key with random values
    randombytes(sk_a, CRYPTO_SECRETKEYBYTES);

    //Alice uses Bobs response to get her secre key
    crypto_kem_dec(key_a, sendb, sk_a);

    if(!memcmp(key_a, key_b, CRYPTO_BYTES))
    {
      send_USART_str("ERROR invalid sk_a\n");
    }
    else
    {
      send_USART_str("OK invalid sk_a\n");
    }
  }

  return 0;
}


static int test_invalid_ciphertext(void)
{
  unsigned char sk_a[CRYPTO_SECRETKEYBYTES];
  unsigned char key_a[CRYPTO_BYTES], key_b[CRYPTO_BYTES];
  unsigned char pk[CRYPTO_PUBLICKEYBYTES];
  unsigned char sendb[CRYPTO_CIPHERTEXTBYTES];
  int i;
  size_t pos;

  for(i=0; i<NTESTS; i++)
  {
    randombytes((unsigned char *)&pos, sizeof(size_t));

    //Alice generates a public key
    crypto_kem_keypair(pk, sk_a);

    //Bob derives a secret key and creates a response
    crypto_kem_enc(sendb, key_b, pk);

    //Change some byte in the ciphertext (i.e., encapsulated key)
    sendb[pos % CRYPTO_CIPHERTEXTBYTES] ^= 23;

    //Alice uses Bobs response to get her secret key
    crypto_kem_dec(key_a, sendb, sk_a);

    if(!memcmp(key_a, key_b, CRYPTO_BYTES))
    {
      send_USART_str("ERROR invalid ciphertext\n");
    }
    else
    {
      send_USART_str("OK invalid ciphertext\n");
    }
  }

  return 0;
}

// HardFault handler wrapper in assembly language.
// It extracts the location of stack frame and passes it to the handler written
// in C as a pointer. We also extract the LR value as second parameter.
void blocking_handler(void)
{
	__asm(
	"MOV    R0,R13" "\n\t"
    "MOV    R1,R14" "\n\t"
	"BKPT" "\n\t"
	);
	while (1);
}

//
// // __asm void blocking_handler(void)
// // {
// // 	ANDS   R11, LR, #4
// // 	//ITE    EQ
// // 	BEQ    EQUAL
// // 	BNE    NOT_EQUAL
// // 	EQUAL:
// // 	MRS  R0, MSP
// // 	B      NORMAL
// // 	NOT_EQUAL:
// // 	MRS  R0, PSP
// // 	NORMAL:
// // 	MOV    R1, LR
// // 	B      HardFault_Handler_C
// // }
//
// HardFault handler in C, with stack frame location and LR value extracted
// from the assembly wrapper as input parameters
// void extern HardFault_Handler_C(unsigned long * hardfault_args, unsigned int lr_value)
// {
//   unsigned long stacked_r0;
//   unsigned long stacked_r1;
//   unsigned long stacked_r2;
//   unsigned long stacked_r3;
//   unsigned long stacked_r12;
//   unsigned long stacked_lr;
//   unsigned long stacked_pc;
//   unsigned long stacked_psr;
//   unsigned long cfsr;
//   unsigned long bus_fault_address;
//   unsigned long memmanage_fault_address;
//
//   #if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
//   bus_fault_address       = SCB_BFAR;
//   memmanage_fault_address = SCB_MMFAR;
//   cfsr                    = SCB_CFSR;
//   // #endif
//   // stacked_r0  = ((unsigned long) hardfault_args[0]);
//   // stacked_r1  = ((unsigned long) hardfault_args[1]);
//   // stacked_r2  = ((unsigned long) hardfault_args[2]);
//   // stacked_r3  = ((unsigned long) hardfault_args[3]);
//   // stacked_r12 = ((unsigned long) hardfault_args[4]);
//   // stacked_lr  = ((unsigned long) hardfault_args[5]);
//   // stacked_pc  = ((unsigned long) hardfault_args[6]);
//   // stacked_psr = ((unsigned long) hardfault_args[7]);
//
//   // send_USART_str("[HardFault]");
//   // printf ("[HardFault]\n");
//   // printf ("- Stack frame:\n");
//   // printf (" R0  = %x\n", stacked_r0);
//   // printf (" R1  = %x\n", stacked_r1);
//   // printf (" R2  = %x\n", stacked_r2);
//   // printf (" R3  = %x\n", stacked_r3);
//   // printf (" R12 = %x\n", stacked_r12);
//   // printf (" LR  = %x\n", stacked_lr);
//   // printf (" PC  = %x\n", stacked_pc);
//   // printf (" PSR = %x\n", stacked_psr);
//   // printf ("- FSR/FAR:\n");
//   // printf (" CFSR = %x\n", cfsr);
//   // printf (" HFSR = %x\n", SCB->HFSR);
//   // printf (" DFSR = %x\n", SCB->DFSR);
//   // printf (" AFSR = %x\n", SCB->AFSR);
// 	// if (cfsr & 0x0080) printf (" MMFAR = %x\n", memmanage_fault_address);
// 	// if (cfsr & 0x8000) printf (" BFAR = %x\n", bus_fault_address);
//   // printf ("- Misc\n");
//   // printf (" LR/EXC_RETURN= %x\n", lr_value);
//
//   while(1); // endless loop
// }

void main(void)
{
  clock_setup(CLOCK_BENCHMARK);
  gpio_setup();
  usart_setup(115200);
  rng_enable();
  systick_setup();

  SCB_SHCSR = SCB_SHCSR & (~SCB_SHCSR_USGFAULTENA);
  unsigned char recv_byte_start;

  // marker for automated testing
  // send_USART_str("==========================");
  // gpio_set(GPIOA, GPIO7);
  // chaskey(tag, taglen, recv_data_m, mlen, k, k1, k2);
  // send_USART_bytes(&tag,taglen);
  // gpio_clear(GPIOA, GPIO7);
  //test_keys();

  while(1)
  {
      recv_USART_bytes(&recv_byte_start,1);
      if(recv_byte_start == 'S')
      {
          for(int i=0;i<1;i++)
            test_keys();
          //gpio_set(GPIOA,GPIO7);
          //gpio_clear(GPIOA,GPIO7);
      }
  }

  //test_invalid_sk_a();
  //test_invalid_ciphertext();
  // send_USART_str("#");

  // while(1);

  return 0;
}
