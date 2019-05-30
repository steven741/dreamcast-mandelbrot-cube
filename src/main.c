#include "math.h"
#include "dc_registers.h"
#include "dc_locations.h"
#include "dc_ta_instructions.h"



void *sq_cpy(void *dest, const void *src, int n)
{
    volatile uint32_t *d = (volatile uint32_t*)(0xe0000000 | (((uint32_t)dest) & 0x03ffffe0));
    volatile uint32_t *s = (volatile uint32_t*)src;

    /* Set store queue memory area as desired */
    QACR0 = ((((uint32_t)dest) >> 26) << 2) & 0x1c;
    QACR1 = ((((uint32_t)dest) >> 26) << 2) & 0x1c;

    /* fill/write queues as many times necessary */
    n >>= 5;

    while(n--) {
        asm volatile("pref @%0" : : "r"(s + 8));  /* prefetch 32 bytes for next loop */
        d[0] = *(s++);
        d[1] = *(s++);
        d[2] = *(s++);
        d[3] = *(s++);
        d[4] = *(s++);
        d[5] = *(s++);
        d[6] = *(s++);
        d[7] = *(s++);
        asm volatile("pref @%0" : : "r"(d));
        d += 8;
    }

    /* Wait for both store queues to complete */
    d = (volatile uint32_t*)0xe0000000;
    d[0] = d[8] = 0;

    return dest;
}




/*
 MATH 
 */

#define F_PI 3.1415926f

#define XCENTER 320.0
#define YCENTER 240.0

#define COT_FOVY_2 1.73 /* cot(FOVy / 2) */
#define ZNEAR 1.0
#define ZFAR  100.0

#define ZOFFS 5.0

float screenview_matrix[4][4] = {
  { YCENTER,     0.0,   0.0,   0.0 },
  {     0.0, YCENTER,   0.0,   0.0 },
  {     0.0,     0.0,   1.0 ,  0.0 },
  { XCENTER, YCENTER,   0.0,   1.0 },
};

float projection_matrix[4][4] = {
  { COT_FOVY_2,         0.0,                        0.0,   0.0 },
  {        0.0,  COT_FOVY_2,                        0.0,   0.0 },
  {        0.0,         0.0,  (ZFAR+ZNEAR)/(ZNEAR-ZFAR),  -1.0 },
  {        0.0,         0.0,  2*ZFAR*ZNEAR/(ZNEAR-ZFAR),   1.0 },
};

float translation_matrix[4][4] = {
  { 1.0,   0.0,    0.0,   0.0 },
  { 0.0,   1.0,    0.0,   0.0 },
  { 0.0,   0.0,    1.0,   0.0 },
  { 0.0,   0.0,  ZOFFS,   1.0 },
};


#define __fsin(x) \
    ({ float __value, __arg = (x), __scale = 10430.37835; \
        __asm__("fmul   %2,%1\n\t" \
                "ftrc   %1,fpul\n\t" \
                "fsca   fpul,dr0\n\t" \
                "fmov   fr0,%0" \
                : "=f" (__value), "+&f" (__scale) \
                : "f" (__arg) \
                : "fpul", "fr0", "fr1"); \
        __value; })

#define __fcos(x) \
    ({ float __value, __arg = (x), __scale = 10430.37835; \
        __asm__("fmul   %2,%1\n\t" \
                "ftrc   %1,fpul\n\t" \
                "fsca   fpul,dr0\n\t" \
                "fmov   fr1,%0" \
                : "=f" (__value), "+&f" (__scale) \
                : "f" (__arg) \
                : "fpul", "fr0", "fr1"); \
        __value; })

float fsin(float r) {
    return __fsin(r);
}

float fcos(float r) {
    return __fcos(r);
}

void rotate_x(int n)
{
    float matrix[4][4] = {
    { 1.0, 0.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0, 0.0 },
    { 0.0, 0.0, 1.0, 0.0 },
    { 0.0, 0.0, 0.0, 1.0 },
    };

    matrix[1][1] = matrix[2][2] = fcos((float)n * F_PI / 180.0f);
    matrix[1][2] = -(matrix[2][1] = fsin((float)n * F_PI / 180.0f));
    apply_matrix(&matrix);
}

