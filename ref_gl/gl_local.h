/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifdef _WIN32
#  include <windows.h>
#endif

#include <stdio.h>

#include <GL/gl.h>
#ifdef _WIN32
#include "glext.h" /* Knightmare- include new extension header */
#endif
#include <math.h>

#ifndef __linux__
#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT GL_COLOR_INDEX
#endif
#endif

#include "../client/ref.h"

#include "qgl.h"

#define	REF_VERSION	"GL 0.02" /* Knightmare changed, was 0.01 */

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


#ifndef __VIDDEF_T
#define __VIDDEF_T
typedef struct
{
	int	width, height;			// coordinates from main game
} viddef_t;
#endif

extern	viddef_t	vid;


/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

typedef enum 
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky
} imagetype_t;

typedef struct image_s
{
	char	name[MAX_QPATH];			// game path, including extension
	long		hash;					/* Knightmare- added to speed up searching */
	imagetype_t	type;
	int		width, height;				// source image
	int		upload_width, upload_height;	// after power of two and picmip
	int		registration_sequence;		// 0 = free
	struct msurface_s	*texturechain;	// for sort-by-texture world drawing
	int		texnum;						// gl texture binding
	float	sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	qboolean	scrap;
	qboolean	has_alpha;

	qboolean paletted;
} image_t;

#define	MAX_LIGHTMAPS	128		/* Knightmare- moved this here for use by macros */

#define	TEXNUM_LIGHTMAPS	1024
#define	TEXNUM_SCRAPS		TEXNUM_LIGHTMAPS + MAX_LIGHTMAPS	/* Knightmare- changed to use macro, was 1152 */
#define	TEXNUM_IMAGES		TEXNUM_SCRAPS + 1	/* Knightmare- changed to use macro, was 1153 */

#define		MAX_GLTEXTURES	2048	/* Knightmare increased, was 1024 */

//===================================================================

typedef enum
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

#include "gl_model.h"

void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);

void GL_SetDefaultState( void );
void GL_UpdateSwapInterval( void );
void GL_PrintError (int errorCode, char *funcName);	/* Knightmare added */

extern	float	gldepthmin, gldepthmax;

typedef struct
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} glvert_t;


#define	MAX_LBM_HEIGHT		480

#define BACKFACE_EPSILON	0.01

//====================================================
/* Knightmare- moved these here from gl_rsurf.c */

#define LIGHTMAP_BYTES 4

#define	LM_BLOCK_WIDTH	128
#define	LM_BLOCK_HEIGHT	128

#define GL_LIGHTMAP_FORMAT GL_BGRA	// was GL_RGBA
#define GL_LIGHTMAP_TYPE GL_UNSIGNED_INT_8_8_8_8_REV	// was GL_UNSIGNED_BYTE

#define BATCH_LM_UPDATES

typedef struct
{
	unsigned int	left;
	unsigned int	right;
	unsigned int	top;
	unsigned int	bottom;
} rect_t;

