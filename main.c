#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>


#include "ari.h"



long long d_freq;

typedef struct Frequency
{
    unsigned long long char_to_index[256];
    unsigned long long index_to_char[257];
    unsigned long long freq[257];
    unsigned long long cum_freq[257];
    unsigned long long sym;
} Frequency;

int mod (int a, int b)
{
    if(b < 0) //you can check for b == 0 separately and do what you want
        return mod(a, -b);
    int ret = a % b;
    if(ret < 0)
        ret+=b;
    return ret;
}



void bits_plus_follow(FILE *f, unsigned long long int bits_to_follow, int bit)
{
    static unsigned char st = 0;
    static int i = 7;
    if (bit == -1 && st) {
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

void init(Frequency *fr) {
    for (unsigned i = 0; i < 257; i++)
    {
        fr->char_to_index[i] = i + 1;
        fr->index_to_char[i + 1] = i;
    }
    for (int i = 0; i < 257; i++)
    {
        fr->freq[i] = 1;
        fr->cum_freq[i] = 257U - i;
    }
    fr->freq[0] = 0;
}

void update(Frequency *fr, unsigned long long sym, long long d_freq) {
    unsigned long long ch_i, ch_symbol;
    unsigned long long cum;
    if (fr->cum_freq[0] >= MAX_CODE)
    {
        cum = 0;
        for (long long i = 256; i >= 0; i--)
        {
            fr->freq[i] = (fr->freq [i] + 1) / 2;
            fr->cum_freq[i] = cum;
            cum += fr->freq [i];
        }
    }

    unsigned long long i;
    for ( i = sym; fr->freq [i] == fr->freq [i - 1]; i--);
    if (i < sym) {
        ch_i = fr->index_to_char[i];
        ch_symbol = fr->index_to_char[sym];
        fr->index_to_char[i] = ch_symbol;
        fr->index_to_char[sym] = ch_i;
        fr->char_to_index[ch_i] = sym;
        fr->char_to_index[ch_symbol] = i;

    }
    unsigned long long int a = fr->cum_freq[256];
    unsigned long long b = i;
    fr->freq[i] += d_freq;

    while (i-- > 0)
    {
        fr->cum_freq[i] += d_freq;
    }
    if (fr->cum_freq[256] == 1001) {
        int c = 0;
    }
}

unsigned long long dist(const unsigned long long int a[257], const unsigned long long int b[257]) {
    unsigned long long res = 0;
    for (int i = 0; i < 257; i++) {
        res += (a[i] - b[i])*(a[i] - b[i]);
    }
    return res;
}

double expected(const unsigned long long int a[257]) {
    unsigned long long sum = 0;
    double exp = 0;
    for (int i = 0; i < 257; i++) {
        sum += a[i];
    }
    for (int i = 0; i < 257; i++) {
        exp += ((double) a[i]/sum)*i;
    }
    return exp;
}

void transform(Frequency *base_freq, unsigned long long buf_freq[][257], unsigned long long int buf_ind, unsigned long long int step) {
    for (unsigned long long int i = 1; i < 257; i++) {
        if (buf_freq[buf_ind][i] - buf_freq[(buf_ind + 1) % step][i] == 0 && base_freq->freq[i] != 1) {

            update(base_freq, i, -(base_freq->freq[i]/2));
        }
    }
}

void compress_ari(char *ifile, char *ofile) {

    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    /** PUT YOUR CODE HERE
      * implement an arithmetic encoding algorithm for compression
      * don't forget to change header file `ari.h`
    */
    fseek(ifp, 0, SEEK_END); // seek to end of file
    unsigned long long  int size = (unsigned long long int) ftell(ifp); // get current file pointer
    fseek(ifp, 0, SEEK_SET);
    if (size)
        fwrite(&size, sizeof(size), 1, ofp);
    /*unsigned char_to_index[256];
    unsigned index_to_char[257];
    unsigned freq[257];
    unsigned cum_freq[257];
    */
    Frequency base_freq;
    init(&base_freq);
    Frequency ctrl_freq;
    init(&ctrl_freq);
    double exps[10];
    for (int i = 0; i < 10; i++) {
        exps[i] = 128.5;
    }
    //unsigned long long int step = MIN(100, MAX(10, size/10));
    unsigned long long int step = 30;
    unsigned long long int buf_freq[step][257];
    unsigned long long int buf_ind = 0;
    for (int i = 0; i < step; i++) {
        memcpy(buf_freq[i], ctrl_freq.freq, 257*sizeof(*(ctrl_freq.freq)));
    }
    d_freq = START_D_FREQ;
    unsigned long long int high = MAX_CODE;
    unsigned long long int low = 0;
    unsigned long long int pending_bits = 0;
    unsigned long long int First_qtr = (high +1)/4;
    unsigned long long int Half = First_qtr*2; // = 16384 = 32768
    unsigned long long int Third_qtr = First_qtr*3;
    int c;
    while ((c = fgetc(ifp)) != EOF) {
        base_freq.sym = base_freq.char_to_index[c];
        unsigned long long int range =  high - low + 1;
        high = (unsigned long long) (low + ( range * base_freq.cum_freq[base_freq.sym - 1])/base_freq.cum_freq[0] - 1);
        low = (unsigned long long) (low + (range * base_freq.cum_freq [base_freq.sym])/base_freq.cum_freq[0]);
        for ( ; ; ) {
            if ( high < Half ) {
                bits_plus_follow(ofp, pending_bits, 0);
                pending_bits = 0;
            } else if ( low >= Half ) {
                bits_plus_follow(ofp, pending_bits, 1);
                pending_bits = 0;
                low -= Half;
                high -= Half;
            }
            else if ( low >= First_qtr && high < Third_qtr ) {
                pending_bits++;
                low -= First_qtr;
                high -= First_qtr;
            }
            else
                break;
            high <<= 1;
            high++;
            low <<= 1;
            high &= MAX_CODE;
            low &= MAX_CODE;
        }
        //printf("%lld ", d_freq);
        update(&base_freq, base_freq.sym, d_freq);
        update(&ctrl_freq, base_freq.sym, 1);
        memcpy(buf_freq[buf_ind], ctrl_freq.freq, 257*sizeof(*(ctrl_freq.freq)));
        //unsigned long long dist1 = dist(buf_freq[buf_ind], buf_freq[mod(buf_ind - 1, 10)]);
        //unsigned long long dist2 = dist(buf_freq[mod(buf_ind - 1, 10)], buf_freq[mod(buf_ind - 2, 10)]);
        //long long d_dist = dist1 - dist2;
        //printf("%lld ", d_freq);
        //exps[buf_ind] = expected(base_freq.freq);
        //transform(&base_freq, buf_freq, buf_ind, step);
        //if (fabs(exps[buf_ind] - exps[mod(buf_ind - 9, 10)]) >= MAX_EXP) {
        //d_freq += DD_FREQ;
        //init(&base_freq);
        //d_freq = MIN(MAX_FREQ, d_freq + DD_FREQ);aaaa
        //} else {

        //init(&base_freq);
        //d_freq += 2*DD_FREQ;
        //d_freq = MAX(1000, d_freq - 1);
        //}
        buf_ind = (buf_ind + 1) % step;
    }
    if (size) {
        pending_bits++;
        if (low < First_qtr)
            bits_plus_follow(ofp, pending_bits, 0);
        else
            bits_plus_follow(ofp, pending_bits, 1);
        bits_plus_follow(ofp, 0, -1);
    }

    fseek(ofp, 0, SEEK_SET);
    printf("\n\n\n\n");


    // This is an implementation of simple copying
//    size_t n, m;
//    unsigned char buff[8192];
//
//    do {
//        n = fread(buff, 1, sizeof buff, ifp);
//        if (n)
//            m = fwrite(buff, 1, n, ofp);
//        else
//            m = 0;
//    } while ((n > 0) && (n == m));

    fclose(ifp);
    fclose(ofp);

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


void decompress_ari(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");
    unsigned long long int size;
    fread(&size, sizeof(size), 1, ifp);
    unsigned long long int high = MAX_CODE;
    unsigned long long int low = 0;
    unsigned long long int First_qtr = (high +1)/4;
    unsigned long long int Half = First_qtr*2; // = 16384 = 32768
    unsigned long long int Third_qtr = First_qtr*3;
    Frequency base_freq;
    init(&base_freq);
    Frequency ctrl_freq;
    init(&ctrl_freq);

    double exps[10];
    for (int i = 0; i < 10; i++) {
        exps[i] = 128.5;
    }
    //unsigned long long int step = MIN(100, MAX(10, size/10));
    unsigned long long int step = 10;
    unsigned long long int buf_freq[step][257];
    unsigned long long int buf_ind = 0;
    for (int i = 0; i < step; i++) {
        memcpy(buf_freq[i], ctrl_freq.freq, 257*sizeof(*(ctrl_freq.freq)));
    }
    d_freq = START_D_FREQ;
    uint64_t value = 0;
    for (int i = 0; i < COUNT; i++) {
        value <<= 1;
        int b = read_bit(ifp);
        value += b;
    }
    while (size--) {
        unsigned long long range = high - low + 1;
        long double tmp = ((long double) (value - low + 1));
        long double tmp1 = (tmp * base_freq.cum_freq[0] - 1);
        unsigned long long frequency = (unsigned long long int) (tmp1 / range);
        if (frequency == 0) {
            int d = 7;
        }
        for (base_freq.sym = 1; base_freq.cum_freq[base_freq.sym] > frequency; base_freq.sym++);
        fputc((int) base_freq.index_to_char[base_freq.sym], ofp);
        printf("%d ", (int) base_freq.index_to_char[base_freq.sym]);
        //fputc(base_freq.index_to_char[base_freq.sym], stdout);
        high = low + (range * base_freq.cum_freq[base_freq.sym - 1])/base_freq.cum_freq[0] - 1;
        low = low + (range * base_freq.cum_freq [base_freq.sym])/base_freq.cum_freq [0];
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
            value += read_bit(ifp);
            value &= MAX_CODE;
            low &= MAX_CODE;
            high &= MAX_CODE;
        }
        //printf("%lld ", d_freq);
        update(&base_freq, base_freq.sym, d_freq);
        update(&ctrl_freq, base_freq.sym, 1LL);
        memcpy(buf_freq[buf_ind], ctrl_freq.freq, 257*sizeof(*(ctrl_freq.freq)));
        //unsigned long long dist1 = dist(buf_freq[buf_ind], buf_freq[mod(buf_ind - 1, 10)]);
        //unsigned long long dist2 = dist(buf_freq[mod(buf_ind - 1, 10)], buf_freq[mod(buf_ind - 2, 10)]);
        //long long d_dist = dist1 - dist2;
        //printf("%lld ", d_freq);
        //exps[buf_ind] = expected(base_freq.freq);
        //transform(&base_freq, buf_freq, buf_ind, step);
        //if (fabs(exps[buf_ind] - exps[mod(buf_ind - 9, 10)]) >= MAX_EXP) {
        //d_freq += DD_FREQ;
        //init(&base_freq);
        //d_freq = MIN(MAX_FREQ, d_freq + DD_FREQ);aaaa
        //} else {

        //init(&base_freq);
        //d_freq += 2*DD_FREQ;
        //d_freq = MAX(1000, d_freq - 1);
        //}
        buf_ind = (buf_ind + 1) % step;
    }

    // This is an implementation of simple copying
//    size_t n, m;
//    unsigned char buff[8192];
//
//    do {
//        n = fread(buff, 1, sizeof buff, ifp);
//        if (n)
//            m = fwrite(buff, 1, n, ofp);
//        else
//            m = 0;
//    } while ((n > 0) && (n == m));

    fclose(ifp);
    fclose(ofp);
}


int main(void)
{
//    FILE *in = fopen("f", "r");
//    FILE *out = fopen("out.bin", "w+");
//    FILE *out1 = fopen("out1.txt", "w");
    compress_ari("test_7", "tmp1.bin");
    decompress_ari("tmp1.bin", "out1.bin");
    return 0;
}