void rotate_y(int n)
{
    float matrix[4][4] = {
    { 1.0, 0.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0, 0.0 },
    { 0.0, 0.0, 1.0, 0.0 },
    { 0.0, 0.0, 0.0, 1.0 },
    };

    matrix[0][0] = matrix[2][2] = fcos((float)n * F_PI / 180.0f);
    matrix[2][0] = -(matrix[0][2] = fsin((float)n * F_PI / 180.0f));
    apply_matrix(&matrix);
}

void rotate_z(int n)
{
    float matrix[4][4] = {
    { 1.0, 0.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0, 0.0 },
    { 0.0, 0.0, 1.0, 0.0 },
    { 0.0, 0.0, 0.0, 1.0 },
    };

    matrix[0][0] = matrix[1][1] =  fcos((float)n * F_PI / 180.0f);
    matrix[0][1] = -(matrix[1][0] = fsin((float)n * F_PI / 180.0f));
    apply_matrix(&matrix);
}






/*
 GRAPHICS
 */

#define SIZE_OF_OPB          ((16) * ( 640/32 ) * ( 480/32 ) * 4)
#define SIZE_OF_BACKGROUND   0x3C
#define SIZE_OF_REGION_ARRAY 0x1C20
#define SIZE_OF_FRAMEBUFFER  0x96000 /* 640x480 * 2 Bytes if 565 colors */


#define BPP                     16
#define WIDTH                   640
#define HEIGHT                  480

#define CB_VGA                  0
#define CB_NONE                 1
#define CB_RGB                  2
#define CB_COMPOSITE            3

void graphics_init()
{
    volatile unsigned int *porta = (unsigned int *)0xff80002c;

    *porta = (*(unsigned int *)0xff80002c & ~0xf0000) | 0xa0000;


    switch( ((*(volatile unsigned short *)(porta+1))>>8)&3 )
    {
        case CB_VGA:
        {
            SPG_LOAD       = 524 << 16 | 857 << 0;
            SPG_HBLANK     = 126 << 16 | 837 << 0;
            SPG_VBLANK     =  40 << 16 | 520 << 0;
            SPG_WIDTH      = 504 << 22 | 403 << 12 | 1 << 8 | 63 << 0;
            SPG_CONTROL    =   1 << 8;
            SPG_VBLANK_INT =  21 << 16;

            /* Framebuffer settings */
            FB_R_CTRL       = ( 1 << 23 | 1 << 2 | 1 << 0 );
            FB_R_SIZE       = (                                 1 << 20
                               |                   ( HEIGHT - 1 ) << 10
                               | ( ( WIDTH * (32 / BPP) )/4 - 1 ) << 0 );

            FB_W_CTRL       = ( 1 << 0 );
            FB_W_LINESTRIDE = (WIDTH * (32 / BPP))/8;
            FB_BURSTCTRL    = 0x00093f39;
            FB_X_CLIP       = 0x02800000;
            FB_Y_CLIP       = 0x01e00000;

            VO_STARTX     = 168 << 0;
            VO_STARTY     = 640 << 16 | 40 << 0;
            VO_CONTROL    =  22 << 16;
            VO_BORDER_COL = 0x00000000;
        } break;

        case CB_COMPOSITE:
        {
            SPG_LOAD    = 0x020C0359;
            SPG_HBLANK  = 0x007E0345;
            SPG_VBLANK  = 0x00240204;
            SPG_WIDTH   = 0x07D6C63F;
            SPG_CONTROL = 0x00000150;
            SPG_VBLANK_INT = 21 << 16 | 258 << 0;

            /* Framebuffer settings */
            FB_R_CTRL       = ( 1 << 0 | 1 << 2 );
            FB_R_SIZE       = (   ((WIDTH * (32 / BPP) >> 2) + 1) << 20
                               |                 ( HEIGHT/2 - 1 ) << 10
                               | ( ( WIDTH * (32 / BPP) )/4 - 1 ) << 0 );

            FB_W_CTRL       = ( 1 << 0 );
            FB_W_LINESTRIDE = (WIDTH * (32 / BPP))/8;
            FB_BURSTCTRL    = 0x00093f39;
            FB_X_CLIP       = 0x02800000;
            FB_Y_CLIP       = 0x01e00000;

            VO_STARTX     = 0x000000A4;
            VO_STARTY     = 0x00120012;
            VO_CONTROL    = 0x00160000;
            VO_BORDER_COL = 0x00000000;
        } break;

        default:
        {
            /* ERROR */
        } break;
    }


    SDRAM_CFG      = 0x15F28997;
    SDRAM_ARB_CFG;
    SDRAM_REFRESH  = 0x00000020;

    ISP_FEED_CFG   = 0x00800408 | ( 1 << 3 );
    SPAN_SORT_CFG  = 1 << 1 | 1 << 0;

    FPU_SHAD_SCALE = 0x00000000;
    FPU_PARAM_CFG  = 0x0027df77; /* FPU_PARAM_CFG - Sets data configuration for region area */
    FPU_CULL_VAL   = 0x3F800000;
    FPU_PERP_VAL   = 0x00000000;

    HALF_OFFSET    = 0x00000007; /* USED FOR TEXTURE FILTERING */
    TEXT_CONTROL   = 0x00000001;
    PT_ALPHA_REF   = 0x000000FF;
}


