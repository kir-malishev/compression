#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


unsigned int *get_arr(FILE *f)
{
    unsigned int *arr = calloc(257, sizeof(unsigned int));
    int c;
    while ((c = fgetc(f)) != EOF) {
        arr[c + 1]++;
    }
    fseek(f, 0, SEEK_SET);
    return arr;
}

unsigned int get_b(const unsigned int *arr, int j)
{
    unsigned int b = 0;
    for (int i = 0; i <= j; i++) {
        b += arr[i];
    }
    return b;
}

void bits_plus_follow(FILE *f, unsigned long long int bits_to_follow, int bit)
{
    static unsigned char st = 0;
    static int i = 7;
    if (bit == -1) {
        //st |= (1 << i) - 1;
        fputc(st, f);
        return;
    }
    st ^= (-(bit) ^ st) & (1UL << i);
    if (i) {
        i--;
    } else {
        fputc(st, f);
        i = 7;
    }
    for (; bits_to_follow > 0; bits_to_follow--) {
        st ^= (-(!bit) ^ st) & (1UL << i);
        if (i) {
            i--;
        } else {
            fputc(st, f);
            i = 7;
        }
    }
}



unsigned long long int round_div(unsigned long long int x, unsigned long long int y)
{
    return (x + (y / 2)) / y;
}

#define COUNT 32
//const unsigned COUNT = 40;
const unsigned long long int MAX_CODE = (1ULL << COUNT) - 1;

void compress(FILE *f, FILE *out)
{

    fseek(f, 0, SEEK_END); // seek to end of file
    unsigned long long int size = (unsigned long long) ftell(f); // get current file pointer
    fseek(f, 0, SEEK_SET);
    unsigned  int *arr = get_arr(f);
    fwrite(arr, sizeof(unsigned int), 257, out);
    for (int k = 0; k < 256; k++) {
        printf("%c %u\n", k, arr[k + 1]);
    }
    unsigned long long int high = MAX_CODE;
    unsigned long long int low = 0;
    unsigned long long pending_bits = 0;
    unsigned long long int First_qtr = (high +1)/4;
    unsigned long long int Half = First_qtr*2; // = 16384 = 32768
    unsigned long long int Third_qtr = First_qtr*3;
    //unsigned long long int First_qtr = 0x40000000;
    int c;
    while ((c = fgetc(f)) != EOF) {
        int j = c + 1;

        unsigned long long int range =  high - low + 1;
        //prob p = model.getProbability(c);
        //        //i++; // Находим его индекс
//        int a = get_b(arr, j - 1);
//        int b = get_b(arr, j);
//        unsigned long long c = get_b(arr, j - 1) * range;
//        unsigned long long d = (range * get_b(arr, j - 1))/size;
//        unsigned long long e = (range * get_b(arr, j))/size;
        high = low + range * get_b(arr, j)/size - 1;
        low = low +  range * get_b(arr, j - 1)/size;
        for ( ; ; ) {
            if ( high < Half ) {
                bits_plus_follow(out, pending_bits, 0);
                pending_bits = 0;
//                low <<= 1;
//                high <<= 1;
//                high |= 1;
            } else if ( low >= Half ) {
                bits_plus_follow(out, pending_bits, 1);
                pending_bits = 0;
                low -= Half;
                high -= Half;
                //low -= 0x80000000U;
                //high -= 0x80000000U;
//                low <<= 1;
//                high <<= 1;
//                high |= 1;
            }
            else if ( low >= First_qtr && high < Third_qtr ) {
                pending_bits++;
                low -= First_qtr;
                high -= First_qtr;
//                low <<= 1;
//                low &= 0x7FFFFFFF;
//                high <<= 1;
//                high |= 0x80000001;
            }
            else
                break;
            high <<= 1;
            high++;
            low <<= 1;
            high &= MAX_CODE;
            low &= MAX_CODE;
            //low &= 0x7FFFFFFF;
            //high |= 0x80000001;
//            low = (unsigned) low;
//            high = (unsigned) high;
        }
    }
    pending_bits++;
    if ( low < First_qtr)
        bits_plus_follow(out, pending_bits, 0);
    else
        bits_plus_follow(out, pending_bits, 1);
    bits_plus_follow(out, 0, -1);
    free(arr);
    fseek(out, 0, SEEK_SET);

//    fseek(f, 0, SEEK_END); // seek to end of file
//    unsigned long long int size = (unsigned long long) ftell(f); // get current file pointer
//    fseek(f, 0, SEEK_SET);
//    unsigned long long int *arr = get_arr(f);
//    fwrite(arr, sizeof(unsigned long long int), 257, out);
//    for (int k = 0; k < 256; k++) {
//        //printf("%c %lld\n", k, arr[k + 1]);
//    }
//    unsigned long long int l = 0;
//    unsigned long long int h = 0xFFFFFFFFU - 1;
//    unsigned long long int i = 0;
//    unsigned long long int delitel = size;
//    unsigned long long int First_qtr = 0x40000000; //(h + 1) / 4;
//    unsigned long long int Half = First_qtr * 2; // = 16384 = 32768
//    unsigned long long int Third_qtr = First_qtr * 3;
//    unsigned long long int bits_to_follow = 0; // = 49152, Сколько бит сбрасывать
//    int c;
//    while ((c = fgetc(f)) != EOF) {
//        //printf("%lld\n", i);
//        int j = c + 1;
//        unsigned long long long long range = h - l + 1;
//        //i++; // Находим его индекс
//        int a = get_b(arr, j - 1);
//        int b = get_b(arr, j);
//        unsigned long long long long c = get_b(arr, j - 1) * range;
//        l = l + ((unsigned long long long long) get_b(arr, j - 1)) * range / delitel;
//        h = l + ((unsigned long long long long) get_b(arr, j)) * range / delitel - 1;
//        for (; ;) { // Обрабатываем варианты
//            if (h < Half) { // переполнения
//                bits_plus_follow(out, bits_to_follow, 0);
//                bits_to_follow = 0;
//                l <<= 1;
//                h <<= 1;
//                h |= 1;
//            } else if (l >= Half) {
//                bits_plus_follow(out, bits_to_follow, 1);
//                bits_to_follow = 0;
//                l <<= 1;
//                h <<= 1;
//                h |= 1;
//                //li -= Half;
//                //hi -= Half;
//            } else if ((h < Third_qtr) && (l >= First_qtr)){
//                bits_to_follow++;
//                l <<= 1;
//                l &= 0x7FFFFFFF;
//                h <<= 1;
//                h |= 0x80000001;
////                l -= First_qtr;
////                h -= First_qtr;
//            } else {
//                break;
//            }
////            l += l;
////            h += h + 1;
//        }
//    }
//    bits_plus_follow(out, 0, -1);
//    free(arr);
//    fseek(out, 0, SEEK_SET);
}

