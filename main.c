
#include <stdio.h>
#include <stdint.h>




#define  TOP	   (1<<24)

typedef struct RangeCoder
{
    unsigned int code;
    unsigned int range;
    unsigned int FFNum;
    unsigned int Cache;
    int64_t low;
    FILE *f;
} RangeCoder;

void rc_clear(RangeCoder *AC) {
    AC->code = 0;
    AC->range = 0;
    AC->FFNum = 0;
    AC->Cache = 0;
    AC->low = 0;
    AC->f = 0;
}

void ShiftLow(RangeCoder *rc)
{
    if ( (rc->low>>24)!=0xFF ) {
        putc ((int) (rc->Cache + (rc->low>>32)), rc->f );
        int c = (int) (0xFF+(rc->low>>32));
        while(rc->FFNum ) putc(c, rc->f), (rc->FFNum)--;
        rc->Cache = (unsigned int) (rc->low)>>24;
    } else (rc->FFNum)++;
    rc->low = (unsigned int) (rc->low)<<8;
}

void StartEncode(RangeCoder *rc, FILE *out )
{
    rc->low=rc->FFNum=rc->Cache=0;
    rc->range=(unsigned int)-1;
    rc->f = out;
}

void StartDecode(RangeCoder *rc, FILE *in )
{
    rc->code=0;
    rc->range=(unsigned int)-1;
    rc->f = in;
    for (int i = 0; i < 5; i++) {
        rc->code = (rc->code << 8) | getc(rc->f);
    }
}

void FinishEncode(RangeCoder *rc)
{
    rc->low+=1;
    for (int i = 0; i < 5; i++) {
        ShiftLow(rc);
    }
}


void encode(RangeCoder *rc, int cumFreq, int freq, int totFreq)
{
    rc->low += cumFreq * (rc->range/= totFreq);
    rc->range*= freq;
    while(rc->range<TOP ) ShiftLow(rc), rc->range<<=8;
}


unsigned int get_freq (RangeCoder *rc, int totFreq) {
    return rc->code / (rc->range/= totFreq);
}

void decode_update (RangeCoder *rc, int cumFreq, int freq)
{
    rc->code -= cumFreq*rc->range;
    rc->range *= freq;
    while( rc->range<TOP ) rc->code=(rc->code<<8)|getc(rc->f), rc->range<<=8;
}




typedef  struct ContextModel{
    int	esc,
            TotFr;
    int	count[256];
} ContextModel;

void cm_clear(ContextModel *cm) {
    cm->esc = 0;
    cm->TotFr = 0;
    for (int i = 0; i < 256; i++) {
        cm->count[i] = 0;
    }
}


const int MAX_TotFr = 0x3fff;



void init_model (int context[2], int *SP, ContextModel cm1[257], ContextModel cm2[256][256], RangeCoder *AC){
    rc_clear(AC);
    for ( int j = 0; j < 256; j++ ) {
        for (int k = 0; k < 256; k++) {
            cm_clear(&cm2[j][k]);
        }
        cm_clear(&cm1[j]);
        cm1[256].count[j] = 1;
    }
    cm1[256].TotFr = 256;
    cm1[256].esc = 1;
    context[0] = 0;
    context[1] = 0;
    *SP = 0;
}



int encode_sym (int *SP, ContextModel *stack[], RangeCoder *AC, ContextModel *CM, int c){
    stack[(*SP)++] = CM;
    if (CM->count[c]){
        int CumFreqUnder = 0;
        for (int i = 0; i < c; i++)
            CumFreqUnder += CM->count[i];
        encode(AC, CumFreqUnder, CM->count[c], CM->TotFr + CM->esc);
        return 1;
    }else{
        if (CM->esc){
            encode(AC, CM->TotFr, CM->esc, CM->TotFr + CM->esc);
        }
        return 0;
    }
}

