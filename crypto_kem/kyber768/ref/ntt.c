#include "inttypes.h"
#include "ntt.h"
#include "params.h"
#include "reduce.h"
#include "randombytes.h"
#include "stm32wrapper.h"
#include "api.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

extern uint8_t ordered_array_256[];

// Calculate reversebits of a number (Count means the number of bits in the number)
static int reverseBits(int num, int count)
{
    int mask = 0x1FF;
    num = num & mask;
    int reverse_num = 0;
    char bit=0;
    for(int i=count-1;i>=0;i--)
    {
        bit=num&0b1;
        reverse_num = reverse_num + (bit<<i);
        num >>=1;
    }
    return reverse_num;
}

static unsigned long long overflowcnt = 0;
unsigned int t0, t1;

// void sys_tick_handler(void)
// {
//   ++overflowcnt;
// }

// void sys_tick_handler(void)
// {
//   ++overflowcnt;
// }

static void printcycles(const char *s, unsigned long long c)
{
  char outs[32];
  snprintf(outs,sizeof(outs),"%llu\n",c);
  send_USART_str(outs);
}

extern const uint16_t omegas_inv_bitrev_montgomery[];
extern const uint16_t psis_inv_montgomery[];
extern const uint16_t zetas[];
extern const uint16_t zetas_full_bitrev_montgomery[];
extern const uint16_t zetas_full_normal_order_montgomery[];
extern const uint16_t omegas_inv_bitrev_montgomery[];
extern const uint16_t omegas_inv_full_normal_order_montgomery[];
extern const uint16_t psis_inv_montgomery_full[];
extern const uint16_t inv_psis_montgomery_normal_order[];

uint8_t ordered_array[128] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
    70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94,
    95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
    116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127};

uint8_t index_array[8][128] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239},
    {0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 32, 33, 34, 35, 36, 37, 38, 39, 48, 49, 50, 51, 52, 53, 54, 55, 64, 65, 66, 67, 68, 69, 70, 71, 80, 81, 82, 83, 84, 85, 86, 87, 96, 97, 98, 99, 100, 101, 102, 103, 112, 113, 114, 115, 116, 117, 118, 119, 128, 129, 130, 131, 132, 133, 134, 135, 144, 145, 146, 147, 148, 149, 150, 151, 160, 161, 162, 163, 164, 165, 166, 167, 176, 177, 178, 179, 180, 181, 182, 183, 192, 193, 194, 195, 196, 197, 198, 199, 208, 209, 210, 211, 212, 213, 214, 215, 224, 225, 226, 227, 228, 229, 230, 231, 240, 241, 242, 243, 244, 245, 246, 247},
    {0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27, 32, 33, 34, 35, 40, 41, 42, 43, 48, 49, 50, 51, 56, 57, 58, 59, 64, 65, 66, 67, 72, 73, 74, 75, 80, 81, 82, 83, 88, 89, 90, 91, 96, 97, 98, 99, 104, 105, 106, 107, 112, 113, 114, 115, 120, 121, 122, 123, 128, 129, 130, 131, 136, 137, 138, 139, 144, 145, 146, 147, 152, 153, 154, 155, 160, 161, 162, 163, 168, 169, 170, 171, 176, 177, 178, 179, 184, 185, 186, 187, 192, 193, 194, 195, 200, 201, 202, 203, 208, 209, 210, 211, 216, 217, 218, 219, 224, 225, 226, 227, 232, 233, 234, 235, 240, 241, 242, 243, 248, 249, 250, 251},
    {0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29, 32, 33, 36, 37, 40, 41, 44, 45, 48, 49, 52, 53, 56, 57, 60, 61, 64, 65, 68, 69, 72, 73, 76, 77, 80, 81, 84, 85, 88, 89, 92, 93, 96, 97, 100, 101, 104, 105, 108, 109, 112, 113, 116, 117, 120, 121, 124, 125, 128, 129, 132, 133, 136, 137, 140, 141, 144, 145, 148, 149, 152, 153, 156, 157, 160, 161, 164, 165, 168, 169, 172, 173, 176, 177, 180, 181, 184, 185, 188, 189, 192, 193, 196, 197, 200, 201, 204, 205, 208, 209, 212, 213, 216, 217, 220, 221, 224, 225, 228, 229, 232, 233, 236, 237, 240, 241, 244, 245, 248, 249, 252, 253},
    {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190, 192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254}
};

