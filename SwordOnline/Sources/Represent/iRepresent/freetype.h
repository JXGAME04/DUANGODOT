//File redeclare freetype structs

#ifndef FREETYPE_H_
#define FREETYPE_H_
typedef struct FT_MemoryRec_*  FT_Memory;

typedef void*
  (*FT_Alloc_Func)( FT_Memory  memory,
                    long       size );
					
typedef void
  (*FT_Free_Func)( FT_Memory  memory,
                   void*      block );
				   
typedef void*
  (*FT_Realloc_Func)( FT_Memory  memory,
                      long       cur_size,
                      long       new_size,
                      void*      block );

struct  FT_MemoryRec_
  {
    void*            user;
    FT_Alloc_Func    alloc;
    FT_Free_Func     free;
    FT_Realloc_Func  realloc;
  };

typedef int  FT_Error;
typedef signed int  FT_Int;
typedef unsigned int  FT_UInt;
typedef unsigned long  FT_ULong;
typedef signed long  FT_Long;
typedef char  FT_String;
typedef signed long  FT_Fixed;
typedef signed long  FT_Pos;
typedef signed short  FT_Short;
typedef unsigned short  FT_UShort;
typedef void*  FT_Pointer;

typedef struct FT_ModuleRec_*  FT_Module;

typedef FT_Error
  (*FT_Module_Constructor)( FT_Module  module );
typedef void
  (*FT_Module_Destructor)( FT_Module  module );


typedef FT_Pointer  FT_Module_Interface;
typedef FT_Module_Interface
  (*FT_Module_Requester)( FT_Module    module,
                          const char*  name );

typedef struct  FT_Module_Class_
  {
    FT_ULong               module_flags;
    FT_Long                module_size;
    const FT_String*       module_name;
    FT_Fixed               module_version;
    FT_Fixed               module_requires;

    const void*            module_interface;

    FT_Module_Constructor  module_init;
    FT_Module_Destructor   module_done;
    FT_Module_Requester    get_interface;

  } FT_Module_Class;
  
typedef struct FT_LibraryRec_  *FT_Library;

typedef struct  FT_ModuleRec_
  {
    FT_Module_Class*  clazz;
    FT_Library        library;
    FT_Memory         memory;

  } FT_ModuleRec;
  
typedef struct FT_ListNodeRec_*  FT_ListNode;
typedef struct  FT_ListNodeRec_
  {
    FT_ListNode  prev;
    FT_ListNode  next;
    void*        data;

  } FT_ListNodeRec;
typedef struct  FT_ListRec_
  {
    FT_ListNode  head;
    FT_ListNode  tail;

  } FT_ListRec;

typedef struct FT_RendererRec_*  FT_Renderer;

typedef struct  FT_Bitmap_Size_
  {
    FT_Short  height;
    FT_Short  width;

    FT_Pos    size;

    FT_Pos    x_ppem;
    FT_Pos    y_ppem;

  } FT_Bitmap_Size;
 typedef enum  FT_Encoding_
  {
    FT_ENC_TAG( FT_ENCODING_NONE, 0, 0, 0, 0 ),

    FT_ENC_TAG( FT_ENCODING_MS_SYMBOL, 's', 'y', 'm', 'b' ),
    FT_ENC_TAG( FT_ENCODING_UNICODE,   'u', 'n', 'i', 'c' ),

    FT_ENC_TAG( FT_ENCODING_SJIS,    's', 'j', 'i', 's' ),
    FT_ENC_TAG( FT_ENCODING_PRC,     'g', 'b', ' ', ' ' ),
    FT_ENC_TAG( FT_ENCODING_BIG5,    'b', 'i', 'g', '5' ),
    FT_ENC_TAG( FT_ENCODING_WANSUNG, 'w', 'a', 'n', 's' ),
    FT_ENC_TAG( FT_ENCODING_JOHAB,   'j', 'o', 'h', 'a' ),

    /* for backward compatibility */
    FT_ENCODING_GB2312     = FT_ENCODING_PRC,
    FT_ENCODING_MS_SJIS    = FT_ENCODING_SJIS,
    FT_ENCODING_MS_GB2312  = FT_ENCODING_PRC,
    FT_ENCODING_MS_BIG5    = FT_ENCODING_BIG5,
    FT_ENCODING_MS_WANSUNG = FT_ENCODING_WANSUNG,
    FT_ENCODING_MS_JOHAB   = FT_ENCODING_JOHAB,

    FT_ENC_TAG( FT_ENCODING_ADOBE_STANDARD, 'A', 'D', 'O', 'B' ),
    FT_ENC_TAG( FT_ENCODING_ADOBE_EXPERT,   'A', 'D', 'B', 'E' ),
    FT_ENC_TAG( FT_ENCODING_ADOBE_CUSTOM,   'A', 'D', 'B', 'C' ),
    FT_ENC_TAG( FT_ENCODING_ADOBE_LATIN_1,  'l', 'a', 't', '1' ),

    FT_ENC_TAG( FT_ENCODING_OLD_LATIN_2, 'l', 'a', 't', '2' ),

    FT_ENC_TAG( FT_ENCODING_APPLE_ROMAN, 'a', 'r', 'm', 'n' )

  } FT_Encoding;
