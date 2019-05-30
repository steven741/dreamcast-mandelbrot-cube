#ifndef DC_TA_INSTRUCTIONS_H_INCLUDED
#define DC_TA_INSTRUCTIONS_H_INCLUDED
/**
*
*   TA Parameters
*
*   Flags used by the TA to determine how data that is sent is processed.
*
*
*   There are 3 types of parameters \Control, \Global and \Vertex.
*
*   \Control Parameters are used for special data processing and ending lists
*
*   \Global Parameters are used for setting built in ways of handling data i.e. opaque, translucent, etc.
*
*   \Vertex Parameters are used for specifying the type of vertices sent.
*
* * * */
/**
*   \Parameter Control
* * */
/* Control Parameter */
#define END_OF_LIST_TA     0x00000000
#define USER_TILE_CLIP_TA  0x20000000
#define OBJECT_LIST_SET_TA 0x40000000

#define MODIFIER_VOLUME_TA 0x80000000
#define POLYGON_VOLUME_TA  0x80000000
#define SPRITE_TA          0xA0000000
#define VERTEX_TA          0xE0000000

/** Valid only in the Vertex Parameters. A parameter in which this bit is "1" ends a strip.
    The Spite and Modifier Volume Vertex Parameter must be set to "1". */
#define END_OF_STRIP_TA 0xE0000000 | 0x10000000

/** Specifies the Object List type. */
#define OPAQUE_LIST_TA                     0x00000000
#define OPAQUE_MODIFIER_VOLUME_TA          0x01000000
#define TRANSLUCENT_LIST_TA                0x02000000
#define TRANSLUCENT_MODIFIER_VOLUME_TA     0x03000000
#define PUNCH_THROUGH_LIST_TA              0x04000000


/**
*   \Global Control
*
*   The following only work when GROUP_ENABLE_TA is set
* * */
/** Set "1" in order to update the Strip_Len and User_Clip settings. */
#define GROUP_ENABLE_TA 0x00800000

/** Specifies the length of the strip that is to be partitioned. */
#define STRIPS_1_TA 0x00000000
#define STRIPS_2_TA 0x00040000
#define STRIPS_4_TA 0x00080000
#define STRIPS_6_TA 0x000C0000

/** Specifies how the User Tile Clipping area is to be used. */
#define DISABLE_USER_CLIP_TA         0x00000000
#define INSIDE_ENABLED_USER_CLIP_TA  0x00020000
#define OUTSIDE_ENABLED_USER_CLIP_TA 0x00030000


/**
*   \Object Control
*
* * */
#define SHADOW_TA                 0x00000080
#define VOLUME_TA                 0x00000040

/* Internally Everything is a Packed Color */
#define COLOR_TYPE_PACKED_TA      0x00000000
#define COLOR_TYPE_FLOATING_TA    0x00000010
#define COLOR_TYPE_INTENSITY_1_TA 0x00000020
#define COLOR_TYPE_INTENSITY_2_TA 0x00000030

#define TEXTURE_TA                 0x00000008
#define OFFSET_TA                  0x00000004
#define GOURAUD_TA                 0x00000002
#define UV_16BIT_TA                0x00000001






/**
*
*   ISP/TSP Instruction Word
*
*   Flags used by the TA to determine how data that is sent is proccessed.
*
*
*
* * * */
/**
    Following used for lists only
*/
/* Depth Modes */
#define TA_ISP_TSP_DEPTH_COMPARE_MODE_NEVER            ( 0 << 29 )
#define TA_ISP_TSP_DEPTH_COMPARE_MODE_LESS             ( 1 << 29 )
#define TA_ISP_TSP_DEPTH_COMPARE_MODE_EQUAL            ( 2 << 29 )
#define TA_ISP_TSP_DEPTH_COMPARE_MODE_LESS_OS_EQUAL    ( 3 << 29 )
#define TA_ISP_TSP_DEPTH_COMPARE_MODE_GREATER          ( 4 << 29 )
#define TA_ISP_TSP_DEPTH_COMPARE_MODE_NOT_EQUAL        ( 5 << 29 )
#define TA_ISP_TSP_DEPTH_COMPARE_MODE_GREATER_OR_EQUAL 0xC0000000
#define TA_ISP_TSP_DEPTH_COMPARE_MODE_ALWAYS           ( 7 << 29 )

/**
    Following used for BOTH
*/
/* Culling Modes */
#define TA_ISP_TSP_NO_CULLING                          ( 0 << 27 )
#define TA_ISP_TSP_CULL_IF_SMALL                       ( 1 << 27 )
#define TA_ISP_TSP_CULL_IF_NEG                         ( 2 << 27 ) //CCW
#define TA_ISP_TSP_CULL_IF_POS                         ( 3 << 27 ) //CW

/**
    Following used for modifiers only
*/
/* Volume Instruction */
#define TA_ISP_TSP_NORMAL_POLY                         ( 0 << 29 )
#define TA_ISP_TSP_INSIDE_LAST_POLY                    ( 1 << 29 )
#define TA_ISP_TSP_DEPTH_OUTSIDE_LAST_POLY             ( 2 << 29 )