typedef struct
{
	int			internal_format;
	int			external_format;	/* Knightmare added */
	int			type;				/* Knightmare added */
	int			current_lightmap_texture;

	msurface_t	*lightmap_surfaces[MAX_LIGHTMAPS];

	int			allocated[LM_BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	unsigned	lightmap_buffer[LM_BLOCK_WIDTH*LM_BLOCK_HEIGHT];
#ifdef BATCH_LM_UPDATES /* Knightmare added */
	unsigned	*lightmap_update[MAX_LIGHTMAPS];
	rect_t		lightrect[MAX_LIGHTMAPS];
	qboolean	modified[MAX_LIGHTMAPS];
#endif
} gllightmapstate_t;

extern gllightmapstate_t gl_lms;

//====================================================

extern	image_t		gltextures[MAX_GLTEXTURES];
extern	int			numgltextures;


extern	image_t		*r_notexture;
extern	image_t		*r_particletexture;
extern	entity_t	*currententity;
extern	int			r_worldframe;	/* Knightmare- added for trans animations */
extern	model_t		*currentmodel;
extern	int			r_visframecount;
extern	int			r_framecount;
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;


extern	int			gl_filter_min, gl_filter_max;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_speeds;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_lerpmodels;
extern	cvar_t	*r_ignorehwgamma; /* Knightmare- hardware gamma */
extern	cvar_t	*r_displayrefresh; /* Knightmare- refresh rate control */

extern	cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern cvar_t	*gl_vertex_arrays;

extern cvar_t	*gl_ext_swapinterval;
extern cvar_t	*gl_ext_palettedtexture;
extern cvar_t	*gl_ext_multitexture;
extern cvar_t	*gl_ext_pointparameters;
extern cvar_t	*gl_ext_compiled_vertex_array;
extern cvar_t	*gl_arb_texturenonpoweroftwo;	/* Knightmare- non-power-of-two texture support */
extern cvar_t	*gl_nonpoweroftwo_mipmaps;		/* Knightmare- non-power-of-two texture support */

extern cvar_t	*gl_newtextureformat;			/* Knightmare- whether to use RGBA textures / BGRA lightmaps */

extern cvar_t	*gl_particle_min_size;
extern cvar_t	*gl_particle_max_size;
extern cvar_t	*gl_particle_size;
extern cvar_t	*gl_particle_att_a;
extern cvar_t	*gl_particle_att_b;
extern cvar_t	*gl_particle_att_c;

extern	cvar_t	*r_entity_fliproll;		/* Knightmare- allow disabling of backwards alias model roll */
extern	cvar_t	*r_lightcutoff;	//** DMP - allow dynamic light cutoff to be user-settable

extern	cvar_t	*gl_nosubimage;
extern	cvar_t	*gl_bitdepth;
extern	cvar_t	*gl_mode;
extern	cvar_t	*gl_log;
extern	cvar_t	*gl_lightmap;
extern	cvar_t	*gl_shadows;
extern	cvar_t	*gl_shadowalpha; /* Knightmare- added shadow alpha */
extern	cvar_t	*gl_dynamic;
extern  cvar_t  *gl_monolightmap;
extern	cvar_t	*gl_nobind;
extern	cvar_t	*gl_round_down;
extern	cvar_t	*gl_picmip;
extern	cvar_t	*gl_skymip;
extern	cvar_t	*gl_showtris;
extern	cvar_t	*gl_showbbox;	/* Knightmare- show model bounding box */
extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_ztrick;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_cull;
extern	cvar_t	*gl_poly;
extern	cvar_t	*gl_texsort;
extern	cvar_t	*gl_polyblend;
extern	cvar_t	*gl_flashblend;
extern	cvar_t	*gl_lightmaptype;
extern	cvar_t	*gl_modulate;
extern	cvar_t	*gl_playermip;
extern	cvar_t	*gl_drawbuffer;
extern	cvar_t	*gl_3dlabs_broken;
extern  cvar_t  *gl_driver;
extern	cvar_t	*gl_swapinterval;
extern	cvar_t	*gl_anisotropic;		/* Knightmare- added anisotropic filtering */
extern	cvar_t	*gl_anisotropic_avail;	/* Knightmare- added anisotropic filtering */
extern	cvar_t	*gl_texturemode;
extern	cvar_t	*gl_texturealphamode;
extern	cvar_t	*gl_texturesolidmode;
extern  cvar_t  *gl_saturatelighting;
extern  cvar_t  *gl_lockpvs;

extern	cvar_t	*vid_fullscreen;
extern	cvar_t	*vid_gamma;

extern	cvar_t	*intensity;

extern	cvar_t	*r_skydistance; /* Knightmare- variable sky range */

/* FS: New stuff */
extern	cvar_t	*r_refreshrate;

extern	int		gl_lightmap_format;
extern	int		gl_solid_format;
extern	int		gl_alpha_format;
extern	int		gl_tex_solid_format;
extern	int		gl_tex_alpha_format;

extern	int		c_visible_lightmaps;
extern	int		c_visible_textures;

extern	float	r_world_matrix[16];

void R_TranslatePlayerSkin (int playernum);
void GL_Bind (int texnum);
void GL_MBind( GLenum target, int texnum );
void GL_TexEnv( GLenum value );
void GL_EnableMultitexture( qboolean enable );
void GL_SelectTexture( GLenum );

void R_LightPoint (vec3_t p, vec3_t color);
void R_PushDlights (void);

//====================================================================

extern	model_t	*r_worldmodel;

extern	unsigned	d_8to24table[256];

extern	int		registration_sequence;


void V_AddBlend (float r, float g, float b, float a, float *v_blend);

int 	R_Init( void *hinstance, void *hWnd );
void	R_Shutdown( void );

void R_RenderView (refdef_t *fd);
void GL_ScreenShot_f (void);
void GL_ScreenShot_Silent_f (void);
void R_DrawAliasModel (entity_t *e);
void R_DrawBrushModel (entity_t *e);
void R_DrawSpriteModel (entity_t *e);
void R_DrawBeam( entity_t *e );
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawAlphaSurfaces (void);
void R_RenderBrushPoly (msurface_t *fa);
void R_InitParticleTexture (void);
void Draw_InitLocal (void);
void GL_SubdivideSurface (msurface_t *fa);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_RotateForEntity (entity_t *e, qboolean full);
int R_RollMult (void);
void R_MarkLeaves (void);

glpoly_t *WaterWarpPolyVerts (glpoly_t *p);
void EmitWaterPolys (msurface_t *fa);
void R_AddSkySurface (msurface_t *fa);
void R_ClearSkyBox (void);
void R_DrawSkyBox (void);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);