int read_bit(FILE *f)
{
    static unsigned char st = 0;
    static int i = 0;
    if (i == 0) {
        if (fread(&st, sizeof(st), 1, f) < 1) {
            st = 0;
        }
        i = 7;
        return (st >> i) & 1;
    } else {
        i--;
        return (st >> i) & 1;
    }
}

void write_bit(FILE *f, int bit)
{
    static char st = 0;
    static int i = 7;
    if (bit == -1) {
        fputc(st, f);
        return;
    }
    st ^= (-(bit) ^ st) & (1UL << i);
    if (i) {
        i--;
    } else {
        fputc(st, f);
        i = 7;
    }
}


void decompress(FILE *f, FILE *out)
{
    unsigned long long int high = MAX_CODE;
    unsigned long long int low = 0;
    unsigned long long int First_qtr = (high +1)/4;
    unsigned long long int Half = First_qtr*2; // = 16384 = 32768
    unsigned long long int Third_qtr = First_qtr*3;
    //unsigned long long int low = 0;
    //unsigned long long int h = 65535;
    //unsigned long long int high = 0xFFFFFFFFU;
    //unsigned long long int i = 0;
    unsigned int *arr = calloc(257, sizeof(unsigned int));
    fread(arr, sizeof(unsigned int), 257, f);
    for (int k = 0; k < 256; k++) {
        printf("%c %u\n", k, arr[k + 1]);
    }
    unsigned long long int size = get_b(arr, 256);
    unsigned long long int delitel = size;
//    unsigned long long int First_qtr = 0x40000000;   // = 16384
//    unsigned long long int Half = First_qtr * 2;        // = 32768
//    unsigned long long int Third_qtr = First_qtr * 3;    // = 49152
    unsigned char val;
    uint64_t value = 0;
    for (int i = 0; i < COUNT; i++) {
        value <<= 1;
        int b = read_bit(f);
        value += b;
    }
//    for (int k = 0; k < COUNT/8; k++) {
//        value <<= 8;
//        if (fread(&val, sizeof(val), 1, f) < 1) {
//            val = 0;
//        }
//        value |= val;
//    }
//    unsigned long long char val = 0;
//    int a = fread(&val, sizeof(val), 1, f);
//    uint16_t value = val << 8;
//    if (fread(&val, sizeof(val), 1, f) < 1) {
//        val = 0;
//    }
//    value |= val;
    while (size--) {
        unsigned long long range = high - low + 1;
        unsigned long long freq = ((value - low + 1)*delitel - 1)/range;
        //unsigned long long int freq = round_div(((value - l + 1) * delitel - 1), (h - l + 1));
//        if (freq == -1) {
//            freq = 0;
//        }
        int j;
        for (j = 1; get_b(arr, j) <= freq; j++);
        char c = (char) (j - 1);
        //if (j == 257) break;
        //i++; // Находим его индекс
        int a = get_b(arr, j - 1);
        int b = get_b(arr, j);
        int d = get_b(arr, j) * range / delitel;
        high = low + get_b(arr, j) * range / delitel - 1;
        low = (low + get_b(arr, j - 1) * range / delitel);
        for (;;) {
            if ( high < Half ) {
                //do nothing, bit is a zero
            } else if ( low >= Half ) {
                value -= Half;  //subtract one half from all three code values
                low -= Half;
                high -= Half;
            } else if ( low >= First_qtr && high < Third_qtr) {
                value -= First_qtr;
                low -= First_qtr;
                high -= First_qtr;
            } else
                break;
            low <<= 1;
            high <<= 1;
            high++;
            value <<= 1;
//            low = (unsigned) low;
//            high = (unsigned) high;
            value += read_bit(f);
            value &= MAX_CODE;
            low &= MAX_CODE;
            high &= MAX_CODE;
//            if (low >= 0x80000000U || high < 0x80000000U) {
//                l <<= 1;
//                h <<= 1;
//                h |= 1;
//                value += read_bit(f);
//            } else if (l >= 0x40000000 && h < 0xC0000000U) {
//                l <<= 1;
//                l &= 0x7FFFFFFF;
//                h <<= 1;
//                h |= 0x80000001;
//                value += read_bit(f);
//            } else
//                break;
//            l = (unsigned) l;
//            h = (unsigned) h;
        }
        fputc(c, out);
        //printf("%d %c\n", c, c);
    }
    free(arr);
}


int main(void)
{
    FILE *in = fopen("in.pdf", "r");
    FILE *out = fopen("out.bin", "w+");
    FILE *out1 = fopen("out1.pdf", "w");
    compress(in, out);
    decompress(out, out1);
    return 0;
}