/** Other Settings */
#define TA_ISP_TSP_Z_WRITE_DISABLE                     ( 1 << 26 )
#define TA_ISP_TSP_TEXTURE                             ( 1 << 25 ) /*redundant*/
#define TA_ISP_TSP_OFFEST                              ( 1 << 24 ) /*redundant*/
#define TA_ISP_TSP_GOURAUD                             ( 1 << 23 ) /*redundant*/
#define TA_ISP_TSP_16BIT_UV                            ( 1 << 22 ) /*redundant*/
#define TA_ISP_TSP_CACHE_BYPASS                        ( 1 << 21 )
#define TA_ISP_TSP_DCALC_CTRL                          ( 1 << 20 )







/**
*
*   TSP Instruction Word
*
*
*
*
* * * */
#define TA_TSP_SRC_ALPHA_INSTRUCTION_ZERO                         ( 0 << 29 )
#define TA_TSP_SRC_ALPHA_INSTRUCTION_ONE                          ( 1 << 29 )
#define TA_TSP_SRC_ALPHA_INSTRUCTION_OTHER_COLOR                  ( 2 << 29 )
#define TA_TSP_SRC_ALPHA_INSTRUCTION_INVERSE_OTHER_COLOR          ( 3 << 29 )
#define TA_TSP_SRC_ALPHA_INSTRUCTION_SRC_ALPHA                    ( 4 << 29 )
#define TA_TSP_SRC_ALPHA_INSTRUCTION_INVERSE_SRC_ALPHA            ( 5 << 29 )
#define TA_TSP_SRC_ALPHA_INSTRUCTION_DST_ALPHA                    ( 6 << 29 )
#define TA_TSP_SRC_ALPHA_INSTRUCTION_INVERSE_DST_ALPHA            ( 7 << 29 )

#define TA_TSP_DST_ALPHA_INSTRUCTION_ZERO                         ( 0 << 26 )
#define TA_TSP_DST_ALPHA_INSTRUCTION_ONE                          ( 1 << 26 )
#define TA_TSP_DST_ALPHA_INSTRUCTION_OTHER_COLOR                  ( 2 << 26 )
#define TA_TSP_DST_ALPHA_INSTRUCTION_INVERSE_OTHER_COLOR          ( 3 << 26 )
#define TA_TSP_DST_ALPHA_INSTRUCTION_SRC_ALPHA                    ( 4 << 26 )
#define TA_TSP_DST_ALPHA_INSTRUCTION_INVERSE_SRC_ALPHA            ( 5 << 26 )
#define TA_TSP_DST_ALPHA_INSTRUCTION_DST_ALPHA                    ( 6 << 26 )
#define TA_TSP_DST_ALPHA_INSTRUCTION_INVERSE_DST_ALPHA            ( 7 << 26 )

#define TA_TSP_SRC_ALPHA_SELECT                                   ( 1 << 25 )
#define TA_TSP_DST_ALPHA_SELECT                                   ( 1 << 24 )


#define TA_TSP_FOG_LOOK_UP_TABLE                                  ( 0 << 22 )
#define TA_TSP_FOG_PER_VERTEX                                     ( 1 << 22 )
#define TA_TSP_FOG_NO_FOG                                         ( 2 << 22 )
#define TA_TSP_FOG_LOOK_UP_TABLE_2                                ( 3 << 22 )


#define TA_TSP_COLOR_CLAMP                                        ( 1 << 21 )
#define TA_TSP_ALPHA                                              ( 1 << 20 )
#define TA_TSP_IGNORE_ALPHA                                       ( 1 << 19 )


#define TA_TSP_FLIP_UV                          ( 0 << 17 )
#define TA_TSP_CLAMP_UV                         ( 0 << 15 )


#define TA_TSP_FILTER_MODE_POINT_SAMPLE                           ( 0 << 13 )
#define TA_TSP_FILTER_MODE_BILINEAR                               ( 1 << 13 )
#define TA_TSP_FILTER_MODE_TRILINEAR_TYPE_A                       ( 2 << 13 )
#define TA_TSP_FILTER_MODE_TRILINEAR_TYPE_B                       ( 3 << 13 )


#define TA_TSP_SUPER_SAMPLE                                       ( 1 << 12 )


#define TA_TSP_MIP_MAP_D_ADJUST_FULL                              ( 4 << 8 )


#define TA_TSP_SHADING_DECAL                                      ( 0 << 6 )
#define TA_TSP_SHADING_MODULATE                                   ( 1 << 6 )
#define TA_TSP_SHADING_DECAL_ALPHA                                ( 2 << 6 )
#define TA_TSP_SHADING_MODULATE_ALPHA                             ( 3 << 6 )


#define TA_TSP_U_256                           ( 5 << 3 )
#define TA_TSP_V_256                           ( 5 << 0 )





/**
*
*   Texture Instruction Word
*
*
*
* * * */

#endif /* DC_TA_INSTRUCTIONS_H_INCLUDED */