void ta_createRegionArray()
{
  int x, y;
  uint32_t *vr = (uint32_t*)( VRAM_BASE + VRAM_BANK2_BASE + SIZE_OF_OPB );


  for (y=0; y<(480/32); y++)
    for (x=0; x<(640/32); x++)
      {
	const int cur_tile = x + y * ( 640 / 32 );

	/* Note: end-of-list on the last tile! */
	if (x == (640/32)-1 && y == (480/32)-1)
	  *vr++ = 0x80000000 | (y << 8) | (x << 2);
	else
	  *vr++ = (y << 8) | (x << 2);


	*vr++ = VRAM_BANK2_BASE + ( ( (     0 + 16 * cur_tile ) * 4 )              );
	*vr++ = VRAM_BANK2_BASE + ( ( (  4256 +  8 * cur_tile ) * 4 ) | 0x80000000 );
	*vr++ = VRAM_BANK2_BASE + ( ( (  6384 + 16 * cur_tile ) * 4 ) | 0x80000000 );
	*vr++ = VRAM_BANK2_BASE + ( ( ( 10640 +  8 * cur_tile ) * 4 ) | 0x80000000 );
	*vr++ = VRAM_BANK2_BASE + ( ( ( 12768 + 16 * cur_tile ) * 4 ) | 0x80000000 );
      }
}




void ta_buildBackgroundPlane()
{
    uint32_t *vram = ( uint32_t* )(VRAM_BASE + VRAM_BANK2_BASE + SIZE_OF_OPB + SIZE_OF_REGION_ARRAY);

    ISP_BACKGND_T   = 0x01000000 | ( ( VRAM_BANK2_BASE + SIZE_OF_OPB + SIZE_OF_REGION_ARRAY ) << 1 );
    ISP_BACKGND_D   = 0x3F800000;

    *vram++ = 0x90800000; /* ISP/TSP Instruction Word */
    *vram++ = 0x20800440; /* TSP Instruction Word     */
    *vram++ = 0x00000000; /* Texture Control Word     */

    *vram++ = 0x00000000;
    *vram++ = 0x00000000;
    *vram++ = 0x3F800000;
    *vram++ = 0xFFFF0000;

    *vram++ = 0x00000000;
    *vram++ = 0x43F00000;
    *vram++ = 0x3F800000;
    *vram++ = 0xFF00FF00;

    *vram++ = 0x44200000;
    *vram++ = 0x00000000;
    *vram++ = 0x3F800000;
    *vram++ = 0xFF0000FF;

    FB_R_SOF1 = VRAM_BANK2_BASE + SIZE_OF_OPB + SIZE_OF_REGION_ARRAY + SIZE_OF_BACKGROUND;
    FB_W_SOF1 = VRAM_BANK2_BASE + SIZE_OF_OPB + SIZE_OF_REGION_ARRAY + SIZE_OF_BACKGROUND;
}


uint32_t ta_parameter[4] =
{
 GROUP_ENABLE_TA
 | POLYGON_VOLUME_TA
 | OUTSIDE_ENABLED_USER_CLIP_TA
 | STRIPS_6_TA
 | TEXTURE_TA,

 TA_ISP_TSP_DEPTH_COMPARE_MODE_ALWAYS
 | TA_ISP_TSP_CULL_IF_NEG,

 TA_TSP_SRC_ALPHA_INSTRUCTION_ONE
 | TA_TSP_DST_ALPHA_INSTRUCTION_ZERO
 | TA_TSP_FOG_NO_FOG
 | TA_TSP_FILTER_MODE_BILINEAR
 | TA_TSP_U_256
 | TA_TSP_V_256
};

