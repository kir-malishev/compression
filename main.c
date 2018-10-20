#include <stdio.h>


long long *get_arr(FILE *f)
{
    long long *arr = calloc(257, sizeof(long long));
    int c;
    while ((c = fgetc(f)) != EOF) {
        arr[(unsigned char) c + 1]++;
    }
    fseek(f, 0, SEEK_SET);
    return arr;
}

long long get_b(long long *arr, int j)
{
    long long b = 0;
    for (int i = 0; i <= j; i++) {
        b += arr[i];
    }
    return b;
}

void bits_plus_follow(FILE *f, long long bits_to_follow, int bit)
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

void compress(FILE *f, FILE *out)
{
    fseek(f, 0, SEEK_END); // seek to end of file
    long long size = ftell(f); // get current file pointer
    fseek(f, 0, SEEK_SET);
    long long *arr = get_arr(f);
    fwrite(arr, sizeof(long long), 257, out);
    for (int k = 0; k < 256; k++) {
        //printf("%c %lld\n", k, arr[k + 1]);
    }
    long long l = 0;
    long long h = 65535 * 2;
    long long i = 0;
    long long delitel = size;
    long long First_qtr = (h + 1) / 4;
    long long Half = First_qtr * 2; // = 16384 = 32768
    long long Third_qtr = First_qtr * 3;
    long long bits_to_follow = 0; // = 49152, Сколько бит сбрасывать
    int c;
    while ((c = fgetc(f)) != EOF) {
        //printf("%lld\n", i);
        int j = (unsigned char) c + 1;
        i++; // Находим его индекс
        long long li = l + get_b(arr, j - 1) * (h - l + 1) / delitel;
        long long hi = l + get_b(arr, j) * (h - l + 1) / delitel - 1;
        for (; li <= hi;) { // Обрабатываем варианты
            if (hi < Half) { // переполнения
                bits_plus_follow(out, bits_to_follow, 0);
                bits_to_follow = 0;
            } else if (li >= Half) {
                bits_plus_follow(out, bits_to_follow, 1);
                bits_to_follow = 0;
                li -= Half;
                hi -= Half;
            } else if ((hi < First_qtr) && (li >= Third_qtr)) {
                bits_to_follow++;
                li -= First_qtr;
                hi -= First_qtr;
            } else break;
            li += li;
            hi += hi + 1;
        }
        l = li;
        h = hi;
    }
    bits_plus_follow(out, 0, -1);
    free(arr);
    fseek(out, 0, SEEK_SET);
}

int read_bit(FILE *f)
{
    static char st = 0;
    static int i = -1;
    if (i == -1) {
        fread(&st, sizeof(st), 1, f);
        i = 7;
        return st >> i;
    } else {
        i--;
        return (st >> (i + 1)) & 1;
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
    long long l = 0;
    long long h = 65535;
    long long i = 0;
    long long *arr = calloc(257, sizeof(long long));
    fread(arr, sizeof(long long), 257, f);
//    for (int k = 0; k < 256; k++) {
//        printf("%c %lld\n", k, arr[k + 1]);
//    }
    long long size = get_b(arr, 257);
    long long delitel = size;
    long long First_qtr = (h + 1) / 4;        // = 16384
    long long Half = First_qtr * 2;        // = 32768
    long long Third_qtr = First_qtr * 3;    // = 49152
    char val;
    fread(&val, sizeof(val), 1, f);
    long long value = val;
    while (!feof(f)) {
        long long freq = ((value - l + 1) * delitel - 1) / (h - l + 1);
        int j;
        for (j = 1; get_b(arr, j) <= freq; j++);
        int a = 5;
        if (j == 257) break;
        i++; // Находим его индекс
        long long li = l + get_b(arr, j - 1) * (h - l + 1) / delitel;
        long long hi = l + get_b(arr, j) * (h - l + 1) / delitel - 1;
        for (;;) {
            if (hi < Half);
            else if (li >= Half) {
                value -= Half;
                li -= Half;
                hi -= Half;
            } else if ((hi < First_qtr) && (li >= Third_qtr)) {
                value -= First_qtr;
                li -= First_qtr;
                hi -= First_qtr;
            } else break;
            li += li;
            hi += hi + 1;
            value += value + read_bit(f);//Добавляем бит из файла
        }
        l = li;
        h = hi;
        fputc(j - 1, out);
        fputc(j - 1, out);
    }
    free(arr);
}


int main(void)
{
    FILE *in = fopen("in.txt", "r");
    FILE *out = fopen("out.txt", "w+");
    FILE *out1 = fopen("out1.txt", "w");
    compress(in, out);
    decompress(out, out1);
    return 0;
}
