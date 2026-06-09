#include "platform.h"
#include "timer_f.h"
#include "xtime_l.h"

#define PB_FRQ 100000000
#define INVERT 0

static XTmrCtr tR, tG, tB;
static unsigned long g_period = 0;

static inline void pwm_set_pct(XTmrCtr* t, unsigned char pct){
    if(pct>100)pct=100;
    unsigned long high=(g_period*(unsigned long)pct)/100u;
#if INVERT
    high=g_period-high;
#endif
    PwmConfig(t,g_period,high);
}

void rgbled_init(unsigned int freq,unsigned char width){
    if(freq==0)freq=1;
    g_period=(unsigned long)(PB_FRQ/freq);
    PwmInit(&tR,0);
    PwmInit(&tG,1);
    PwmInit(&tB,2);
    pwm_set_pct(&tR,width);
    pwm_set_pct(&tG,width);
    pwm_set_pct(&tB,width);
}

void rgbled_setpwmwidths(unsigned char r,unsigned char g,unsigned char b){
    pwm_set_pct(&tR,r);
    pwm_set_pct(&tG,g);
    pwm_set_pct(&tB,b);
}

enum state{BLUE_001,GREEN_010,CYAN_011,RED_100,MAGENTA_101,YELLOW_110,WHITE_111};

int main(void){
    init_platform();

    rgbled_init(200,0);

    const unsigned step_ms=10;
    const int steps=100;
    XTime tick_inc=(XTime)((COUNTS_PER_SECOND/1000)*step_ms);

    enum state s=BLUE_001;
    int dir=+1;
    int k=0;

    XTime now,next_tick;
    XTime_GetTime(&now);
    next_tick=now+tick_inc;

    while(1){
        do{ XTime_GetTime(&now); }while(now<next_tick);
        next_tick=next_tick+tick_inc;

        unsigned char brightness;
        if(dir>0) brightness=(unsigned char)(1+(k*98)/100);
        else      brightness=(unsigned char)(99-(k*98)/100);

        unsigned char r=0,g=0,b=0;
        switch(s){
            case BLUE_001:
            	r=0; g=0; b=brightness;
            	break;
            case GREEN_010:
            	r=0; g=brightness; b=0;
            	break;
            case CYAN_011:
            	r=0; g=brightness; b=brightness;
            	break;
            case RED_100:
            	r=brightness; g=0; b=0;
            	break;
            case MAGENTA_101:
            	r=brightness; g=0; b=brightness;
            	break;
            case YELLOW_110:
            	r=brightness; g=brightness; b=0;
            	break;
            case WHITE_111:
            	r=brightness; g=brightness; b=brightness;
            	break;
        }
        rgbled_setpwmwidths(r,g,b);

        k=k+1;
        if(k>=steps){
            k=0;
            dir=-dir;
            if(dir>0){
                if(s==WHITE_111) s=BLUE_001;
                else s=(enum state)(s+1);
            }
        }
    }

    cleanup_platform();
    return 0;
}