uint32_t end_of_list[8] = { 0x00000000, 0x00000000, 
                            0x00000000, 0x00000000, 
                            0x00000000, 0x00000000, 
                            0x00000000, 0x00000000 };

struct
{
  uint32_t flag;
  float x, y, z;
  float u, v;
  uint32_t color;
  uint32_t offset_color;
} vert;

void draw_face(float *p1, float *p2, float *p3, float *p4, void *tex, int pal)
{
  ta_parameter[3] = 6 << 27
                  | pal << 25
                  | (((uint32_t)(tex)) - VRAM64_BASE) >> 3;
  sq_cpy(TA_Area, ta_parameter, 32);


  vert.flag = VERTEX_TA;
  vert.x = p1[0];
  vert.y = p1[1];
  vert.z = p1[2];
  vert.u = 0.0;
  vert.v = 0.0;
  sq_cpy(TA_Area, &vert, 32);

  vert.x = p2[0];
  vert.y = p2[1];
  vert.z = p2[2];
  vert.u = 1.0;
  vert.v = 0.0;
  sq_cpy(TA_Area, &vert, 32);

  vert.x = p3[0];
  vert.y = p3[1];
  vert.z = p3[2];
  vert.u = 0.0;
  vert.v = 1.0;
  sq_cpy(TA_Area, &vert, 32);

  vert.flag = END_OF_STRIP_TA;
  vert.x = p4[0];
  vert.y = p4[1];
  vert.z = p4[2];
  vert.u = 1.0;
  vert.v = 1.0;
  sq_cpy(TA_Area, &vert, 32);
}





/*
 Mandelbrot
 */

uint16_t *tex[2];

uint32_t compute_texture(int x, int y, int julia)
{
  float c_re = (x-128)*(1.0/16384)-1.313747;
  float c_im = (y-128)*(1.0/16384)-0.073227;
  float z_re = 0.0;
  float z_im = 0.0;
  int n=-1;

  if(julia) {
    z_re = c_re;
    z_im = c_im;
    c_re = -1.313747;
    c_im = -0.073227;
  }

  do {
    float tmp_r = z_re;
    z_re = z_re*z_re - z_im*z_im + c_re;
    z_im = 2*tmp_r*z_im + c_im;
  } while(++n<255 && z_re*z_re+z_im*z_im<=2.0);

  return n;
}