typedef struct FT_CharMapRec_*  FT_CharMap;
typedef struct FT_FaceRec_*  FT_Face;
typedef struct  FT_CharMapRec_
  {
    FT_Face      face;
    FT_Encoding  encoding;
    FT_UShort    platform_id;
    FT_UShort    encoding_id;

  } FT_CharMapRec;
typedef struct  FT_FaceRec_
  {
    FT_Long           num_faces;
    FT_Long           face_index;

    FT_Long           face_flags;
    FT_Long           style_flags;

    FT_Long           num_glyphs;

    FT_String*        family_name;
    FT_String*        style_name;

    FT_Int            num_fixed_sizes;
    FT_Bitmap_Size*   available_sizes;

    FT_Int            num_charmaps;
    FT_CharMap*       charmaps;

    FT_Generic        generic;

    /* The following member variables (down to `underline_thickness`) */
    /* are only relevant to scalable outlines; cf. @FT_Bitmap_Size    */
    /* for bitmap fonts.                                              */
    FT_BBox           bbox;

    FT_UShort         units_per_EM;
    FT_Short          ascender;
    FT_Short          descender;
    FT_Short          height;

    FT_Short          max_advance_width;
    FT_Short          max_advance_height;

    FT_Short          underline_position;
    FT_Short          underline_thickness;

    FT_GlyphSlot      glyph;
    FT_Size           size;
    FT_CharMap        charmap;

    /* private fields, internal to FreeType */

    FT_Driver         driver;
    FT_Memory         memory;
    FT_Stream         stream;

    FT_ListRec        sizes_list;

    FT_Generic        autohint;   /* face-specific auto-hinter data */
    void*             extensions; /* unused                         */

    FT_Face_Internal  internal;

  } FT_FaceRec;
 typedef struct  FT_Generic_
  {
    void*                 data;
    FT_Generic_Finalizer  finalizer;

  } FT_Generic;
 typedef struct  FT_Glyph_Metrics_
  {
    FT_Pos  width;
    FT_Pos  height;

    FT_Pos  horiBearingX;
    FT_Pos  horiBearingY;
    FT_Pos  horiAdvance;

    FT_Pos  vertBearingX;
    FT_Pos  vertBearingY;
    FT_Pos  vertAdvance;

  } FT_Glyph_Metrics;
typedef struct  FT_Vector_
  {
    FT_Pos  x;
    FT_Pos  y;

  } FT_Vector;
typedef enum  FT_Glyph_Format_
  {
    FT_IMAGE_TAG( FT_GLYPH_FORMAT_NONE, 0, 0, 0, 0 ),

    FT_IMAGE_TAG( FT_GLYPH_FORMAT_COMPOSITE, 'c', 'o', 'm', 'p' ),
    FT_IMAGE_TAG( FT_GLYPH_FORMAT_BITMAP,    'b', 'i', 't', 's' ),
    FT_IMAGE_TAG( FT_GLYPH_FORMAT_OUTLINE,   'o', 'u', 't', 'l' ),
    FT_IMAGE_TAG( FT_GLYPH_FORMAT_PLOTTER,   'p', 'l', 'o', 't' ),
    FT_IMAGE_TAG( FT_GLYPH_FORMAT_SVG,       'S', 'V', 'G', ' ' )

  } FT_Glyph_Format;
typedef struct  FT_Bitmap_
  {
    unsigned int    rows;
    unsigned int    width;
    int             pitch;
    unsigned char*  buffer;
    unsigned short  num_grays;
    unsigned char   pixel_mode;
    unsigned char   palette_mode;
    void*           palette;

  } FT_Bitmap;