#if 0
short LittleShort (short l);
short BigShort (short l);
int	LittleLong (int l);
float LittleFloat (float f);

char	*va(char *format, ...);
// does a varargs printf into a temp buffer
#endif

void COM_StripExtension (char *in, char *out);

void	Draw_GetPicSize (int *w, int *h, char *name);
void	Draw_Pic (int x, int y, char *name);
void	Draw_StretchPic (int x, int y, int w, int h, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_BeginFrame( float camera_separation );
void	R_SwapBuffers( int );
void	R_SetPalette ( const unsigned char *palette);

int		Draw_GetPalette (void);

//void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight);
void GL_ResampleTexture (void *indata, int inwidth, int inheight, void *outdata,  int outwidth, int outheight);

struct image_s *R_RegisterSkin (char *name);

void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits);
image_t	*GL_FindImage (char *name, imagetype_t type);
void	GL_UpdateAnisoMode (void);	/* Knightmare- added anisotropic filter update */
void	GL_TextureMode( char *string );
void	GL_ImageList_f (void);
int nearest_power_of_2 (int size);

void	GL_SetTexturePalette( unsigned palette[256] );

void	GL_InitImages (void);
void	GL_ShutdownImages (void);

void	GL_FreeUnusedImages (void);

void GL_TextureAlphaMode( char *string );
void GL_TextureSolidMode( char *string );

/*
** GL extension emulation functions
*/
void GL_DrawParticles( int n, const particle_t particles[], const unsigned colortable[768] );

/*
** GL config stuff
*/
/* Knightmare- new vendor list */
enum {
	GL_RENDERER_DEFAULT		= 1 << 0,

	GL_RENDERER_MCD			= 1 << 1,
	GL_RENDERER_3DLABS		= 1 << 2,
	GL_RENDERER_GLINT_MX	= 1 << 3,
	GL_RENDERER_PCX1		= 1 << 4,
	GL_RENDERER_PCX2		= 1 << 5,
	GL_RENDERER_PERMEDIA2	= 1 << 6,
	GL_RENDERER_PMX			= 1 << 7,
	GL_RENDERER_POWERVR		= 1 << 8,
	GL_RENDERER_REALIZM		= 1 << 9,
	GL_RENDERER_RENDITION	= 1 << 10,
	GL_RENDERER_SGI			= 1 << 11,
	GL_RENDERER_SIS			= 1 << 12,
	GL_RENDERER_VOODOO		= 1 << 13,

	GL_RENDERER_NVIDIA		= 1 << 14,
	GL_RENDERER_GEFORCE		= 1 << 15,

	GL_RENDERER_ATI			= 1 << 16,
	GL_RENDERER_RADEON		= 1 << 17,

	GL_RENDERER_MATROX		= 1 << 18,
	GL_RENDERER_INTEL		= 1 << 19
};

typedef struct
{
	int         renderer;
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *extensions_string;
	/* Knightmare- for parsing newer OpenGL versions */
	int			version_major;
	int			version_minor;
	int			version_release;

	qboolean	allow_cds;

	/* Knightmare added */
	qboolean	multitexture;				// multitexture is enabled
	int			max_texsize;				// max texture size
	int			max_texunits;				// texunits available

	qboolean	arbTextureNonPowerOfTwo; 	// non-power-of-two texture support
	qboolean	have_stencil;				// stencil buffer

	qboolean	anisotropic;				// anisotropic filtering
	float		max_anisotropy;

	qboolean	newTexFormat;			// whether to use GL_RGBA textures / GL_BGRA lightmaps

} glconfig_t;

typedef struct
{
	float inverse_intensity;
	qboolean fullscreen;

	int     prev_mode;

	unsigned char *d_16to8table;

	int lightmap_textures;

	int	currenttextures[2];
	int currenttmu;

	float		camera_separation;
	qboolean	stereo_enabled;

	qboolean	gammaRamp;
	qboolean	multitextureEnabled;	/* Knightmare added */

	unsigned char originalRedGammaTable[256];
	unsigned char originalGreenGammaTable[256];
	unsigned char originalBlueGammaTable[256];
} glstate_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( float camera_separation );
void		GLimp_EndFrame( void );
int 		GLimp_Init( void *hinstance, void *hWnd );
void		GLimp_Shutdown( void );
rserr_t 	GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen );
void		GLimp_AppActivate( qboolean active );
void		GLimp_EnableLogging( qboolean enable );
void		GLimp_LogNewFrame( void );