void build_texture()
{
    int twiddletab[1024];

    unsigned int red_pal[] = {
      0xff000000,0xff3c3c3c,0xff413c3c,0xff493c3c,0xff4d3838,0xff553838,0xff593434,0xff613434,
      0xff653030,0xff6d3030,0xff712c2c,0xff792c2c,0xff822828,0xff862828,0xff8e2424,0xff922424,
      0xff9a2020,0xff9e2020,0xffa61c1c,0xffaa1c1c,0xffb21818,0xffb61818,0xffbe1414,0xffc71414,
      0xffcb1010,0xffd31010,0xffd70c0c,0xffdf0c0c,0xffe30808,0xffeb0808,0xffef0404,0xfff70404,
      0xffff0000,0xffff0400,0xffff0c00,0xffff1400,0xffff1c00,0xffff2400,0xffff2c00,0xffff3400,
      0xffff3c00,0xffff4500,0xffff4d00,0xffff5500,0xffff5d00,0xffff6500,0xffff6d00,0xffff7500,
      0xffff7d00,0xffff8600,0xffff8e00,0xffff9600,0xffff9e00,0xffffa600,0xffffae00,0xffffb600,
      0xffffbe00,0xffffc700,0xffffcf00,0xffffd700,0xffffdf00,0xffffe700,0xffffef00,0xfffff700,
      0xffffff00,0xffffff04,0xffffff0c,0xffffff14,0xffffff1c,0xffffff24,0xffffff2c,0xffffff34,
      0xffffff3c,0xffffff45,0xffffff4d,0xffffff55,0xffffff5d,0xffffff65,0xffffff6d,0xffffff75,
      0xffffff7d,0xffffff86,0xffffff8e,0xffffff96,0xffffff9e,0xffffffa6,0xffffffae,0xffffffb6,
      0xffffffbe,0xffffffc7,0xffffffcf,0xffffffd7,0xffffffdf,0xffffffe7,0xffffffef,0xfffffff7,
      0xffffffff,0xffffffff,0xfffffbfb,0xfffffbf7,0xfffff7f3,0xfffff7ef,0xfffff3eb,0xfffff3e7,
      0xffffefe3,0xffffefdf,0xffffebdb,0xffffebd7,0xffffe7d3,0xffffe7cf,0xffffe3cb,0xffffe3c7,
      0xffffdfc3,0xffffdfbe,0xffffdbba,0xffffdbb6,0xffffd7b2,0xffffd7ae,0xffffd3aa,0xffffd3a6,
      0xffffcfa2,0xffffcf9e,0xffffcb9a,0xffffcb96,0xffffc792,0xffffc78e,0xffffc38a,0xffffc386,
      0xffffbe82,0xffffba7d,0xffffba79,0xffffb675,0xffffb671,0xffffb26d,0xffffb269,0xffffae65,
      0xffffae61,0xffffaa5d,0xffffaa59,0xffffa655,0xffffa651,0xffffa24d,0xffffa249,0xffff9e45,
      0xffff9e41,0xffff9a3c,0xffff9a38,0xffff9634,0xffff9630,0xffff922c,0xffff9228,0xffff8e24,
      0xffff8e20,0xffff8a1c,0xffff8a18,0xffff8614,0xffff8610,0xffff820c,0xffff8208,0xffff7d04,
      0xffff7900,0xffff7900,0xffff7500,0xffff7100,0xffff6d00,0xffff6900,0xffff6500,0xffff6100,
      0xffff5d00,0xffff5900,0xffff5500,0xffff5100,0xffff4d00,0xffff4900,0xffff4500,0xffff4100,
      0xffff3c00,0xffff3c00,0xffff3800,0xffff3400,0xffff3000,0xffff2c00,0xffff2800,0xffff2400,
      0xffff2000,0xffff1c00,0xffff1800,0xffff1400,0xffff1000,0xffff0c00,0xffff0800,0xffff0400,
      0xffff0000,0xffff0000,0xfffb0000,0xfff70000,0xfff70000,0xfff30000,0xffef0000,0xffeb0000,
      0xffeb0000,0xffe70000,0xffe30000,0xffe30000,0xffdf0000,0xffdb0000,0xffd70000,0xffd70000,
      0xffd30000,0xffcf0000,0xffcf0000,0xffcb0000,0xffc70000,0xffc30000,0xffc30000,0xffbe0000,
      0xffba0000,0xffba0000,0xffb60000,0xffb20000,0xffae0000,0xffae0000,0xffaa0000,0xffa60000,
      0xffa20000,0xffa20000,0xff9e0404,0xff9a0404,0xff960808,0xff920808,0xff8e0c0c,0xff8e0c0c,
      0xff8a1010,0xff861010,0xff821414,0xff7d1414,0xff791818,0xff791818,0xff751c1c,0xff711c1c,
      0xff6d2020,0xff692020,0xff652424,0xff652424,0xff612828,0xff5d2828,0xff592c2c,0xff552c2c,
      0xff513030,0xff513030,0xff4d3434,0xff493434,0xff453838,0xff413838,0xff3c3c3c,0xff3c3c3c,
    };

    unsigned int blue_pal[] = {
      0xff000000,0xff000000,0xff000004,0xff00000c,0xff000010,0xff000018,0xff000020,0xff000024,
      0xff00002c,0xff000030,0xff000038,0xff000041,0xff000045,0xff00004d,0xff000051,0xff000059,
      0xff000061,0xff000065,0xff00006d,0xff000075,0xff000079,0xff000082,0xff000086,0xff00008e,
      0xff000096,0xff00009a,0xff0000a2,0xff0000a6,0xff0000ae,0xff0000b6,0xff0000ba,0xff0000c3,
      0xff0000cb,0xff0004cb,0xff000ccb,0xff0010cf,0xff0018cf,0xff001cd3,0xff0024d3,0xff0028d3,
      0xff0030d7,0xff0038d7,0xff003cdb,0xff0045db,0xff0049db,0xff0051df,0xff0055df,0xff005de3,
      0xff0065e3,0xff0069e3,0xff0071e7,0xff0075e7,0xff007deb,0xff0082eb,0xff008aeb,0xff008eef,
      0xff0096ef,0xff009ef3,0xff00a2f3,0xff00aaf3,0xff00aef7,0xff00b6f7,0xff00bafb,0xff00c3fb,
      0xff00cbff,0xff04cbff,0xff0ccbff,0xff14cfff,0xff1ccfff,0xff24d3ff,0xff2cd3ff,0xff34d3ff,
      0xff3cd7ff,0xff45d7ff,0xff4ddbff,0xff55dbff,0xff5ddbff,0xff65dfff,0xff6ddfff,0xff75e3ff,
      0xff7de3ff,0xff86e3ff,0xff8ee7ff,0xff96e7ff,0xff9eebff,0xffa6ebff,0xffaeebff,0xffb6efff,
      0xffbeefff,0xffc7f3ff,0xffcff3ff,0xffd7f3ff,0xffdff7ff,0xffe7f7ff,0xffeffbff,0xfff7fbff,
      0xffffffff,0xfffbffff,0xfff7ffff,0xfff3ffff,0xffebffff,0xffe7ffff,0xffe3ffff,0xffdbffff,
      0xffd7ffff,0xffd3ffff,0xffcbffff,0xffc7ffff,0xffc3ffff,0xffbaffff,0xffb6ffff,0xffb2ffff,
      0xffaaffff,0xffa6ffff,0xffa2ffff,0xff9effff,0xff96ffff,0xff92ffff,0xff8effff,0xff86ffff,
      0xff82ffff,0xff7dffff,0xff75ffff,0xff71ffff,0xff6dffff,0xff65ffff,0xff61ffff,0xff5dffff,
      0xff55ffff,0xff51ffff,0xff4dffff,0xff49ffff,0xff41ffff,0xff3cffff,0xff38ffff,0xff30ffff,
      0xff2cffff,0xff28ffff,0xff20ffff,0xff1cffff,0xff18ffff,0xff10ffff,0xff0cffff,0xff08ffff,
      0xff00ffff,0xff00fbff,0xff00f7ff,0xff00f3ff,0xff00ebff,0xff00e7ff,0xff00e3ff,0xff00dbff,
      0xff00d7ff,0xff00d3ff,0xff00cbff,0xff00c7ff,0xff00c3ff,0xff00baff,0xff00b6ff,0xff00b2ff,
      0xff00aaff,0xff00a6ff,0xff00a2ff,0xff009eff,0xff0096ff,0xff0092ff,0xff008eff,0xff0086ff,
      0xff0082ff,0xff007dff,0xff0075ff,0xff0071ff,0xff006dff,0xff0065ff,0xff0061ff,0xff005dff,
      0xff0055ff,0xff0051ff,0xff004dff,0xff0049ff,0xff0041ff,0xff003cff,0xff0038ff,0xff0030ff,
      0xff002cff,0xff0028ff,0xff0020ff,0xff001cff,0xff0018ff,0xff0010ff,0xff000cff,0xff0008ff,
      0xff0000ff,0xff0000fb,0xff0000f7,0xff0000f3,0xff0000ef,0xff0000eb,0xff0000e7,0xff0000e3,
      0xff0000df,0xff0000db,0xff0000d7,0xff0000d3,0xff0000cf,0xff0000cb,0xff0000c7,0xff0000c3,
      0xff0000be,0xff0000ba,0xff0000b6,0xff0000b2,0xff0000ae,0xff0000aa,0xff0000a6,0xff0000a2,
      0xff00009e,0xff00009a,0xff000096,0xff000092,0xff00008e,0xff00008a,0xff000086,0xff000082,
      0xff00007d,0xff000079,0xff000075,0xff000071,0xff00006d,0xff000069,0xff000065,0xff000061,
      0xff00005d,0xff000059,0xff000055,0xff000051,0xff00004d,0xff000049,0xff000045,0xff000041,
      0xff00003c,0xff000038,0xff000034,0xff000030,0xff00002c,0xff000028,0xff000024,0xff000020,
      0xff00001c,0xff000018,0xff000014,0xff000010,0xff00000c,0xff000008,0xff000000,0xff000000,
    };

    unsigned int purplish_pal[] = {
      0xff000000,0xff9208e7,0xff9208e3,0xff9608e3,0xff9a04df,0xff9e04df,0xff9e04db,0xffa204db,
      0xffa600d7,0xffaa00d7,0xffaa00d3,0xffae00cf,0xffb200cf,0xffb600cb,0xffb600c7,0xffba00c7,
      0xffbe00c3,0xffbe00be,0xffc300be,0xffc700ba,0xffc700b6,0xffcb00b6,0xffcf00b2,0xffcf00ae,
      0xffd300aa,0xffd700aa,0xffd700a6,0xffdb04a2,0xffdb049e,0xffdf049e,0xffdf049a,0xffe30896,
      0xffe30892,0xffe70892,0xffe7088e,0xffeb0c8a,0xffeb0c86,0xffef0c82,0xffef1082,0xffef107d,
      0xfff31479,0xfff31475,0xfff31475,0xfff71871,0xfff7186d,0xfff71c69,0xfffb1c65,0xfffb2065,
      0xfffb2061,0xfffb245d,0xffff2859,0xffff2859,0xffff2c55,0xffff2c51,0xffff304d,0xffff344d,
      0xffff3449,0xffff3845,0xffff3c45,0xffff3c41,0xffff413c,0xffff453c,0xffff4538,0xffff4934,
      0xffff4d34,0xffff4d30,0xffff512c,0xffff552c,0xffff5928,0xffff5928,0xfffb5d24,0xfffb6120,
      0xfffb6520,0xfffb651c,0xfff7691c,0xfff76d18,0xfff77118,0xfff37514,0xfff37514,0xfff37914,
      0xffef7d10,0xffef8210,0xffef820c,0xffeb860c,0xffeb8a0c,0xffe78e08,0xffe79208,0xffe39208,
      0xffe39608,0xffdf9a04,0xffdf9e04,0xffdb9e04,0xffdba204,0xffd7a600,0xffd7aa00,0xffd3aa00,
      0xffcfae00,0xffcfb200,0xffcbb600,0xffc7b600,0xffc7ba00,0xffc3be00,0xffbebe00,0xffbec300,
      0xffbac700,0xffb6c700,0xffb6cb00,0xffb2cf00,0xffaecf00,0xffaad300,0xffaad700,0xffa6d700,
      0xffa2db04,0xff9edb04,0xff9edf04,0xff9adf04,0xff96e308,0xff92e308,0xff92e708,0xff8ee708,
      0xff8aeb0c,0xff86eb0c,0xff82ef0c,0xff82ef10,0xff7def10,0xff79f314,0xff75f314,0xff75f314,
      0xff71f718,0xff6df718,0xff69f71c,0xff65fb1c,0xff65fb20,0xff61fb20,0xff5dfb24,0xff59ff28,
      0xff59ff28,0xff55ff2c,0xff51ff2c,0xff4dff30,0xff4dff34,0xff49ff34,0xff45ff38,0xff45ff3c,
      0xff41ff3c,0xff3cff41,0xff3cff45,0xff38ff45,0xff34ff49,0xff34ff4d,0xff30ff4d,0xff2cff51,
      0xff2cff55,0xff28ff59,0xff28ff59,0xff24fb5d,0xff20fb61,0xff20fb65,0xff1cfb65,0xff1cf769,
      0xff18f76d,0xff18f771,0xff14f375,0xff14f375,0xff14f379,0xff10ef7d,0xff10ef82,0xff0cef82,
      0xff0ceb86,0xff0ceb8a,0xff08e78e,0xff08e792,0xff08e392,0xff08e396,0xff04df9a,0xff04df9e,
      0xff04db9e,0xff04dba2,0xff00d7a6,0xff00d7aa,0xff00d3aa,0xff00cfae,0xff00cfb2,0xff00cbb6,
      0xff00c7b6,0xff00c7ba,0xff00c3be,0xff00bebe,0xff00bec3,0xff00bac7,0xff00b6c7,0xff00b6cb,
      0xff00b2cf,0xff00aecf,0xff00aad3,0xff00aad7,0xff00a6d7,0xff04a2db,0xff049edb,0xff049edf,
      0xff049adf,0xff0896e3,0xff0892e3,0xff0892e7,0xff088ee7,0xff0c8aeb,0xff0c86eb,0xff0c82ef,
      0xff1082ef,0xff107def,0xff1479f3,0xff1475f3,0xff1475f3,0xff1871f7,0xff186df7,0xff1c69f7,
      0xff1c65fb,0xff2065fb,0xff2061fb,0xff245dfb,0xff2859ff,0xff2859ff,0xff2c55ff,0xff2c51ff,
      0xff304dff,0xff344dff,0xff3449ff,0xff3845ff,0xff3c45ff,0xff3c41ff,0xff413cff,0xff453cff,
      0xff4538ff,0xff4934ff,0xff4d34ff,0xff4d30ff,0xff512cff,0xff552cff,0xff5928ff,0xff5928ff,
      0xff5d24fb,0xff6120fb,0xff6520fb,0xff651cfb,0xff691cf7,0xff6d18f7,0xff7118f7,0xff7514f3,
      0xff7514f3,0xff7914f3,0xff7d10ef,0xff8210ef,0xff820cef,0xff860ceb,0xff8a0ceb,0xff8e08e7,
    };

    unsigned int (*palette)[4][256] = (unsigned int (*)[4][256])0xa05f9000;

    PAL_RAM_CTRL = 0x3; // looks like ARGB8888

    for(int n = 0; n<256; n++) {
        (*palette)[0][n] = red_pal[n];
        (*palette)[1][n] = blue_pal[n];
        (*palette)[2][n] = purplish_pal[n];
    }

    for(int x=0; x<1024; x++)
      twiddletab[x] = (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)|((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9);

    tex[0] = (uint16_t*)(VRAM64_BASE + 2*(SIZE_OF_OPB + SIZE_OF_BACKGROUND + SIZE_OF_REGION_ARRAY + SIZE_OF_FRAMEBUFFER));
    tex[1] = (uint16_t*)((VRAM64_BASE + 2*( SIZE_OF_OPB + SIZE_OF_BACKGROUND + SIZE_OF_REGION_ARRAY + SIZE_OF_FRAMEBUFFER))+256*256);

    for(int i=0; i<256; i++)
        for(int j=0; j<256; j+=2)
        {
            /* Texture 0 = Mandelbrot */
            tex[0][twiddletab[i]|(twiddletab[j]>>1)] = compute_texture(i, j, 0) | (compute_texture(i, j+1, 0)<<8);

            /* Texture 1 = Julia */
            tex[1][twiddletab[i]|(twiddletab[j]>>1)] = compute_texture(i, j, 1) | (compute_texture(i, j+1, 1)<<8);
        }
}





float coords[8][3] = {
  { -1.0, -1.0, -1.0 },
  {  1.0, -1.0, -1.0 },
  { -1.0,  1.0, -1.0 },
  {  1.0,  1.0, -1.0 },
  { -1.0, -1.0,  1.0 },
  {  1.0, -1.0,  1.0 },
  { -1.0,  1.0,  1.0 },
  {  1.0,  1.0,  1.0 },
};

float trans_coords[8][3];


int main ()
{
    build_texture();
    graphics_init();
    ta_createRegionArray();
    ta_buildBackgroundPlane();

    for(int i = 0; ; i++)
    {
        clear_matrix();
        apply_matrix(&screenview_matrix);
        apply_matrix(&projection_matrix);
        apply_matrix(&translation_matrix);
        rotate_x(i);
        rotate_y(i);
        rotate_z(i);
        transform_coords(coords, trans_coords, 8);


	PARAM_BASE  = 0x00000000;
	REGION_BASE = VRAM_BANK2_BASE + SIZE_OF_OPB;

	SOFTRESET = 1;
	SOFTRESET = 0;

	TA_GLOB_TILE_CLIP  = (480/32-1) << 16 | (640/32-1);
	TA_ALLOC_CTRL      = 0x00000002;

	TA_ISP_BASE        = 0x00000000;
	TA_ISP_LIMIT       = SIZE_OF_OPB + SIZE_OF_BACKGROUND + SIZE_OF_REGION_ARRAY + SIZE_OF_FRAMEBUFFER;

	TA_OL_BASE         = VRAM_BANK2_BASE;
	TA_OL_LIMIT        = VRAM_BANK2_BASE + SIZE_OF_OPB;

	TA_LIST_INIT       = 0x80000000;
	TA_LIST_INIT;

        draw_face(trans_coords[0], trans_coords[1], trans_coords[2], trans_coords[3], tex[0], 0);
        draw_face(trans_coords[1], trans_coords[5], trans_coords[3], trans_coords[7], tex[0], 1);
        draw_face(trans_coords[4], trans_coords[5], trans_coords[0], trans_coords[1], tex[0], 2);
        draw_face(trans_coords[5], trans_coords[4], trans_coords[7], trans_coords[6], tex[1], 0);
        draw_face(trans_coords[4], trans_coords[0], trans_coords[6], trans_coords[2], tex[1], 1);
        draw_face(trans_coords[2], trans_coords[3], trans_coords[6], trans_coords[7], tex[1], 2);
        sq_cpy( TA_Area, end_of_list, 32 );

	SB_ISTNRM = 0x08;
	while(!(SB_ISTNRM & 0x08)) {};

        STARTRENDER = 0xFFFFFFFF;
  }
}