int decode_sym (int *SP, ContextModel *stack[], RangeCoder *AC, ContextModel *CM, int *c){
    stack[(*SP)++] = CM;
    if (!CM->esc) return 0;

    int cum_freq = get_freq(AC, CM->TotFr + CM->esc);
    if (cum_freq < CM->TotFr){
        int CumFreqUnder = 0;
        int i = 0;
        for (;;) {
            if ( (CumFreqUnder + CM->count[i]) <= cum_freq)
                CumFreqUnder += CM->count[i];
            else break;
            i++;
        }
        decode_update(AC, CumFreqUnder, CM->count[i]);
        *c = i;
        return 1;
    }else{
        decode_update(AC, CM->TotFr, CM->esc);
        return 0;
    }
}

void rescale (ContextModel *CM){
    CM->TotFr = 0;
    for (int i = 0; i < 256; i++){
        CM->count[i] -= CM->count[i] >> 1;
        CM->TotFr += CM->count[i];
    }
}

void update_model(int *SP, ContextModel *stack[], int c){
    while (*SP) {
        (*SP)--;
        if (stack[*SP]->TotFr >= MAX_TotFr)
            rescale (stack[*SP]);
        stack[*SP]->TotFr += 1;
        if (!stack[*SP]->count[c])
            stack[*SP]->esc += 1;
        stack[*SP]->count[c] += 1;
    }
}


ContextModel cm1[257], cm2[256][256];
void compress (FILE *ifp, FILE *ofp) {
    int	context[2];
    int SP;
    ContextModel *stack[3];
    RangeCoder AC;
    //int cnt = 0;
    int	c, success;
    init_model(context, &SP, cm1, cm2, &AC);
    StartEncode (&AC, ofp);
    while (( c = getc(ifp) ) != EOF) {
        //cnt++;
        success = encode_sym(&SP, stack, &AC, &cm2[context[0]][context[1]], c);
    if (!success) {
        success = encode_sym(&SP, stack, &AC, &cm1[context[0]], c);
        if (!success)
            encode_sym(&SP, stack, &AC, &cm1[256], c);
    }
        update_model(&SP, stack, c);
        context[1] = context[0];
        context [0] = c;
    }
    if (cm2[context[0]][context[1]].TotFr) {
        encode(&AC, cm2[context[0]][context[1]].TotFr, cm2[context[0]][context[1]].esc, cm2[context[0]][context[1]].TotFr + cm2[context[0]][context[1]].esc);
    }
    if (cm1[context[0]].TotFr) {
        encode(&AC, cm1[context[0]].TotFr, cm1[context[0]].esc,
               cm1[context[0]].TotFr + cm1[context[0]].esc);
    }
    encode (&AC, cm1[256].TotFr, cm1[256].esc,
            cm1[256].TotFr + cm1[256].esc);
    FinishEncode(&AC);
}

void decode (FILE *ifp, FILE *ofp){
    int	context[2];
    int SP;
    ContextModel *stack[3];
    RangeCoder AC;
    int	c, success;
    init_model(context, &SP, cm1, cm2, &AC);
    StartDecode (&AC, ifp);
    for (;;){

        success = decode_sym(&SP, stack, &AC,  &cm2[context[0]][context[1]], &c);
        if (!success) {
            success = decode_sym(&SP, stack, &AC, &cm1[context[0]], &c);
            if (!success){
                success = decode_sym(&SP, stack, &AC, &cm1[256], &c);
                if (!success) break;
            }
        }
        update_model(&SP, stack, c);
        context[1] = context[0];
        context [0] = c;
        putc(c, ofp);
    }
}


void compress_ppm(char *ifile, char *ofile) {
    FILE *ifp = fopen(ifile, "rb");
    FILE *ofp = fopen(ofile, "wb");



    compress (ifp, ofp);


    fclose(ifp);
    fclose(ofp);
}

void decompress_ppm(char *ifile, char *ofile) {
    FILE *ifp = fopen(ifile, "rb");
    FILE *ofp = fopen(ofile, "wb");

    decode (ifp, ofp);



    fclose(ifp);
    fclose(ofp);
}


int main (int argc, char* argv[]){
    compress_ppm("Война и мир.txt", "tmp.bin");
    decompress_ppm("tmp.bin", "out.bin");
}