typedef struct  FT_Outline_
  {
    unsigned short   n_contours;  /* number of contours in glyph        */
    unsigned short   n_points;    /* number of points in the glyph      */

    FT_Vector*       points;      /* the outline's points               */
    unsigned char*   tags;        /* the points flags                   */
    unsigned short*  contours;    /* the contour end points             */

    int              flags;       /* outline masks                      */

  } FT_Outline;
typedef struct FT_SubGlyphRec_*  FT_SubGlyph;
typedef struct  FT_SubGlyphRec_
  {
    FT_Int     index;
    FT_UShort  flags;
    FT_Int     arg1;
    FT_Int     arg2;
    FT_Matrix  transform;

  } FT_SubGlyphRec;
typedef struct FT_Slot_InternalRec_*  FT_Slot_Internal;
typedef struct  FT_Slot_InternalRec_
  {
    FT_GlyphLoader  loader;
    FT_UInt         flags;
    FT_Bool         glyph_transformed;
    FT_Matrix       glyph_matrix;
    FT_Vector       glyph_delta;
    void*           glyph_hints;

    FT_Int32        load_flags;

  } FT_GlyphSlot_InternalRec;
typedef struct FT_GlyphSlotRec_*  FT_GlyphSlot;
typedef struct  FT_GlyphSlotRec_
  {
    FT_Library        library;
    FT_Face           face;
    FT_GlyphSlot      next;
    FT_UInt           glyph_index; /* new in 2.10; was reserved previously */
    FT_Generic        generic;

    FT_Glyph_Metrics  metrics;
    FT_Fixed          linearHoriAdvance;
    FT_Fixed          linearVertAdvance;
    FT_Vector         advance;

    FT_Glyph_Format   format;

    FT_Bitmap         bitmap;
    FT_Int            bitmap_left;
    FT_Int            bitmap_top;

    FT_Outline        outline;

    FT_UInt           num_subglyphs;
    FT_SubGlyph       subglyphs;

    void*             control_data;
    long              control_len;

    FT_Pos            lsb_delta;
    FT_Pos            rsb_delta;

    void*             other;

    FT_Slot_Internal  internal;

  } FT_GlyphSlotRec;
  
typedef FT_Error
  (*FT_Renderer_RenderFunc)( FT_Renderer       renderer,
                             FT_GlyphSlot      slot,
                             FT_Render_Mode    mode,
                             const FT_Vector*  origin );
typedef struct  FT_Renderer_Class_
  {
    FT_Module_Class            root;

    FT_Glyph_Format            glyph_format;

    FT_Renderer_RenderFunc     render_glyph;
    FT_Renderer_TransformFunc  transform_glyph;
    FT_Renderer_GetCBoxFunc    get_glyph_cbox;
    FT_Renderer_SetModeFunc    set_mode;

    const FT_Raster_Funcs*     raster_class;

  } FT_Renderer_Class;
typedef struct  FT_RendererRec_
  {
    FT_ModuleRec            root;
    FT_Renderer_Class*      clazz;
    FT_Glyph_Format         glyph_format;
    FT_Glyph_Class          glyph_class;

    FT_Raster               raster;
    FT_Raster_Render_Func   raster_render;
    FT_Renderer_RenderFunc  render;

  } FT_RendererRec;

typedef struct  FT_LibraryRec_
  {
    FT_Memory          memory;           /* library's memory manager */

    FT_Int             version_major;
    FT_Int             version_minor;
    FT_Int             version_patch;

    FT_UInt            num_modules;
    FT_Module          modules[FT_MAX_MODULES];  /* module objects  */

    FT_ListRec         renderers;        /* list of renderers        */
    FT_Renderer        cur_renderer;     /* current outline renderer */
    FT_Module          auto_hinter;

    FT_DebugHook_Func  debug_hooks[4];

#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
    FT_LcdFiveTapFilter      lcd_weights;      /* filter weights, if any */
    FT_Bitmap_LcdFilterFunc  lcd_filter_func;  /* filtering callback     */
#else
    FT_Vector                lcd_geometry[3];  /* RGB subpixel positions */
#endif

    FT_Int             refcount;

  } FT_LibraryRec;
  


#endif /* FREETYPE_H_ */


/* END */