uint8_t twiddle_factor_index_array[8][128] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3},
    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7},
    {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15},
    {16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31},
    {32, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 37, 38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 41, 41, 41, 41, 42, 42, 42, 42, 43, 43, 43, 43, 44, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 47, 47, 47, 47, 48, 48, 48, 48, 49, 49, 49, 49, 50, 50, 50, 50, 51, 51, 51, 51, 52, 52, 52, 52, 53, 53, 53, 53, 54, 54, 54, 54, 55, 55, 55, 55, 56, 56, 56, 56, 57, 57, 57, 57, 58, 58, 58, 58, 59, 59, 59, 59, 60, 60, 60, 60, 61, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63, 63},
    {64, 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 73, 74, 74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 79, 80, 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 88, 88, 89, 89, 90, 90, 91, 91, 92, 92, 93, 93, 94, 94, 95, 95, 96, 96, 97, 97, 98, 98, 99, 99, 100, 100, 101, 101, 102, 102, 103, 103, 104, 104, 105, 105, 106, 106, 107, 107, 108, 108, 109, 109, 110, 110, 111, 111, 112, 112, 113, 113, 114, 114, 115, 115, 116, 116, 117, 117, 118, 118, 119, 119, 120, 120, 121, 121, 122, 122, 123, 123, 124, 124, 125, 125, 126, 126, 127, 127},
    {128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255}
};

uint8_t inverse_index_array[8][128] = {
    {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190, 192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254},
    {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61, 65, 69, 73, 77, 81, 85, 89, 93, 97, 101, 105, 109, 113, 117, 121, 125, 129, 133, 137, 141, 145, 149, 153, 157, 161, 165, 169, 173, 177, 181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253},
    {0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 1, 9, 17, 25, 33, 41, 49, 57, 65, 73, 81, 89, 97, 105, 113, 121, 129, 137, 145, 153, 161, 169, 177, 185, 193, 201, 209, 217, 225, 233, 241, 249, 2, 10, 18, 26, 34, 42, 50, 58, 66, 74, 82, 90, 98, 106, 114, 122, 130, 138, 146, 154, 162, 170, 178, 186, 194, 202, 210, 218, 226, 234, 242, 250, 3, 11, 19, 27, 35, 43, 51, 59, 67, 75, 83, 91, 99, 107, 115, 123, 131, 139, 147, 155, 163, 171, 179, 187, 195, 203, 211, 219, 227, 235, 243, 251},
    {0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 1, 17, 33, 49, 65, 81, 97, 113, 129, 145, 161, 177, 193, 209, 225, 241, 2, 18, 34, 50, 66, 82, 98, 114, 130, 146, 162, 178, 194, 210, 226, 242, 3, 19, 35, 51, 67, 83, 99, 115, 131, 147, 163, 179, 195, 211, 227, 243, 4, 20, 36, 52, 68, 84, 100, 116, 132, 148, 164, 180, 196, 212, 228, 244, 5, 21, 37, 53, 69, 85, 101, 117, 133, 149, 165, 181, 197, 213, 229, 245, 6, 22, 38, 54, 70, 86, 102, 118, 134, 150, 166, 182, 198, 214, 230, 246, 7, 23, 39, 55, 71, 87, 103, 119, 135, 151, 167, 183, 199, 215, 231, 247},
    {0, 32, 64, 96, 128, 160, 192, 224, 1, 33, 65, 97, 129, 161, 193, 225, 2, 34, 66, 98, 130, 162, 194, 226, 3, 35, 67, 99, 131, 163, 195, 227, 4, 36, 68, 100, 132, 164, 196, 228, 5, 37, 69, 101, 133, 165, 197, 229, 6, 38, 70, 102, 134, 166, 198, 230, 7, 39, 71, 103, 135, 167, 199, 231, 8, 40, 72, 104, 136, 168, 200, 232, 9, 41, 73, 105, 137, 169, 201, 233, 10, 42, 74, 106, 138, 170, 202, 234, 11, 43, 75, 107, 139, 171, 203, 235, 12, 44, 76, 108, 140, 172, 204, 236, 13, 45, 77, 109, 141, 173, 205, 237, 14, 46, 78, 110, 142, 174, 206, 238, 15, 47, 79, 111, 143, 175, 207, 239},
    {0, 64, 128, 192, 1, 65, 129, 193, 2, 66, 130, 194, 3, 67, 131, 195, 4, 68, 132, 196, 5, 69, 133, 197, 6, 70, 134, 198, 7, 71, 135, 199, 8, 72, 136, 200, 9, 73, 137, 201, 10, 74, 138, 202, 11, 75, 139, 203, 12, 76, 140, 204, 13, 77, 141, 205, 14, 78, 142, 206, 15, 79, 143, 207, 16, 80, 144, 208, 17, 81, 145, 209, 18, 82, 146, 210, 19, 83, 147, 211, 20, 84, 148, 212, 21, 85, 149, 213, 22, 86, 150, 214, 23, 87, 151, 215, 24, 88, 152, 216, 25, 89, 153, 217, 26, 90, 154, 218, 27, 91, 155, 219, 28, 92, 156, 220, 29, 93, 157, 221, 30, 94, 158, 222, 31, 95, 159, 223},
    {0, 128, 1, 129, 2, 130, 3, 131, 4, 132, 5, 133, 6, 134, 7, 135, 8, 136, 9, 137, 10, 138, 11, 139, 12, 140, 13, 141, 14, 142, 15, 143, 16, 144, 17, 145, 18, 146, 19, 147, 20, 148, 21, 149, 22, 150, 23, 151, 24, 152, 25, 153, 26, 154, 27, 155, 28, 156, 29, 157, 30, 158, 31, 159, 32, 160, 33, 161, 34, 162, 35, 163, 36, 164, 37, 165, 38, 166, 39, 167, 40, 168, 41, 169, 42, 170, 43, 171, 44, 172, 45, 173, 46, 174, 47, 175, 48, 176, 49, 177, 50, 178, 51, 179, 52, 180, 53, 181, 54, 182, 55, 183, 56, 184, 57, 185, 58, 186, 59, 187, 60, 188, 61, 189, 62, 190, 63, 191},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127}
};

uint8_t inverse_twiddle_factor_index_array[8][128] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7},
    {0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3},
    {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/*************************************************
* Name:        ntt
*
* Description: Computes negacyclic number-theoretic transform (NTT) of
*              a polynomial (vector of 256 coefficients) in place;
*              inputs assumed to be in normal order, output in bitreversed order
*
* Arguments:   - uint16_t *p: pointer to in/output polynomial
**************************************************/
void ntt(uint16_t *p, int protection_level)
{
  int level, start, j, k;
  uint16_t zeta, t, t1, zeta1;
  uint8_t random_index, temp;
  uint8_t stage = 0;
  uint16_t temp1;

    if(protection_level == 0)
    {
        #if PRINT_NTT == 1
        // t0 = systick_get_value();
        // overflowcnt = 0;
        #endif
        k = 1;
        for(level = 7; level >= 0; level--)
        {
        for(start = 0; start < KYBER_N; start = j + (1<<level))
        {
          zeta = zetas[k++];
          for(j = start; j < start + (1<<level); ++j)
          {
            t = montgomery_reduce((uint32_t)zeta * p[j + (1<<level)]);
            p[j + (1<<level)] = barrett_reduce(p[j] + 4*KYBER_Q - t);

            if(level & 1) /* odd level */
              p[j] = p[j] + t; /* Omit reduction (be lazy) */
            else
              p[j] = barrett_reduce(p[j] + t);
          }
        }
        }
        #if PRINT_NTT == 1
        // t1 = systick_get_value();
        // printcycles("", t0);
        // printcycles("", t1);
        // printcycles("", overflowcnt);
        // printcycles("", (t0+overflowcnt*2400000llu)-t1);
        #endif
    }

    else if(protection_level == 1)
    {
        #if PRINT_NTT == 1
        // send_USART_str("Shuffled NTT\n");
        // t0 = systick_get_value();
        // overflowcnt = 0;
        #endif

        // t0 = systick_get_value();
        // overflowcnt = 0;

        // Determining the shuffling order...

        for (int i = 128-1; i >= 1; --i)
        {
            randombytes(&random_index,1);
            random_index = random_index%(i + 1);
            temp = ordered_array[i];
            ordered_array[i] = ordered_array[random_index];
            ordered_array[random_index] = temp;
        }

        // t1 = systick_get_value();
        // printcycles("", (t0+overflowcnt*2400000llu)-t1);

        k = 1;
        stage = 0;
        for(level = 7; level >= 0; level--)
        {
            // If we uncomment this, we are shuffling for every stage of the NTT operation.

            // for (int i = 128-1; i >= 1; --i)
            // {
            //     randombytes(&random_index,1);
            //     random_index = random_index%(i + 1);
            //     temp = ordered_array[i];
            //     ordered_array[i] = ordered_array[random_index];
            //     ordered_array[random_index] = temp;
            // }
          for(start = 0; start < 128; start++)
          {
              temp = ordered_array[start];
              zeta = zetas[twiddle_factor_index_array[stage][temp]];
              temp = index_array[stage][temp];
               // zeta calculation to be done here...
               // zeta calculation to be done here...

               t = montgomery_reduce((uint32_t)zeta * p[temp + (1<<level)]);
               p[temp + (1<<level)] = barrett_reduce(p[temp] + 4*KYBER_Q - t);

               if(level & 1) /* odd level */
                 p[temp] = p[temp] + t; /* Omit reduction (be lazy) */
               else
                 p[temp] = barrett_reduce(p[temp] + t);
          }
          stage++;
        }

        #if PRINT_NTT == 1
        // t1 = systick_get_value();
        // printcycles("", t0);
        // printcycles("", t1);
        // printcycles("", overflowcnt);
        // printcycles("", (t0+overflowcnt*2400000llu)-t1);
        #endif
    }

    else if(protection_level == 2)
    {
        #if PRINT_NTT == 1
        // t0 = systick_get_value();
        // overflowcnt = 0;
        #endif

        uint8_t random_indices[2];

        // Performing side channel resistant Inverse NTT structure with different twiddle factors...

        uint16_t random_twiddle_exponent[8]; //Generating random exponents for each stage...
        uint16_t sum_random_twiddle_exponent = 0;

        int index1, index2;
        for(int i=0;i<7;i++)
        {
          randombytes(random_indices,2); // Sample from less than 512...
          random_twiddle_exponent[i] = (random_indices[1]*8+random_indices[0])&0x1FF;
          // printf("%d, ",random_twiddle_exponent[i]);
          sum_random_twiddle_exponent += random_twiddle_exponent[i];
        }

        random_twiddle_exponent[7] = (2*KYBER_N) - (sum_random_twiddle_exponent&0X1FF);

        //The sum of all the random twiddle factors much be equal to 2*paramN, since psi^{2*paramN} = 1.
        // The idea is that we multiple a random twiddle factor to each element after every stage. This is integrated into the butterfly operation and thus we get two modified twiddle factors...
        // ...for each butterfly operation. The final result is such that that the real output of the NTT is multiplied with sum of powers of the randomized twiddle factors that are used...
        // ....Thus a post-scaling step is required to remove the product of the random twiddle factors. But since we know that psi^{2*paramN} = 1, we ensure that the generated twiddle factors...
        // ...multiply to produce a 1, which removes the post-scaling step...

        k = 1;
        stage = 0;
        for(level = 7; level >= 0; level--)
        {
        for(start = 0; start < KYBER_N; start = j + (1<<level))
        {
            // index1 = reverseBits(((reverseBits(k,8) + random_twiddle_exponent[stage])&0x1FF),9);
            // index2 = reverseBits(random_twiddle_exponent[stage],9);
            index1 = (reverseBits(k,8) + random_twiddle_exponent[stage])&0x1FF; // Correct...
            // index1 = (k + random_twiddle_exponent[stage])&0x1FF; // Faulty...
            index2 = random_twiddle_exponent[stage];
          // zeta = zetas_full_bitrev_montgomery[index1];
          zeta = zetas_full_normal_order_montgomery[index1];
          zeta1 = zetas_full_normal_order_montgomery[index2];

          for(j = start; j < start + (1<<level); ++j)
          {
            t = montgomery_reduce((uint32_t)zeta * p[j + (1<<level)]);
            t1 = montgomery_reduce((uint32_t)zeta1 * p[j]);
            p[j + (1<<level)] = barrett_reduce(t1 + 4*KYBER_Q - t);

            if(level & 1) /* odd level */
              p[j] = t1 + t; /* Omit reduction (be lazy) */
            else
              p[j] = barrett_reduce(t1 + t);
          }
          k = k+1;
        }
        stage = stage+1;
        }
        #if PRINT_NTT == 1
        // t1 = systick_get_value();
        // printcycles("", t0);
        // printcycles("", t1);
        // printcycles("", overflowcnt);
        // printcycles("", (t0+overflowcnt*2400000llu)-t1);
        #endif
    }

    else if (protection_level == 3)
    {
        #if PRINT_NTT == 1
        // send_USART_str("Masked and Shuffled NTT\n");
        // t0 = systick_get_value();
        // overflowcnt = 0;
        #endif

        // Determining the shuffling order...shuffling only once per NTT operation... If this piece of code is inside
        // the outermost for loop, then we are having a new shuffling order for every stage of the NTT operation.
        for (int i = 128-1; i >= 1; --i)
        {
            randombytes(&random_index,1);
            random_index = random_index%(i + 1);
            temp = ordered_array[i];
            ordered_array[i] = ordered_array[random_index];
            ordered_array[random_index] = temp;
        }

        uint8_t random_indices[2];

        // Performing side channel resistant Inverse NTT structure with different twiddle factors...
        uint16_t random_twiddle_exponent[8]; //Generating random exponents for each stage...
        uint16_t sum_random_twiddle_exponent = 0;

        int index1, index2;
        for(int i=0;i<7;i++)
        {
            randombytes(random_indices,2); // Sample from less than 512...
            random_twiddle_exponent[i] = (random_indices[1]*8+random_indices[0])&0x1FF;
            // printf("%d, ",random_twiddle_exponent[i]);
            sum_random_twiddle_exponent += random_twiddle_exponent[i];
        }

        random_twiddle_exponent[7] = (2*KYBER_N) - (sum_random_twiddle_exponent&0X1FF);

        //The sum of all the random twiddle factors much be equal to 2*paramN, since psi^{2*paramN} = 1.
        // The idea is that we multiple a random twiddle factor to each element after every stage. This is integrated into the butterfly operation and thus we get two modified twiddle factors...
        // ...for each butterfly operation. The final result is such that that the real output of the NTT is multiplied with sum of powers of the randomized twiddle factors that are used...
        // ....Thus a post-scaling step is required to remove the product of the random twiddle factors. But since we know that psi^{2*paramN} = 1, we ensure that the generated twiddle factors...
        // ...multiply to produce a 1, which removes the post-scaling step...

        k = 1;
        stage = 0;
        // t0 = systick_get_value();
        // overflowcnt = 0;
        for(level = 7; level >= 0; level--)
        {
          for(start = 0; start < 128; start++)
          {
              temp = ordered_array[start];
              temp1 = twiddle_factor_index_array[stage][temp];
              index1 = (reverseBits(temp1,8) + random_twiddle_exponent[stage])&0x1FF; // Correct...
              // index1 = (k + random_twiddle_exponent[stage])&0x1FF; // Faulty...
              index2 = random_twiddle_exponent[stage];

              zeta = zetas_full_normal_order_montgomery[index1];
              zeta1 = zetas_full_normal_order_montgomery[index2];

              // zeta = zetas[twiddle_factor_index_array[stage][temp]];
              temp = index_array[stage][temp];

               t = montgomery_reduce((uint32_t)zeta * p[temp + (1<<level)]);
               t1 = montgomery_reduce((uint32_t)zeta1 * p[temp]);
               p[temp + (1<<level)] = barrett_reduce(t1 + 4*KYBER_Q - t);

               if(level & 1) /* odd level */
                 p[temp] = t1 + t; /* Omit reduction (be lazy) */
               else
                 p[temp] = barrett_reduce(t1 + t);
          }
          stage++;
        }
        #if PRINT_NTT == 1
        // t1 = systick_get_value();
        // printcycles("", t0);
        // printcycles("", t1);
        // printcycles("", overflowcnt);
        // printcycles("", (t0+overflowcnt*2400000llu)-t1);
        #endif
    }
}

/*************************************************
* Name:        invntt
*
* Description: Computes inverse of negacyclic number-theoretic transform (NTT) of
*              a polynomial (vector of 256 coefficients) in place;
*              inputs assumed to be in bitreversed order, output in normal order
*
* Arguments:   - uint16_t *a: pointer to in/output polynomial
**************************************************/
void invntt(uint16_t * a, int protection_level)
{
  int start, j, jTwiddle, level;
  uint16_t temp, W1, W2, W;
  uint32_t t;

  uint8_t random_index, temp2;
  uint8_t stage = 0;

    if(protection_level == 0)
    {
        #if PRINT_INTT == 1
        // t0 = systick_get_value();
        // overflowcnt = 0;
        #endif
        for(level=0;level<8;level++)
        {
            for(start = 0; start < (1<<level);start++)
            {
              jTwiddle = 0;
              for(j=start;j<KYBER_N-1;j+=2*(1<<level))
              {
                W = omegas_inv_bitrev_montgomery[jTwiddle++];
                temp = a[j];

                if(level & 1) /* odd level */
                  a[j] = barrett_reduce((temp + a[j + (1<<level)]));
                else
                  a[j] = (temp + a[j + (1<<level)]); /* Omit reduction (be lazy) */

                t = (W * ((uint32_t)temp + 4*KYBER_Q - a[j + (1<<level)]));

                a[j + (1<<level)] = montgomery_reduce(t);
              }
            }
        }

        for(j = 0; j < KYBER_N; j++)
        {
            a[j] = montgomery_reduce((a[j] * psis_inv_montgomery[j]));
        }

        #if PRINT_INTT == 1
        // t1 = systick_get_value();
        // printcycles("", t0);
        // printcycles("", t1);
        // printcycles("", overflowcnt);
        // printcycles("", (t0+overflowcnt*2400000llu)-t1);
        #endif
    }

    else if(protection_level == 1)
    {
        #if PRINT_INTT == 1
        // t0 = systick_get_value();
        // overflowcnt = 0;
        #endif

        // This decides the shuffling order... Here we only shuffle once per NTT operation
        for (int i = 128-1; i >= 1; --i)
        {
        randombytes(&random_index,1);
        random_index = random_index%(i + 1);
        temp2 = ordered_array[i];
        ordered_array[i] = ordered_array[random_index];
        ordered_array[random_index] = temp2;
        }

        stage = 0;
        for(level=0;level<8;level++)
        {
            // If we uncomment this, we are shuffling for every stage of the NTT operation.
            // for (int i = 128-1; i >= 1; --i)
            // {
            // randombytes(&random_index,1);
            // random_index = random_index%(i + 1);
            // temp2 = ordered_array[i];
            // ordered_array[i] = ordered_array[random_index];
            // ordered_array[random_index] = temp2;
            // }

          // Operations within every stage...
          for(start = 0; start < 128; start++)
          {
              temp2 = ordered_array[start];
              W = omegas_inv_bitrev_montgomery[inverse_twiddle_factor_index_array[stage][temp2]];
              temp2 = inverse_index_array[stage][temp2];
              temp = a[temp2];

              if(level & 1) /* odd level */
                a[temp2] = barrett_reduce((temp + a[temp2 + (1<<level)]));
              else
                a[temp2] = (temp + a[temp2 + (1<<level)]); /* Omit reduction (be lazy) */

              t = (W * ((uint32_t)temp + 4*KYBER_Q - a[temp2 + (1<<level)]));

              a[temp2 + (1<<level)] = montgomery_reduce(t);
          }
          stage++;
        }

        for(j = 0; j < KYBER_N; j++)
        a[j] = montgomery_reduce((a[j] * psis_inv_montgomery[j]));

        #if PRINT_INTT == 1
        // t1 = systick_get_value();
        // printcycles("", t0);
        // printcycles("", t1);
        // printcycles("", overflowcnt);
        // printcycles("", (t0+overflowcnt*2400000llu)-t1);
        #endif
    }

    else if(protection_level == 2)
    {
        uint8_t random_indices[2];

        #if PRINT_INTT == 1
        // send_USART_str("Masked INTT\n");
        // t0 = systick_get_value();
        // overflowcnt = 0;
        #endif
        // Performing side channel resistant Inverse NTT structure with different twiddle factors...
        uint16_t random_twiddle_exponent[9]; //Generating random exponents for each stage...
        uint16_t sum_random_twiddle_exponent = 0;

        int index1, index2;
        for(int i=0;i<8;i++)
        {
            randombytes(random_indices,2); // Sample from less than 512...
            random_twiddle_exponent[i] = (random_indices[1]*8+random_indices[0])&0x1FF;
            sum_random_twiddle_exponent += random_twiddle_exponent[i];
        }

        random_twiddle_exponent[8] = (2*KYBER_N) - (sum_random_twiddle_exponent&0X1FF);

        stage = 0;
        for(level=0;level<8;level++)
        {
        for(start = 0; start < (1<<level);start++)
        {
          jTwiddle = 0;
          for(j=start;j<KYBER_N-1;j+=2*(1<<level))
          {
              index1 = (2*reverseBits(jTwiddle,7) + random_twiddle_exponent[stage])&0x1FF; // Correct...
              index2 = random_twiddle_exponent[stage];

              W1 = inv_psis_montgomery_normal_order[index1]; // To change array...
              W2 = inv_psis_montgomery_normal_order[index2]; // To change array...

            temp = a[j];

            a[j] = montgomery_reduce((uint32_t)W2 *(temp + a[j + (1<<level)])); /* Omit reduction (be lazy) */
            t = (W1 * ((uint32_t)temp + 4*KYBER_Q - a[j + (1<<level)]));
            a[j + (1<<level)] = montgomery_reduce(t);
            jTwiddle++;
          }
        }
        stage++;
        }

        for(j = 0; j < KYBER_N; j++)
        {
            temp = (j + random_twiddle_exponent[8])&0x1FF;
            a[j] = montgomery_reduce((a[j] * psis_inv_montgomery_full[temp]));
        }

        #if PRINT_INTT == 1
        // t1 = systick_get_value();
        // printcycles("", t0);
        // printcycles("", t1);
        // printcycles("", overflowcnt);
        // printcycles("", (t0+overflowcnt*2400000llu)-t1);
        #endif
    }

    else if(protection_level == 3)
    {
        #if PRINT_INTT == 1
        // send_USART_str("Masked and Shuffled INTT\n");
        // t0 = systick_get_value();
        // overflowcnt = 0;
        #endif
        // Performing side channel resistant Inverse NTT structure with different twiddle factors...
        uint16_t random_twiddle_exponent[9]; //Generating random exponents for each stage...
        uint16_t sum_random_twiddle_exponent = 0;

        uint8_t random_indices[2];
        int index1, index2;
        for(int i=0;i<8;i++)
        {
            randombytes(random_indices,2); // Sample from less than 512...
            random_twiddle_exponent[i] = (random_indices[1]*8+random_indices[0])&0x1FF;
            sum_random_twiddle_exponent += random_twiddle_exponent[i];
        }

        random_twiddle_exponent[8] = (2*KYBER_N) - (sum_random_twiddle_exponent&0X1FF);

        for (int i = 128-1; i >= 1; --i)
        {
        randombytes(&random_index,1);
        random_index = random_index%(i + 1);
        temp2 = ordered_array[i];
        ordered_array[i] = ordered_array[random_index];
        ordered_array[random_index] = temp2;
        }

        stage = 0;
        for(level=0;level<8;level++)
        {
          // Operations within every stage...
          for(start = 0; start < 128; start++)
          {
              temp2 = ordered_array[start];
              index1 = (2*reverseBits(inverse_twiddle_factor_index_array[stage][temp2],7) + random_twiddle_exponent[stage])&0x1FF; // Correct...
              index2 = random_twiddle_exponent[stage];

              W1 = inv_psis_montgomery_normal_order[index1];
              W2 = inv_psis_montgomery_normal_order[index2];
              temp2 = inverse_index_array[stage][temp2];
              temp = a[temp2];

              a[temp2] = montgomery_reduce((uint32_t)W2 *(temp + a[temp2 + (1<<level)])); /* Omit reduction (be lazy) */
              t = (W1 * ((uint32_t)temp + 4*KYBER_Q - a[temp2 + (1<<level)]));
              a[temp2 + (1<<level)] = montgomery_reduce(t);
          }
          stage++;
        }

        // send_USART_str("Shuffled poly_sub\n");
        // Shuffling for every stage...
        for (int i = 256-1; i >= 1; --i)
        {
              randombytes(&random_index,1);
              random_index = random_index%(i + 1);
              temp = ordered_array_256[i];
              ordered_array_256[i] = ordered_array_256[random_index];
              ordered_array_256[random_index] = temp;
        }

        uint16_t temp3;
        for(j = 0; j < KYBER_N; j++)
        {
            temp = ordered_array_256[j];
            temp3 = (temp + random_twiddle_exponent[8])&0x1FF;
            a[temp] = montgomery_reduce((a[temp] * psis_inv_montgomery_full[temp3]));
        }

        #if PRINT_INTT == 1
        // t1 = systick_get_value();
        // printcycles("", t0);
        // printcycles("", t1);
        // printcycles("", overflowcnt);
        // printcycles("", (t0+overflowcnt*2400000llu)-t1);
        #endif
    }

    // else if(protection_level == 4)
    // {
    //     #if PRINT_INTT == 1
    //     // send_USART_str("Masked INTT\n");
    //     // t0 = systick_get_value();
    //     // overflowcnt = 0;
    //     #endif
    //     // Performing side channel resistant Inverse NTT structure with different twiddle factors...
    //     uint16_t random_twiddle_exponent[8]; //Generating random exponents for each stage...
    //     uint16_t sum_random_twiddle_exponent = 0;
    //
    //     int index1, index2;
    //     for(int i=0;i<7;i++)
    //     {
    //         randombytes(&random_index,1); // Sample from less than 512...
    //         random_twiddle_exponent[i] = random_index;
    //         sum_random_twiddle_exponent += random_twiddle_exponent[i];
    //     }
    //
    //     random_twiddle_exponent[7] = (KYBER_N) - (sum_random_twiddle_exponent&0XFF);
    //
    //     stage = 0;
    //     for(level=0;level<8;level++)
    //     {
    //     for(start = 0; start < (1<<level);start++)
    //     {
    //       jTwiddle = 0;
    //       for(j=start;j<KYBER_N-1;j+=2*(1<<level))
    //       {
    //           index1 = (reverseBits(jTwiddle,7) + random_twiddle_exponent[stage])&0xFF; // Correct...
    //           index2 = random_twiddle_exponent[stage];
    //
    //           W1 = omegas_inv_full_normal_order_montgomery[index1]; // To change array...
    //           W2 = omegas_inv_full_normal_order_montgomery[index2]; // To change array...
    //
    //         temp = a[j];
    //
    //         a[j] = montgomery_reduce((uint32_t)W2 *(temp + a[j + (1<<level)])); /* Omit reduction (be lazy) */
    //         t = (W1 * ((uint32_t)temp + 4*KYBER_Q - a[j + (1<<level)]));
    //         a[j + (1<<level)] = montgomery_reduce(t);
    //         jTwiddle++;
    //       }
    //     }
    //     stage++;
    //     }
    //
    //     for(j = 0; j < KYBER_N; j++)
    //     a[j] = montgomery_reduce((a[j] * psis_inv_montgomery[j]));
    //     #if PRINT_INTT == 1
    //     // t1 = systick_get_value();
    //     // printcycles("", t0);
    //     // printcycles("", t1);
    //     // printcycles("", overflowcnt);
    //     // printcycles("", (t0+overflowcnt*2400000llu)-t1);
    //     #endif
    // }
    //
    // else if(protection_level == 5)
    // {
    //     #if PRINT_INTT == 1
    //     // send_USART_str("Masked and Shuffled INTT\n");
    //     // t0 = systick_get_value();
    //     // overflowcnt = 0;
    //     #endif
    //     // Performing side channel resistant Inverse NTT structure with different twiddle factors...
    //     uint16_t random_twiddle_exponent[8]; //Generating random exponents for each stage...
    //     uint16_t sum_random_twiddle_exponent = 0;
    //
    //     int index1, index2;
    //     for(int i=0;i<7;i++)
    //     {
    //         randombytes(&random_index,1); // Sample from less than 512...
    //         random_twiddle_exponent[i] = random_index;
    //         sum_random_twiddle_exponent += random_twiddle_exponent[i];
    //     }
    //
    //     random_twiddle_exponent[7] = (KYBER_N) - (sum_random_twiddle_exponent&0XFF);
    //
    //     for (int i = 128-1; i >= 1; --i)
    //     {
    //     randombytes(&random_index,1);
    //     random_index = random_index%(i + 1);
    //     temp2 = ordered_array[i];
    //     ordered_array[i] = ordered_array[random_index];
    //     ordered_array[random_index] = temp2;
    //     }
    //
    //     stage = 0;
    //     for(level=0;level<8;level++)
    //     {
    //       // Operations within every stage...
    //       for(start = 0; start < 128; start++)
    //       {
    //           temp2 = ordered_array[start];
    //           index1 = (reverseBits(inverse_twiddle_factor_index_array[stage][temp2],7) + random_twiddle_exponent[stage])&0xFF; // Correct...
    //           index2 = random_twiddle_exponent[stage];
    //
    //           W1 = omegas_inv_full_normal_order_montgomery[index1];
    //           W2 = omegas_inv_full_normal_order_montgomery[index2];
    //           temp2 = inverse_index_array[stage][temp2];
    //           temp = a[temp2];
    //
    //           a[temp2] = montgomery_reduce((uint32_t)W2 *(temp + a[temp2 + (1<<level)])); /* Omit reduction (be lazy) */
    //           t = (W1 * ((uint32_t)temp + 4*KYBER_Q - a[temp2 + (1<<level)]));
    //           a[temp2 + (1<<level)] = montgomery_reduce(t);
    //       }
    //       stage++;
    //     }
    //
    //     for(j = 0; j < KYBER_N; j++)
    //     a[j] = montgomery_reduce((a[j] * psis_inv_montgomery[j]));
    //
    //     #if PRINT_INTT == 1
    //     // t1 = systick_get_value();
    //     // printcycles("", t0);
    //     // printcycles("", t1);
    //     // printcycles("", overflowcnt);
    //     // printcycles("", (t0+overflowcnt*2400000llu)-t1);
    //     #endif
    // }
}
