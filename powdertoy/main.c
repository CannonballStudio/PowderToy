/**
 * Powder Toy - Main source
 *
 * Copyright (c) 2008 - 2011 Stanislaw Skowronek.
 * Copyright (c) 2010 - 2011 Simon Robertshaw
 * Copyright (c) 2010 - 2011 Skresanov Savely
 * Copyright (c) 2010 - 2011 Bryan Hoyle
 * Copyright (c) 2010 - 2011 Nathan Cousins
 * Copyright (c) 2010 - 2011 cracker64
 * Copyright (c) 2011 jacksonmj
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "SDL/SDL.h"
#include <time.h>
#include <pthread.h>

#include <sys/stat.h>
#include <unistd.h>

#include "misc.h"
#include "font.h"
#include "powder.h"
#include "gravity.h"
#include "graphics.h"
#include "powdergraphics.h"
#include "md5.h"
#include "hmap.h"
#include "air.h"
#include "console.h"
#ifdef LUACONSOLE
#include "luaconsole.h"
#endif
#include "save.h"

#include "uiwrapper.h"

pixel *vid_buf;
int bsx = 2, bsy = 2;

extern SDL_Window *window;

typedef struct
{
	int start, inc;
	pixel *vid;
} upstruc;

/*static const char *old_ver_msg_beta = "A new beta is available - click here!";
static const char *old_ver_msg = "A new version is available - click here!";*/
char new_message_msg[255];
float mheat = 0.0f;

int saveURIOpen = 0;
char * saveDataOpen = NULL;
int saveDataOpenSize = 0;

int do_open = 0;
int sys_pause = 0;
int sys_shortcuts = 1;
int legacy_enable = 0; //Used to disable new features such as heat, will be set by save.
int aheat_enable; //Ambient heat
int decorations_enable = 1;
int hud_enable = 1;
int active_menu = 0;
int framerender = 0;
int pretty_powder = 0;
int amd = 1;
int FPSB = 0;
int MSIGN =-1;
int frameidx = 0;
//int CGOL = 0;
//int GSPEED = 1;//causes my .exe to crash..
int loop_time = 0;

int debug_flags = 0;
int debug_perf_istart = 1;
int debug_perf_iend = 0;
long debug_perf_frametime[DEBUG_PERF_FRAMECOUNT];
long debug_perf_partitime[DEBUG_PERF_FRAMECOUNT];
long debug_perf_time = 0;

sign signs[MAXSIGNS];

int numCores = 1;

int mousex = 0, mousey = 0;  //They contain mouse position
int kiosk_enable = 0;

int frame_idx=0;
void dump_frame(pixel *src, int w, int h, int pitch)
{
	char frame_name[32];
	int j,i;
	unsigned char c[3];
	FILE *f;
	sprintf(frame_name,"frame%04d.ppm",frame_idx);
	f=fopen(frame_name,"wb");
	fprintf(f,"P6\n%d %d\n255\n",w,h);
	for (j=0; j<h; j++)
	{
		for (i=0; i<w; i++)
		{
			c[0] = PIXR(src[i]);
			c[1] = PIXG(src[i]);
			c[2] = PIXB(src[i]);
			fwrite(c,3,1,f);
		}
		src+=pitch;
	}
	fclose(f);
	frame_idx++;
}

/***********************************************************
 *                    STATE MANAGEMENT                     *
 ***********************************************************/

char bframe = 0;

void clear_sim(void)
{
	int i, x, y;
	memset(bmap, 0, sizeof(bmap));
	memset(emap, 0, sizeof(emap));
	memset(signs, 0, sizeof(signs));
	MSIGN = -1;
	memset(parts, 0, sizeof(particle)*NPART);
	for (i=0; i<NPART-1; i++)
		parts[i].life = i+1;
	parts[NPART-1].life = -1;
	pfree = 0;
	parts_lastActiveIndex = 0;
	memset(pmap, 0, sizeof(pmap));
	memset(pv, 0, sizeof(pv));
	memset(vx, 0, sizeof(vx));
	memset(vy, 0, sizeof(vy));
	memset(fvx, 0, sizeof(fvx));
	memset(fvy, 0, sizeof(fvy));
	memset(photons, 0, sizeof(photons));
	memset(wireless, 0, sizeof(wireless));
	memset(gol2, 0, sizeof(gol2));
	memset(portalp, 0, sizeof(portalp));
	memset(fighters, 0, sizeof(fighters));
	fighcount = 0;
	ISSPAWN1 = ISSPAWN2 = 0;
	player.spwn = 0;
	player2.spwn = 0;
	emp_decor = 0;
	memset(pers_bg, 0, (XRES+BARSIZE)*YRES*PIXELSIZE);
	memset(fire_r, 0, sizeof(fire_r));
	memset(fire_g, 0, sizeof(fire_g));
	memset(fire_b, 0, sizeof(fire_b));
	if(gravmask)
		memset(gravmask, 0xFFFFFFFF, (XRES/CELL)*(YRES/CELL)*sizeof(unsigned));
	if(gravy)
		memset(gravy, 0, (XRES/CELL)*(YRES/CELL)*sizeof(float));
	if(gravx)
		memset(gravx, 0, (XRES/CELL)*(YRES/CELL)*sizeof(float));
	if(gravp)
		memset(gravp, 0, (XRES/CELL)*(YRES/CELL)*sizeof(float));
	for(x = 0; x < XRES/CELL; x++){
		for(y = 0; y < YRES/CELL; y++){
			hv[y][x] = 273.15f+22.0f; //Set to room temperature
		}
	}
	if(bframe)
		draw_bframe();
}

// stamps library

stamp stamps[STAMP_MAX];//[STAMP_X*STAMP_Y];

int stamp_count = 0;

unsigned last_time=0, last_name=0;
void stamp_gen_name(char *fn)
{
	unsigned t=(unsigned)time(NULL);

	if (last_time!=t)
	{
		last_time=t;
		last_name=0;
	}
	else
		last_name++;

	sprintf(fn, "%08x%02x", last_time, last_name);
}

void stamp_update(void)
{
	FILE *f;
	int i;
	f=fopen("stamps" PATH_SEP "stamps.def", "wb");
	if (!f)
		return;
	for (i=0; i<STAMP_MAX; i++)
	{
		if (!stamps[i].name[0])
			break;
		if (stamps[i].dodelete!=1)
		{
			fwrite(stamps[i].name, 1, 10, f);
		}
		else
		{
			char name[30] = {0};
			sprintf(name,"stamps%s%s.stm",PATH_SEP,stamps[i].name);
			remove(name);
		}
	}
	fclose(f);
}

void stamp_gen_thumb(int i)
{
	char fn[64];
	void *data;
	int size, factor_x, factor_y;
	pixel *tmp;

	if (stamps[i].thumb)
	{
		free(stamps[i].thumb);
		stamps[i].thumb = NULL;
	}

	sprintf(fn, "stamps" PATH_SEP "%s.stm", stamps[i].name);
	data = file_load(fn, &size);

	if (data)
	{
		stamps[i].thumb = prerender_save(data, size, &(stamps[i].thumb_w), &(stamps[i].thumb_h));
		if (stamps[i].thumb && (stamps[i].thumb_w>XRES/GRID_S || stamps[i].thumb_h>YRES/GRID_S))
		{
			factor_x = ceil((float)stamps[i].thumb_w/(float)(XRES/GRID_S));
			factor_y = ceil((float)stamps[i].thumb_h/(float)(YRES/GRID_S));
			if (factor_y > factor_x)
				factor_x = factor_y;
			tmp = rescale_img(stamps[i].thumb, stamps[i].thumb_w, stamps[i].thumb_h, &(stamps[i].thumb_w), &(stamps[i].thumb_h), factor_x);
			free(stamps[i].thumb);
			stamps[i].thumb = tmp;
		}
	}

	free(data);
}

int clipboard_ready = 0;
void *clipboard_data = 0;
int clipboard_length = 0;

void stamp_save(int x, int y, int w, int h)
{
	FILE *f;
	int n;
	char fn[64], sn[16];
	void *s=build_save(&n, x, y, w, h, bmap, vx, vy, pv, fvx, fvy, signs, parts);
	if (!s)
		return;

	mkdir("stamps", 0755);
	stamp_gen_name(sn);
	sprintf(fn, "stamps" PATH_SEP "%s.stm", sn);

	f = fopen(fn, "wb");
	if (!f)
		return;
	fwrite(s, n, 1, f);
	fclose(f);

	free(s);

	if (stamps[STAMP_MAX-1].thumb)
		free(stamps[STAMP_MAX-1].thumb);
	memmove(stamps+1, stamps, sizeof(struct stamp)*(STAMP_MAX-1));
	memset(stamps, 0, sizeof(struct stamp));
	if (stamp_count<STAMP_MAX)
		stamp_count++;

	strcpy(stamps[0].name, sn);
	stamp_gen_thumb(0);

	stamp_update();
}

void *stamp_load(int i, int *size)
{
	void *data;
	char fn[64];
	struct stamp tmp;

	if (!stamps[i].thumb || !stamps[i].name[0])
		return NULL;

	sprintf(fn, "stamps" PATH_SEP "%s.stm", stamps[i].name);
	data = file_load(fn, size);
	if (!data)
		return NULL;

	if (i>0)
	{
		memcpy(&tmp, stamps+i, sizeof(struct stamp));
		memmove(stamps+1, stamps, sizeof(struct stamp)*i);
		memcpy(stamps, &tmp, sizeof(struct stamp));

		stamp_update();
	}

	return data;
}

void stamp_init(void)
{
	int i;
	FILE *f;

	memset(stamps, 0, sizeof(stamps));

	f=fopen("stamps" PATH_SEP "stamps.def", "rb");
	if (!f)
		return;
	for (i=0; i<STAMP_MAX; i++)
	{
		fread(stamps[i].name, 1, 10, f);
		if (!stamps[i].name[0])
			break;
		stamp_count++;
		stamp_gen_thumb(i);
	}
	fclose(f);
}

void del_stamp(int d)
{
	stamps[d].dodelete = 1;
	stamp_update();
	stamp_count = 0;
	stamp_init();
}

void thumb_cache_inval(char *id);

char *thumb_cache_id[THUMB_CACHE_SIZE];
void *thumb_cache_data[THUMB_CACHE_SIZE];
int thumb_cache_size[THUMB_CACHE_SIZE];
int thumb_cache_lru[THUMB_CACHE_SIZE];

void thumb_cache_inval(char *id)
{
	int i,j;
	for (i=0; i<THUMB_CACHE_SIZE; i++)
		if (thumb_cache_id[i] && !strcmp(id, thumb_cache_id[i]))
			break;
	if (i >= THUMB_CACHE_SIZE)
		return;
	free(thumb_cache_id[i]);
	free(thumb_cache_data[i]);
	thumb_cache_id[i] = NULL;
	for (j=0; j<THUMB_CACHE_SIZE; j++)
		if (thumb_cache_lru[j] > thumb_cache_lru[i])
			thumb_cache_lru[j]--;
}
void thumb_cache_add(char *id, void *thumb, int size)
{
	int i,m=-1,j=-1;
	thumb_cache_inval(id);
	for (i=0; i<THUMB_CACHE_SIZE; i++)
	{
		if (!thumb_cache_id[i])
			break;
		if (thumb_cache_lru[i] > m)
		{
			m = thumb_cache_lru[i];
			j = i;
		}
	}
	if (i >= THUMB_CACHE_SIZE)
	{
		thumb_cache_inval(thumb_cache_id[j]);
		i = j;
	}
	for (j=0; j<THUMB_CACHE_SIZE; j++)
		thumb_cache_lru[j] ++;
	thumb_cache_id[i] = mystrdup(id);
	thumb_cache_data[i] = malloc(size);
	memcpy(thumb_cache_data[i], thumb, size);
	thumb_cache_size[i] = size;
	thumb_cache_lru[i] = 0;
}
int thumb_cache_find(char *id, void **thumb, int *size)
{
	int i,j;
	for (i=0; i<THUMB_CACHE_SIZE; i++)
		if (thumb_cache_id[i] && !strcmp(id, thumb_cache_id[i]))
			break;
	if (i >= THUMB_CACHE_SIZE)
		return 0;
	for (j=0; j<THUMB_CACHE_SIZE; j++)
		if (thumb_cache_lru[j] < thumb_cache_lru[i])
			thumb_cache_lru[j]++;
	thumb_cache_lru[i] = 0;
	*thumb = malloc(thumb_cache_size[i]);
	*size = thumb_cache_size[i];
	memcpy(*thumb, thumb_cache_data[i], *size);
	return 1;
}

unsigned char last_major=0, last_minor=0, last_build=0;

char *tag = "(c) 2008-9 Stanislaw Skowronek";
int itc = 0;
char itc_msg[64] = "[?]";

int set_scale(int scale, int kiosk){
	int old_scale = sdl_scale, old_kiosk = kiosk_enable;
	sdl_scale = scale;
	kiosk_enable = kiosk;
	if (!sdl_open())
	{
		sdl_scale = old_scale;
		kiosk_enable = old_kiosk;
		sdl_open();
		return 0;
	}
	return 1;
}

void undo() {
    int cbx, cby, cbi;
    
    for (cbi=0; cbi<NPART; cbi++)
        parts[cbi] = cb_parts[cbi];
    parts_lastActiveIndex = NPART-1;
    
    for (cby = 0; cby<YRES; cby++)
        for (cbx = 0; cbx<XRES; cbx++)
            pmap[cby][cbx] = cb_pmap[cby][cbx];
    
    for (cby = 0; cby<(YRES/CELL); cby++)
        for (cbx = 0; cbx<(XRES/CELL); cbx++)
        {
            vx[cby][cbx] = cb_vx[cby][cbx];
            vy[cby][cbx] = cb_vy[cby][cbx];
            pv[cby][cbx] = cb_pv[cby][cbx];
            hv[cby][cbx] = cb_hv[cby][cbx];
            bmap[cby][cbx] = cb_bmap[cby][cbx];
            emap[cby][cbx] = cb_emap[cby][cbx];
        }
    
    force_stacking_check = 1;//check for excessive stacking of particles next time update_particles is run
}

void erase_all() {
    clear_sim();
    int i;
    for (i=0; i<NPART-1; i++)
        parts[i].life = i+1;
    parts[NPART-1].life = -1;
    pfree = 0;
    
    legacy_enable = 0;
    svf_filename[0] = 0;
    svf_fileopen = 0;
    svf_myvote = 0;
    svf_open = 0;
    svf_publish = 0;
    svf_own = 0;
    svf_id[0] = 0;
    svf_name[0] = 0;
    svf_tags[0] = 0;
    svf_description[0] = 0;
    gravityMode = 0;
    airMode = 0;
    svf_last = NULL;
    svf_lsize = 0;
}

void save_simulation(const char *filename)
{
    int save_size;
	void *save_data = build_save(&save_size, 0, 0, XRES, YRES, bmap, vx, vy, pv, fvx, fvy, signs, parts);
    FILE *f = fopen(filename, "wb");
    if (f)
    {
        fwrite(save_data, save_size, 1, f);
        fclose(f);
    }
}

void load_simulation(const char *filename) {
    int size;
    void *data=file_load((char *)filename, &size);
    if(data) {
        parse_save(data, size, 1, 0, 0, bmap, vx, vy, pv, fvx, fvy, signs, parts, pmap);
        free(data);
    }
}

/*long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

long long stard;

void start() {
    stard=current_timestamp();
}

void stop() {
    long long ts=current_timestamp()-stard;
    printf("time: %lld\n",ts);
}*/

int main(int argc, char *argv[])
{
	pixel *part_vbuf; //Extra video buffer
	pixel *part_vbuf_store;
	char uitext[512] = "";
	char heattext[256] = "";
	char coordtext[128] = "";
	int currentTime = 0;
	int FPS = 0, pastFPS = 0, elapsedTime = 0;
	char *ver_data=NULL, *check_data=NULL, *tmp;
	//char console_error[255] = "";
	int result, i, j, bq, bc = 0, do_check=0, do_s_check=0, /*old_version=0,*/ major, minor, buildnum, is_beta = 0, /*old_ver_len,*/ new_message_len=0;
#ifdef INTERNAL
	int vs = 0;
#endif
	//int wavelength_gfx = 0;
	int x, y, line_x, line_y, b = 0, sl=1, sr=0, su=0, c, lb = 0, lx = 0, ly = 0, lm = 0;//, tx, ty;
	int da = 0, dae = 0, db = 0, mx, my;//, quickoptions_tooltip_fade_invert;
	float nfvx, nfvy;
	int load_mode=0, load_w=0, load_h=0, load_x=0, load_y=0, load_size=0;
	void *load_data=NULL;
	pixel *load_img=NULL;//, *fbi_img=NULL;
	int save_mode=0, save_x=0, save_y=0, save_w=0, save_h=0, copy_mode=0;
	unsigned int rgbSave = PIXRGB(127,0,0);
	int username_flash = 0, username_flash_t = 1;
	int saveOpenError = 0;
	limitFPS = 50;
	vid_buf = calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE);
	part_vbuf = calloc((XRES+BARSIZE)*(YRES+MENUSIZE), PIXELSIZE); //Extra video buffer
	part_vbuf_store = part_vbuf;
	pers_bg = calloc((XRES+BARSIZE)*YRES, PIXELSIZE);

	gravity_init();
	GSPEED = 1;

    //TODO: Move out version stuff
	menu_count();
	parts = calloc(sizeof(particle), NPART);
	cb_parts = calloc(sizeof(particle), NPART);
	init_can_move();
	
#ifdef LUACONSOLE
	luacon_open();
#endif
	
	colour_mode = COLOUR_DEFAULT;
	init_display_modes();
	TRON_init_graphics();

	//fbi_img = render_packed_rgb(fbi, FBI_W, FBI_H, FBI_CMP);

	for (i=1; i<argc; i++)
	{
		if (!strncmp(argv[i], "ddir", 5) && i+1<argc)
		{
			chdir(argv[i+1]);
			i++;
		}
		else if (!strncmp(argv[i], "ptsave", 7) && i+1<argc)
		{
			//Prevent reading of any arguments after ptsave for security
			i++;
			argc = i+2;
			break;
		}
		else if (!strncmp(argv[i], "open", 5) && i+1<argc)
		{
			saveDataOpen = file_load(argv[i+1], &saveDataOpenSize);
			i++;
		}
	}
	
	//load_presets();
	clear_sim();

	for (i=1; i<argc; i++)
	{
		if (!strncmp(argv[i], "scale:", 6))
		{
			sdl_scale = (argv[i][6]=='2') ? 2 : 1;
		}
		else if (!strncmp(argv[i], "nohud", 5))
		{
			hud_enable = 0;
		}
		else if (!strncmp(argv[i], "kiosk", 5))
		{
			kiosk_enable = 1;
			//sdl_scale = 2; //Removed because some displays cannot handle the resolution
			hud_enable = 0;
		}
		else if (!strncmp(argv[i], "scripts", 8))
		{
			file_script = 1;
		}
		else if (!strncmp(argv[i], "open", 5) && i+1<argc)
		{
			i++;
		}
		else if (!strncmp(argv[i], "ddir", 5) && i+1<argc)
		{
			i++;
		}
		else if (!strncmp(argv[i], "ptsave", 7) && i+1<argc)
		{
			int ci = 0, ns = 0, okay = 0;
			char * tempString = argv[i+1];
			int tempStringLength = strlen(argv[i+1])-7;
			int tempSaveID = 0;
			char tempNumberString[32];
			puts("Got ptsave");
			i++;
			tempNumberString[31] = 0;
			tempNumberString[0] = 0;
			if(!strncmp(tempString, "ptsave:", 7) && tempStringLength)
			{
				puts("ptsave:// protocol");
				tempString+=7;
				while(tempString[ci] && ns<30 && ci<tempStringLength)
				{
					if(tempString[ci]>=48 && tempString[ci]<=57)
					{
						tempNumberString[ns++] = tempString[ci];
						tempNumberString[ns] = 0;
					}
					else if(tempString[ci]=='#')
					{
						okay = 1;
						break;
					}
					else
					{
						puts("ptsave: invalid save ID");
						break;
					}
					ci++;
				}
				if(!tempString[ci])
				{
					okay = 1;
				}
				if(okay)
				{
					tempSaveID = atoi(tempNumberString);
				}
			}
			if(tempSaveID > 0)
			{
				puts("Got ptsave:id");
				saveURIOpen = tempSaveID;
			}
			break;
		}
	}

	make_kernel();

	stamp_init();

	if (!sdl_open())
	{
		sdl_scale = 1;
		kiosk_enable = 0;
		if (!sdl_open()) exit(1);
	}
	//save_presets(0);

	prepare_alpha(CELL, 1.0f);
	prepare_graphicscache();
	flm_data = generate_gradient(flm_data_colours, flm_data_pos, flm_data_points, 200);
	plasma_data = generate_gradient(plasma_data_colours, plasma_data_pos, plasma_data_points, 200);
	
	if(saveOpenError)
	{
		saveOpenError = 0;
		error_ui(vid_buf, 0, "Unable to open save file.");
	}
#ifdef LUACONSOLE
	luacon_eval("dofile(\"autorun.lua\")"); //Autorun lua script
#endif
	while (!sdl_poll()) //the main loop
	{
		frameidx++;
		frameidx %= 30;
		if (!sys_pause||framerender) //only update air if not paused
		{
			update_air();
			if(aheat_enable)
				update_airh();
		}

		if(gravwl_timeout)
		{
			if(gravwl_timeout==1)
				gravity_mask();
			gravwl_timeout--;
		}

		//Can't be too sure (Limit the cursor size)
		if (bsx>1180)
			bsx = 1180;
		if (bsx<0)
			bsx = 0;
		if (bsy>1180)
			bsy = 1180;
		if (bsy<0)
			bsy = 0;
			
		//Pretty powders, colour cycle
		//sandcolour_r = 0;
		//sandcolour_g = 0;
		sandcolour_b = sandcolour_r = sandcolour_g = (int)(20.0f*sin((float)sandcolour_frame*(M_PI/180.0f)));
		sandcolour_frame++;
		sandcolour_frame%=360;

#ifdef OGLR
		part_vbuf = vid_buf;
#else
		if(ngrav_enable && (display_mode & DISPLAY_WARP))
		{
			part_vbuf = part_vbuf_store;
			memset(vid_buf, 0, (XRES+BARSIZE)*YRES*PIXELSIZE);
		} else {
			part_vbuf = vid_buf;
		}
#endif
		render_before(part_vbuf);
		update_particles(part_vbuf); //update everything
        render_after(part_vbuf, vid_buf);
        
		if(su == WL_GRAV+100)
			draw_grav_zones(part_vbuf);
		
		if(debug_flags & (DEBUG_PERFORMANCE_CALC|DEBUG_PERFORMANCE_FRAME))
		{
			debug_perf_iend++;
			debug_perf_iend %= DEBUG_PERF_FRAMECOUNT;
			debug_perf_istart++;
			debug_perf_istart %= DEBUG_PERF_FRAMECOUNT;
		}
		
		gravity_update_async(); //Check for updated velocity maps from gravity thread
		if (!sys_pause||framerender) //Only update if not paused
			memset(gravmap, 0, (XRES/CELL)*(YRES/CELL)*sizeof(float)); //Clear the old gravmap

		if (framerender) {
			framerender = 0;
			sys_pause = 1;
		}

		memset(vid_buf+((XRES+BARSIZE)*YRES), 0, (PIXELSIZE*(XRES+BARSIZE))*MENUSIZE);//clear menu areas
		clearrect(vid_buf, XRES-1, 0, BARSIZE+1, YRES);

		draw_svf_ui(vid_buf, sdl_mod & (KMOD_LCTRL|KMOD_RCTRL));
		
		if(debug_flags)
		{
			draw_debug_info(vid_buf, lm, lx, ly, x, y, line_x, line_y);
		}

		if (saveDataOpen)
		{
			//Clear all settings and simulation data
			clear_sim();
			legacy_enable = 0;
			svf_filename[0] = 0;
			svf_fileopen = 0;
			svf_myvote = 0;
			svf_open = 0;
			svf_publish = 0;
			svf_own = 0;
			svf_id[0] = 0;
			svf_name[0] = 0;
			svf_tags[0] = 0;
			svf_description[0] = 0;
			gravityMode = 0;
			airMode = 0;
			
			svf_last = saveDataOpen;
			svf_lsize = saveDataOpenSize;
			if(parse_save(saveDataOpen, saveDataOpenSize, 1, 0, 0, bmap, fvx, fvy, vx, vy, pv, signs, parts, pmap))
			{
				saveOpenError = 1;
				svf_last = NULL;
				svf_lsize = 0;
				free(saveDataOpen);
			}
			saveDataOpenSize = 0;
			saveDataOpen = NULL;
		}
#ifdef LUACONSOLE
	if(sdl_key){
		if(!luacon_keyevent(sdl_key, sdl_mod, LUACON_KDOWN))
			sdl_key = 0;
	}
	if(sdl_rkey){
		if(!luacon_keyevent(sdl_rkey, sdl_mod, LUACON_KUP))
			sdl_rkey = 0;
	}
#endif
		if (sys_shortcuts==1)//all shortcuts can be disabled by python scripts
		{
			stickmen_keys();
			if (sdl_key=='q' || sdl_key==SDLK_ESCAPE)
			{
				if (confirm_ui(vid_buf, "You are about to quit", "Are you sure you want to quit?", "Quit"))
				{
					break;
				}
			}
			if (sdl_key=='f')
			{
				framerender = 1;
			}
			if ((sdl_key=='l' || sdl_key=='k') && stamps[0].name[0])
			{
				if (load_mode)
				{
					free(load_img);
					free(load_data);
					load_mode = 0;
					load_data = NULL;
					load_img = NULL;
				}
				if (sdl_key=='k' && stamps[1].name[0])
				{
					j = stamp_ui(vid_buf);
					if (j>=0)
						load_data = stamp_load(j, &load_size);
					else
						load_data = NULL;
				}
				else
					load_data = stamp_load(0, &load_size);
				if (load_data)
				{
					load_img = prerender_save(load_data, load_size, &load_w, &load_h);
					if (load_img)
						load_mode = 1;
					else
						free(load_data);
				}
			}
			if (sdl_key=='s' && ((sdl_mod & (KMOD_CTRL)) || !player2.spwn))
			{
				save_mode = 1;
			}
			if(sdl_key=='e')
			{
				element_search_ui(vid_buf, &sl, &sr);
			}
			//TODO: Superseded by new display mode switching, need some keyboard shortcuts
			if (sdl_key=='1')
			{
				set_cmode(CM_VEL);
			}
			if (sdl_key=='2')
			{
				set_cmode(CM_PRESS);
			}
			if (sdl_key=='3')
			{
				set_cmode(CM_PERS);
			}
			if (sdl_key=='4')
			{
				set_cmode(CM_FIRE);
			}
			if (sdl_key=='5')
			{
				set_cmode(CM_BLOB);
			}
			if (sdl_key=='6')
			{
				set_cmode(CM_HEAT);
			}
			if (sdl_key=='7')
			{
				set_cmode(CM_FANCY);
			}
			if (sdl_key=='8')
			{
				set_cmode(CM_NOTHING);
			}
			if (sdl_key=='9')
			{
				set_cmode(CM_GRAD);
			}
			if (sdl_key=='0')
			{
				set_cmode(CM_CRACK);
			}
			if (sdl_key=='1'&& (sdl_mod & (KMOD_SHIFT)) && DEBUG_MODE)
			{
				set_cmode(CM_LIFE);
			}
			if (sdl_key==SDLK_TAB)
			{
				CURRENT_BRUSH =(CURRENT_BRUSH + 1)%BRUSH_NUM ;
			}
			if (sdl_key==SDLK_LEFTBRACKET) {
				if (sdl_zoom_trig)
				{
					ZSIZE -= 1;
					if (ZSIZE>60)
						ZSIZE = 60;
					if (ZSIZE<2)
						ZSIZE = 2;
					ZFACTOR = 256/ZSIZE;
				}
				else
				{
					if (sdl_mod & (KMOD_LALT|KMOD_RALT) && !(sdl_mod & (KMOD_SHIFT|KMOD_CTRL)))
					{
						bsx -= 1;
						bsy -= 1;
					}
					else if (sdl_mod & (KMOD_SHIFT) && !(sdl_mod & (KMOD_CTRL)))
					{
						bsx -= 1;
					}
					else if (sdl_mod & (KMOD_CTRL) && !(sdl_mod & (KMOD_SHIFT)))
					{
						bsy -= 1;
					}
					else
					{
						bsx -= ceil((bsx/5)+0.5f);
						bsy -= ceil((bsy/5)+0.5f);
					}
					if (bsx>1180)
						bsx = 1180;
					if (bsy>1180)
						bsy = 1180;
					if (bsx<0)
						bsx = 0;
					if (bsy<0)
						bsy = 0;
				}
			}
			if (sdl_key==SDLK_RIGHTBRACKET) {
				if (sdl_zoom_trig)
				{
					ZSIZE += 1;
					if (ZSIZE>60)
						ZSIZE = 60;
					if (ZSIZE<2)
						ZSIZE = 2;
					ZFACTOR = 256/ZSIZE;
				}
				else
				{
					if (sdl_mod & (KMOD_LALT|KMOD_RALT) && !(sdl_mod & (KMOD_SHIFT|KMOD_CTRL)))
					{
						bsx += 1;
						bsy += 1;
					}
					else if (sdl_mod & (KMOD_SHIFT) && !(sdl_mod & (KMOD_CTRL)))
					{
						bsx += 1;
					}
					else if (sdl_mod & (KMOD_CTRL) && !(sdl_mod & (KMOD_SHIFT)))
					{
						bsy += 1;
					}
					else
					{
						bsx += ceil((bsx/5)+0.5f);
						bsy += ceil((bsy/5)+0.5f);
					}
					if (bsx>1180)
						bsx = 1180;
					if (bsy>1180)
						bsy = 1180;
					if (bsx<0)
						bsx = 0;
					if (bsy<0)
						bsy = 0;
				}
			}
			if (sdl_key=='d' && ((sdl_mod & (KMOD_CTRL)) || !player2.spwn))
				DEBUG_MODE = !DEBUG_MODE;
			if (sdl_key=='i')
			{
				int nx, ny;
				for (nx = 0; nx<XRES/CELL; nx++)
					for (ny = 0; ny<YRES/CELL; ny++)
					{
						pv[ny][nx] = -pv[ny][nx];
						vx[ny][nx] = -vx[ny][nx];
						vy[ny][nx] = -vy[ny][nx];
					}
			}
			if (sdl_key==SDLK_INSERT || sdl_key == SDLK_SEMICOLON)// || sdl_key==SDLK_BACKQUOTE)
				REPLACE_MODE = !REPLACE_MODE;
			if (sdl_key==SDLK_BACKQUOTE)
			{
				console_mode = !console_mode;
				//hud_enable = !console_mode;
			}
			if (sdl_key=='b')
			{
				if (sdl_mod & KMOD_CTRL)
				{
					decorations_enable = !decorations_enable;
					itc = 51;
					if (decorations_enable) strcpy(itc_msg, "Decorations layer: On");
					else strcpy(itc_msg, "Decorations layer: Off");
				}
				else
				{
					decorations_enable = 1;
					rgbSave = decorations_ui(vid_buf,&bsx,&bsy,rgbSave);//decoration_mode = !decoration_mode;
				}
			}
			if (sdl_key=='g')
			{
				if(sdl_mod & (KMOD_CTRL))
				{
					drawgrav_enable =! drawgrav_enable;
				}
				else
				{
					if (sdl_mod & (KMOD_SHIFT))
						GRID_MODE = (GRID_MODE+9)%10;
					else
						GRID_MODE = (GRID_MODE+1)%10;
				}
			}
			if (sdl_key=='m')
			{
				if(sl!=sr)
				{
					sl ^= sr;
					sr ^= sl;
					sl ^= sr;
				}
				dae = 51;
			}
			if (sdl_key=='=')
			{
				int nx, ny;
				if (sdl_mod & (KMOD_CTRL))
				{
					for (i=0; i<NPART; i++)
						if (parts[i].type==PT_SPRK)
						{
							if (parts[i].ctype >= 0 && parts[i].ctype < PT_NUM && ptypes[parts[i].ctype].enabled)
							{
								parts[i].type = parts[i].ctype;
								parts[i].life = 0;
							}
							else
								kill_part(i);
						}
				}
				else
				{
					for (nx = 0; nx<XRES/CELL; nx++)
						for (ny = 0; ny<YRES/CELL; ny++)
						{
							pv[ny][nx] = 0;
							vx[ny][nx] = 0;
							vy[ny][nx] = 0;
						}
				}
			}

			if (sdl_key=='w' && (!player2.spwn || (sdl_mod & (KMOD_CTRL)))) //Gravity, by Moach
			{
				++gravityMode; // cycle gravity mode
				itc = 51;

				switch (gravityMode)
				{
				default:
					gravityMode = 0;
				case 0:
					strcpy(itc_msg, "Gravity: Vertical");
					break;
				case 1:
					strcpy(itc_msg, "Gravity: Off");
					break;
				case 2:
					strcpy(itc_msg, "Gravity: Radial");
					break;

				}
			}
			if (sdl_key=='y')
			{
				++airMode;
				itc = 52;

				switch (airMode)
				{
				default:
					airMode = 0;
				case 0:
					strcpy(itc_msg, "Air: On");
					break;
				case 1:
					strcpy(itc_msg, "Air: Pressure Off");
					break;
				case 2:
					strcpy(itc_msg, "Air: Velocity Off");
					break;
				case 3:
					strcpy(itc_msg, "Air: Off");
					break;
				case 4:
					strcpy(itc_msg, "Air: No Update");
					break;
				}
			}

			if (sdl_key=='t')
				VINE_MODE = !VINE_MODE;
			if (sdl_key==SDLK_SPACE)
				sys_pause = !sys_pause;
			if (sdl_key=='u')
				aheat_enable = !aheat_enable;
			if (sdl_key=='h' && !(sdl_mod & KMOD_LCTRL))
			{
				hud_enable = !hud_enable;
			}
			if (sdl_key=='n')
				pretty_powder = !pretty_powder;
			if (sdl_key=='p')
				dump_frame(vid_buf, XRES, YRES, XRES+BARSIZE);
			if (sdl_key=='v'&&(sdl_mod & (KMOD_LCTRL|KMOD_RCTRL)))
			{
				if (clipboard_ready==1 && clipboard_data)
				{
					load_data = malloc(clipboard_length);
					memcpy(load_data, clipboard_data, clipboard_length);
					load_size = clipboard_length;
					if (load_data)
					{
						load_img = prerender_save(load_data, load_size, &load_w, &load_h);
						if (load_img)
							load_mode = 1;
						else
							free(load_data);
					}
				}
			}
			if (load_mode==1)
			{
				matrix2d transform = m2d_identity;
				vector2d translate = v2d_zero;
				void *ndata;
				int doTransform = 0;
				if (sdl_key=='r'&&(sdl_mod & (KMOD_CTRL))&&(sdl_mod & (KMOD_SHIFT)))
				{
					transform = m2d_new(-1,0,0,1); //horizontal invert
					doTransform = 1;
				}
				else if (sdl_key=='r'&&(sdl_mod & (KMOD_LCTRL|KMOD_RCTRL)))
				{
					transform = m2d_new(0,1,-1,0); //rotate anticlockwise 90 degrees
					doTransform = 1;
				}
				else if (sdl_mod & (KMOD_CTRL))
				{
					doTransform = 1;
					if (sdl_key==SDLK_LEFT) translate = v2d_new(-1,0);
					else if (sdl_key==SDLK_RIGHT) translate = v2d_new(1,0);
					else if (sdl_key==SDLK_UP) translate = v2d_new(0,-1);
					else if (sdl_key==SDLK_DOWN) translate = v2d_new(0,1);
					else doTransform = 0;
				}
				if (doTransform)
				{
					ndata = transform_save(load_data, &load_size, transform, translate);
					if (ndata!=load_data) free(load_data);
					free(load_img);
					load_data = ndata;
					load_img = prerender_save(load_data, load_size, &load_w, &load_h);
				}
			}
			if (sdl_key=='r'&&!(sdl_mod & (KMOD_CTRL|KMOD_SHIFT)))
				GENERATION = 0;
			if (sdl_key=='x'&&(sdl_mod & (KMOD_LCTRL|KMOD_RCTRL)))
			{
				save_mode = 1;
				copy_mode = 2;
			}
			if (sdl_key=='c'&&(sdl_mod & (KMOD_LCTRL|KMOD_RCTRL)))
			{
				save_mode = 1;
				copy_mode = 1;
			}
			//TODO: Superseded by new display mode switching, need some keyboard shortcuts
			/*else if (sdl_key=='c')
			{
				set_cmode((cmode+1) % CM_COUNT);
				if (it > 50)
					it = 50;
			}*/
			if (sdl_key=='z') // Undo
			{
				if (sdl_mod & (KMOD_LCTRL|KMOD_RCTRL)) undo();
				else
				{
					if (sdl_mod & KMOD_ALT)//toggle
						sdl_zoom_trig = (!sdl_zoom_trig)*2;
					else
						sdl_zoom_trig = 1;
				}
			}
			if (sdl_rkey == 'z' && sdl_zoom_trig==1)//if ==2 then it was toggled with alt+z, don't turn off on keyup
				sdl_zoom_trig = 0;
		}
#ifdef INTERNAL
		int counterthing;
		if (sdl_key=='v'&&!(sdl_mod & (KMOD_LCTRL|KMOD_RCTRL)))//frame capture
		{
			if (sdl_mod & (KMOD_SHIFT)) {
				if (vs>=1)
					vs = 0;
				else
					vs = 3;//every other frame
			}
			else
			{
				if (vs>=1)
					vs = 0;
				else
					vs = 1;
			}
			counterthing = 0;
		}
		if (vs)
		{
			if (counterthing+1>=vs)
			{
				dump_frame(vid_buf, XRES, YRES, XRES+BARSIZE);
				counterthing = 0;
			}
			counterthing = (counterthing+1)%3;
		}
#endif

		if (sdl_wheel)
		{
			if (sdl_zoom_trig)//zoom window change
			{
				ZSIZE += sdl_wheel;
				if (ZSIZE>60)
					ZSIZE = 60;
				if (ZSIZE<2)
					ZSIZE = 2;
				ZFACTOR = 256/ZSIZE;
				//sdl_wheel = 0;
			}
			else //change brush size
			{
				if (!(sdl_mod & (KMOD_SHIFT|KMOD_CTRL)))
				{
					bsx += sdl_wheel;
					bsy += sdl_wheel;
				}
				else if (sdl_mod & (KMOD_SHIFT) && !(sdl_mod & (KMOD_CTRL)))
				{
					bsx += sdl_wheel;
				}
				else if (sdl_mod & (KMOD_CTRL) && !(sdl_mod & (KMOD_SHIFT)))
				{
					bsy += sdl_wheel;
				}
				if (bsx>1180)
					bsx = 1180;
				if (bsx<0)
					bsx = 0;
				if (bsy>1180)
					bsy = 1180;
				if (bsy<0)
					bsy = 0;
				//sdl_wheel = 0;
				/*if(su >= PT_NUM) {
					if(sl < PT_NUM)
						su = sl;
					if(sr < PT_NUM)
						su = sr;
				}*/
			}
		}

		bq = bc; // bq is previous mouse state
		bc = b = mouse_get_state(&x, &y); // b is current mouse state

#ifdef LUACONSOLE
		if(bc && bq){
			if(!luacon_mouseevent(x, y, bc, LUACON_MPRESS, sdl_wheel)){
				b = 0;
			}
		}
		else if(bc && !bq){
			if(!luacon_mouseevent(x, y, bc, LUACON_MDOWN, sdl_wheel)){
				b = 0;
			}
		}
		else if(!bc && bq){
			if(!luacon_mouseevent(x, y, bq, LUACON_MUP, sdl_wheel)){
				b = 0;
			}
		}
		else if (sdl_wheel){
			luacon_mouseevent(x, y, bq, 0, sdl_wheel);
		}
		
		luacon_step(x, y,sl,sr,bsx,bsy);
#endif
		sdl_wheel = 0;
		//quickoptions_menu(vid_buf, b, bq, x, y);

		for (i=0; i<SC_TOTAL; i++)//draw all the menu sections
		{
			draw_menu(vid_buf, i, active_menu);
		}

		for (i=0; i<SC_TOTAL; i++)//check mouse position to see if it is on a menu section
		{
			if (/*!b&&*/x>=(XRES-2) && x<(XRES+BARSIZE-1) &&y>= ((i*16)+YRES+MENUSIZE-16-(SC_TOTAL*16)) && y<((i*16)+YRES+MENUSIZE-16-(SC_TOTAL*16)+15))
			{
				active_menu = i;
			}
		}
		menu_ui_v3(vid_buf, active_menu, &sl, &su, x, y); //draw the elements in the current menu
		mouse_coords_window_to_sim(&x, &y, x, y);//change mouse position while it is in a zoom window
		if (y>=0 && y<YRES && x>=0 && x<XRES)
		{
			int cr; //cr is particle under mouse, for drawing HUD information
			char nametext[50];
			if (photons[y][x]) {
				cr = photons[y][x];
			} else {
				cr = pmap[y][x];
			}
			if (cr)
			{
				if ((cr&0xFF)==PT_LIFE && parts[cr>>8].ctype>=0 && parts[cr>>8].ctype<NGOLALT)
				{
					sprintf(nametext, "%s (%s)", ptypes[cr&0xFF].name, gmenu[parts[cr>>8].ctype].name);
				}
				else if ((cr&0xFF)==PT_LAVA && parts[cr>>8].ctype > 0 && parts[cr>>8].ctype < PT_NUM )
				{
					char lowername[6];
					int ix;
					strcpy(lowername, ptypes[parts[cr>>8].ctype].name);
					for (ix = 0; lowername[ix]; ix++)
						lowername[ix] = tolower(lowername[ix]);

					sprintf(nametext, "Molten %s", lowername);
				}
				else if (((cr&0xFF)==PT_PIPE || (cr&0xFF) == PT_PPIP) && (parts[cr>>8].tmp&0xFF) > 0 && (parts[cr>>8].tmp&0xFF) < PT_NUM )
				{
					char lowername[6];
					int ix;
					strcpy(lowername, ptypes[parts[cr>>8].tmp&0xFF].name);
					for (ix = 0; lowername[ix]; ix++)
						lowername[ix] = tolower(lowername[ix]);

					sprintf(nametext, "Pipe with %s", lowername);
				}
				else if (DEBUG_MODE)
				{
					int tctype = parts[cr>>8].ctype;
					if ((cr&0xFF)==PT_PIPE || (cr&0xFF) == PT_PPIP)
					{
						tctype = parts[cr>>8].tmp&0xFF;
					}
					if (tctype>=PT_NUM || tctype<0 || (cr&0xFF)==PT_PHOT)
						tctype = 0;
					sprintf(nametext, "%s (%s)", ptypes[cr&0xFF].name, ptypes[tctype].name);
				}
				else
				{
					strcpy(nametext, ptypes[cr&0xFF].name);
				}
				//if ((cr&0xFF)==PT_PHOT) wavelength_gfx = parts[cr>>8].ctype;
			}
            
            char pressur[64];
            sprintf(pressur,"%3.2f",pv[y/CELL][x/CELL]);
            if(strcmp(pressur,"-0.00")==0) sprintf(pressur,"0.00");  //workaround
            sprintf(heattext, "X: %d, Y: %d, Pressure: %s", x, y, pressur);
		}


		mx = x;
		my = y;

		if (y>=(YRES+(MENUSIZE-20))) //mouse checks for buttons at the bottom, to draw mouseover texts
		{
			if (x>=189 && x<=202 && svf_login && svf_open && svf_myvote==0)
			{
				db = svf_own ? 275 : 272;
				if (da < 51)
					da ++;
			}
			else if (x>=204 && x<=217 && svf_login && svf_open && svf_myvote==0)
			{
				db = svf_own ? 275 : 272;
				if (da < 51)
					da ++;
			}
			else if (x>=189 && x<=217 && svf_login && svf_open && svf_myvote!=0)
			{
				db = (svf_myvote==1) ? 273 : 274;
				if (da < 51)
					da ++;
			}
			else if (x>=219 && x<=((XRES+BARSIZE-(510-349))) && svf_login && svf_open)
			{
				db = svf_own ? 257 : 256;
				if (da < 51)
					da ++;
			}
			else if (x>=((XRES+BARSIZE-(510-351))) && x<((XRES+BARSIZE-(510-366))))
			{
				db = 270;
				if (da < 51)
					da ++;
			}
			else if (x>=((XRES+BARSIZE-(510-367))) && x<((XRES+BARSIZE-(510-383))))
			{
				db = 266;
				if (da < 51)
					da ++;
			}
			else if (x>=37 && x<=187)
			{
				if(sdl_mod & (KMOD_LCTRL|KMOD_RCTRL))
				{
					db = 277;
					if (da < 51)
						da ++;
				}
				else if(svf_login)
				{
					db = 259;
					if (svf_open && svf_own && x<=55)
						db = 258;
					if (da < 51)
						da ++;
				}
			}
			else if (x>=((XRES+BARSIZE-(510-385))) && x<=((XRES+BARSIZE-(510-476))))
			{
				db = svf_login ? 261 : 260;
				if (svf_admin)
				{
					db = 268;
				}
				else if (svf_mod)
				{
					db = 271;
				}
				if (da < 51)
					da ++;
			}
			else if (x>=1 && x<=17)
			{
				if(sdl_mod & (KMOD_LCTRL|KMOD_RCTRL))
					db = 276;
				else
					db = 262;
				if (da < 51)
					da ++;
			}
			else if (x>=((XRES+BARSIZE-(510-494))) && x<=((XRES+BARSIZE-(510-509))))
			{
				db = sys_pause ? 264 : 263;
				if (da < 51)
					da ++;
			}
			else if (x>=((XRES+BARSIZE-(510-476))) && x<=((XRES+BARSIZE-(510-491))))
			{
				db = 267;
				if (da < 51)
					da ++;
			}
			else if (x>=19 && x<=35 && svf_open)
			{
				db = 265;
				if (da < 51)
					da ++;
			}
			else if (da > 0)
				da --;
		}
		else if (da > 0)//fade away mouseover text
			da --;

		if (dae > 0) //Fade away selected elements
			dae --;
		
		if (!sdl_zoom_trig && zoom_en==1)
			zoom_en = 0;

		if (sdl_key=='z' && !(sdl_mod & (KMOD_LCTRL|KMOD_RCTRL)) && zoom_en==2 && sys_shortcuts==1)
			zoom_en = 1;

		if (load_mode)
		{
			load_x = CELL*((mx-load_w/2+CELL/2)/CELL);
			load_y = CELL*((my-load_h/2+CELL/2)/CELL);
			if (load_x+load_w>XRES) load_x=XRES-load_w;
			if (load_y+load_h>YRES) load_y=YRES-load_h;
			if (load_x<0) load_x=0;
			if (load_y<0) load_y=0;
			if (bq==1 && !b)
			{
				parse_save(load_data, load_size, 0, load_x, load_y, bmap, vx, vy, pv, fvx, fvy, signs, parts, pmap);
				free(load_data);
				free(load_img);
				load_mode = 0;
			}
			else if (bq==4 && !b)
			{
				free(load_data);
				free(load_img);
				load_mode = 0;
			}
		}
		else if (save_mode==1)//getting the area you are selecting
		{
			save_x = mx;
			save_y = my;
			if (save_x >= XRES) save_x = XRES-1;
			if (save_y >= YRES) save_y = YRES-1;
			save_w = 1;
			save_h = 1;
			if (b==1)
			{
				save_mode = 2;
			}
			else if (b==4)
			{
				save_mode = 0;
				copy_mode = 0;
			}
		}
		else if (save_mode==2)
		{
			save_w = mx + 1 - save_x;
			save_h = my + 1 - save_y;
			if (save_w+save_x>XRES) save_w = XRES-save_x;
			if (save_h+save_y>YRES) save_h = YRES-save_y;
			if (save_w+save_x<0) save_w = 0;
			if (save_h+save_y<0) save_h = 0;
			//if (save_w<1) save_w = 1;
			//if (save_h<1) save_h = 1;
			if (!b)
			{
				if (save_w < 0)
				{
					save_x = save_x + save_w - 1;
					save_w = abs(save_w) + 2;
				}
				if (save_h < 0)
				{
					save_y = save_y + save_h - 1;
					save_h = abs(save_h) + 2;
				}
				if (save_h > 0 && save_w > 0)
				{
					if (copy_mode==1)//CTRL-C, copy
					{
						clipboard_data=build_save(&clipboard_length, save_x, save_y, save_w, save_h, bmap, vx, vy, pv, fvx, fvy, signs, parts);
						if (clipboard_data)
							clipboard_ready = 1;
					}
					else if (copy_mode==2)//CTRL-X, cut
					{
						clipboard_data=build_save(&clipboard_length, save_x, save_y, save_w, save_h, bmap, vx, vy, pv, fvx, fvy, signs, parts);
						if (clipboard_data)
						{
							clipboard_ready = 1;
							clear_area(save_x, save_y, save_w, save_h);
						}
					}
					else//normal save
					{
						stamp_save(save_x, save_y, save_w, save_h);
					}
				}
				copy_mode = 0;
				save_mode = 0;
			}
		}
		else if (sdl_zoom_trig && zoom_en<2)
		{
			x -= ZSIZE/2;
			y -= ZSIZE/2;
			if (x<0) x=0;
			if (y<0) y=0;
			if (x>XRES-ZSIZE) x=XRES-ZSIZE;
			if (y>YRES-ZSIZE) y=YRES-ZSIZE;
			zoom_x = x;
			zoom_y = y;
			zoom_wx = (x<XRES/2) ? XRES-ZSIZE*ZFACTOR : 0;
			zoom_wy = 0;
			zoom_en = 1;
			if (!b && bq)
			{
				zoom_en = 2;
				sdl_zoom_trig = 0;
			}
		}
		else if (b)//there is a click
		{
            //printf("x:%d y:%d\n",x,y);
            
            if (y>=YRES+(MENUSIZE-20))//check if mouse is on menu buttons
			{
				if (!lb)//mouse is NOT held down, so it is a first click
				{
                    if (x>=(XRES+BARSIZE-(510-494)) && x<=(XRES+BARSIZE-(510-509)) && !bq) {
                        //sdl_zoom_trig = 1;
                        int pausa=sys_pause;
                        sys_pause=1;
                        DoActionSheet(window,pausa);
                    }
					lb = 0;
                }
            }
     
//to mamy w dupie calkowicie
#if 0
			if (y>=YRES+(MENUSIZE-20))//check if mouse is on menu buttons
			{
				if (!lb)//mouse is NOT held down, so it is a first click
				{
                    if (x>=(XRES+BARSIZE-(510-494)) && x<=(XRES+BARSIZE-(510-509)) && !bq)
						sys_pause = !sys_pause;
					lb = 0;
#if 0
					/*if (x>=189 && x<=202 && svf_login && svf_open && svf_myvote==0 && svf_own==0)
					{
						if (execute_vote(vid_buf, svf_id, "Up"))
						{
							svf_myvote = 1;
						}
					}
					if (x>=204 && x<=217 && svf_login && svf_open && svf_myvote==0 && svf_own==0)
					{
						if (execute_vote(vid_buf, svf_id, "Down"))
						{
							svf_myvote = -1;
						}
					}
					if (x>=219 && x<=(XRES+BARSIZE-(510-349)) && svf_login && svf_open)
						tag_list_ui(vid_buf);*/
					if (x>=(XRES+BARSIZE-(510-444)) && x<(XRES+BARSIZE-(510-460)) && !bq)
					{
						//legacy_enable = !legacy_enable;
						simulation_ui(vid_buf);
					}
					if (x>=(XRES+BARSIZE-(510-460)) && x<=(XRES+BARSIZE-(510-476)) && !bq)
					{
						clear_sim();
						for (i=0; i<NPART-1; i++)
							parts[i].life = i+1;
						parts[NPART-1].life = -1;
						pfree = 0;

						legacy_enable = 0;
						svf_filename[0] = 0;
						svf_fileopen = 0;
						svf_myvote = 0;
						svf_open = 0;
						svf_publish = 0;
						svf_own = 0;
						svf_id[0] = 0;
						svf_name[0] = 0;
						svf_tags[0] = 0;
						svf_description[0] = 0;
						gravityMode = 0;
						airMode = 0;
						svf_last = NULL;
						svf_lsize = 0;
					}
					//if(sdl_mod & (KMOD_LCTRL|KMOD_RCTRL))
					//{
						if (x>=19 && x<=169)
						{
							save_filename_ui(vid_buf);
									
						}
						if (x>=1 && x<=17)
						{
							catalogue_ui(vid_buf);
						}
					/*} else {
						if (x>=37 && x<=187 && svf_login)
						{
							if (!svf_open || !svf_own || x>51)
							{
								if (save_name_ui(vid_buf)) {
									execute_save(vid_buf);
									if (svf_id[0]) {
										copytext_ui(vid_buf, "Save ID", "Saved successfully!", svf_id);
									}
								}
							}
							else
								execute_save(vid_buf);
							while (!sdl_poll())
								if (!mouse_get_state(&x, &y))
									break;
							b = bq = 0;
						}
						if (x>=1 && x<=17)
						{
							search_ui(vid_buf);
							memset(pers_bg, 0, (XRES+BARSIZE)*YRES*PIXELSIZE);
							memset(fire_r, 0, sizeof(fire_r));
							memset(fire_g, 0, sizeof(fire_g));
							memset(fire_b, 0, sizeof(fire_b));
						}
					}*/
					/*if (x>=19 && x<=35 && svf_last && !bq) {
						//int tpval = sys_pause;
						if (b == 1 || !strncmp(svf_id,"",8))
							parse_save(svf_last, svf_lsize, 1, 0, 0, bmap, vx, vy, pv, fvx, fvy, signs, parts, pmap);
						else
							open_ui(vid_buf, svf_id, NULL);
						//sys_pause = tpval;
					}*/
					if (x>=(XRES+BARSIZE-(510-476)) && x<=(XRES+BARSIZE-(510-491)) && !bq)
					{
						render_ui(vid_buf, XRES+BARSIZE-(510-491)+1, YRES+22, 3);
					}
					if (x>=(XRES+BARSIZE-(510-494)) && x<=(XRES+BARSIZE-(510-509)) && !bq)
						sys_pause = !sys_pause;
					lb = 0;
                    
                    //koniec sosu
#endif
				}
			}
#endif
            
            
            
            
			else if (y<YRES-12 && x<XRES)// mouse is in playing field
			{
				int signi;

				c = (b&1) ? sl : sr; //c is element to be spawned
				su = c;

				if (c==WL_SIGN+100 || MSIGN!=-1) // if sign tool is selected or a sign is being moved
				{
					if (!bq)
						add_sign_ui(vid_buf, x, y);
				}
				else if (c==PT_FIGH)
				{
					if (!bq)
						create_part(-1, x, y, PT_FIGH);
				}
				//for the click functions, lx and ly, are the positions of where the FIRST click happened.  x,y are current mouse position.
				else if (lb)//lb means you are holding mouse down
				{
					if (lm == 1)//line tool
					{
						if (sdl_mod & KMOD_ALT)
						{
							float snap_angle = floor(atan2(y-ly, x-lx)/(M_PI*0.25)+0.5)*M_PI*0.25;
							float line_mag = sqrtf(pow(x-lx,2)+pow(y-ly,2));
							line_x = (int)(line_mag*cos(snap_angle)+lx+0.5f);
							line_y = (int)(line_mag*sin(snap_angle)+ly+0.5f);
						}
						else
						{
							line_x = x;
							line_y = y;
						}
						xor_line(lx, ly, line_x, line_y, vid_buf);
						if (c==WL_FAN+100 && lx>=0 && ly>=0 && lx<XRES && ly<YRES && bmap[ly/CELL][lx/CELL]==WL_FAN)
						{
							nfvx = (line_x-lx)*0.005f;
							nfvy = (line_y-ly)*0.005f;
							flood_parts(lx, ly, WL_FANHELPER, -1, WL_FAN, 0);
							for (j=0; j<YRES/CELL; j++)
								for (i=0; i<XRES/CELL; i++)
									if (bmap[j][i] == WL_FANHELPER)
									{
										fvx[j][i] = nfvx;
										fvy[j][i] = nfvy;
										bmap[j][i] = WL_FAN;
									}
						}
						if (c == SPC_WIND)
						{
							for (j=-bsy; j<=bsy; j++)
								for (i=-bsx; i<=bsx; i++)
									if (lx+i>0 && ly+j>0 && lx+i<XRES && ly+j<YRES && InCurrentBrush(i,j,bsx,bsy))
									{
										vx[(ly+j)/CELL][(lx+i)/CELL] += (line_x-lx)*0.002f;
										vy[(ly+j)/CELL][(lx+i)/CELL] += (line_y-ly)*0.002f;
									}
						}
					}
					else if (lm == 2)//box tool
					{
						xor_line(lx, ly, lx, y, vid_buf);
						xor_line(lx, y, x, y, vid_buf);
						xor_line(x, y, x, ly, vid_buf);
						xor_line(x, ly, lx, ly, vid_buf);
					}
					else//while mouse is held down, it draws lines between previous and current positions
					{
						if (c == SPC_WIND)
						{
							for (j=-bsy; j<=bsy; j++)
								for (i=-bsx; i<=bsx; i++)
									if (x+i>0 && y+j>0 && x+i<XRES && y+j<YRES && InCurrentBrush(i,j,bsx,bsy))
									{
										vx[(y+j)/CELL][(x+i)/CELL] += (x-lx)*0.01f;
										vy[(y+j)/CELL][(x+i)/CELL] += (y-ly)*0.01f;
									}
						}
						else
						{
							create_line(lx, ly, x, y, bsx, bsy, c, get_brush_flags());
						}
						lx = x;
						ly = y;
					}
				}
				else //it is the first click
				{
					//start line tool
					if ((sdl_mod & (KMOD_SHIFT)) && !(sdl_mod & (KMOD_CTRL)))
					{
						lx = x;
						ly = y;
						lb = b;
						lm = 1;//line
					}
					//start box tool
					else if ((sdl_mod & (KMOD_CTRL)) && !(sdl_mod & (KMOD_SHIFT|KMOD_ALT)))
					{
						lx = x;
						ly = y;
						lb = b;
						lm = 2;//box
					}
					//flood fill
					else if ((sdl_mod & (KMOD_CTRL)) && (sdl_mod & (KMOD_SHIFT)) && !(sdl_mod & (KMOD_ALT)))
					{
						if (sdl_mod & (KMOD_CAPS))
							c = 0;
						if (c!=WL_STREAM+100&&c!=SPC_AIR&&c!=SPC_HEAT&&c!=SPC_COOL&&c!=SPC_VACUUM&&!REPLACE_MODE&&c!=SPC_WIND&&c!=SPC_PGRV&&c!=SPC_NGRV)
							flood_parts(x, y, c, -1, -1, get_brush_flags());
						if (c==SPC_HEAT || c==SPC_COOL)
							create_parts(x, y, bsx, bsy, c, get_brush_flags(), 1);
						lx = x;
						ly = y;
						lb = 0;
						lm = 0;
					}
					//sample
					else if (((sdl_mod & (KMOD_ALT)) && !(sdl_mod & (KMOD_SHIFT|KMOD_CTRL))) || b==SDL_BUTTON_MIDDLE)
					{
						if (y>=0 && y<YRES && x>=0 && x<XRES)
						{
							int cr;
							cr = pmap[y][x];
							if (!cr)
								cr = photons[y][x];
							if (cr)
							{
								c = sl = cr&0xFF;
								if (c==PT_LIFE)
									c = sl = (parts[cr>>8].ctype << 8) | c;
							}
							else
							{
								//Something
							}
						}
						lx = x;
						ly = y;
						lb = 0;
						lm = 0;
					}
					else //normal click, spawn element
					{
						//Copy state before drawing any particles (for undo)7
						int cbx, cby, cbi;

						for (cbi=0; cbi<NPART; cbi++)
							cb_parts[cbi] = parts[cbi];

						for (cby = 0; cby<YRES; cby++)
							for (cbx = 0; cbx<XRES; cbx++)
								cb_pmap[cby][cbx] = pmap[cby][cbx];

						for (cby = 0; cby<(YRES/CELL); cby++)
							for (cbx = 0; cbx<(XRES/CELL); cbx++)
							{
								cb_vx[cby][cbx] = vx[cby][cbx];
								cb_vy[cby][cbx] = vy[cby][cbx];
								cb_pv[cby][cbx] = pv[cby][cbx];
								cb_hv[cby][cbx] = hv[cby][cbx];
								cb_bmap[cby][cbx] = bmap[cby][cbx];
								cb_emap[cby][cbx] = emap[cby][cbx];
							}
						create_parts(x, y, bsx, bsy, c, get_brush_flags(), 1);
						lx = x;
						ly = y;
						lb = b;
						lm = 0;
					}
				}
			}
		}
		else
		{
			if (lb && lm) //lm is box/line tool
			{
				c = (lb&1) ? sl : sr;
				su = c;
				if (lm == 1)//line
				{
					if (c!=WL_FAN+100 || lx<0 || ly<0 || lx>=XRES || ly>=YRES || bmap[ly/CELL][lx/CELL]!=WL_FAN)
						create_line(lx, ly, line_x, line_y, bsx, bsy, c, get_brush_flags());
				}
				else//box
					create_box(lx, ly, x, y, c, get_brush_flags());
				lm = 0;
			}
			lb = 0;
		}

		if (load_mode)//draw preview of stamp
		{
			draw_image(vid_buf, load_img, load_x, load_y, load_w, load_h, 128);
			xor_rect(vid_buf, load_x, load_y, load_w, load_h);
		}

		if (save_mode)//draw dotted lines for selection
		{
			int savex = save_x, savey = save_y, savew = save_w, saveh = save_h;
			if (savew < 0)
			{
				savex = savex + savew - 1;
				savew = abs(savew) + 2;
			}
			if (saveh < 0)
			{
				savey = savey + saveh - 1;
				saveh = abs(saveh) + 2;
			}
			xor_rect(vid_buf, savex, savey, savew, saveh);
			da = 51;//draws mouseover text for the message
			if (copy_mode != 2)
				db = 269;//the save message
			else
				db = 278;
		}
        
		if (zoom_en!=1 && !load_mode && !save_mode)//draw normal cursor
		{
			render_cursor(vid_buf, bsx, bsy);
			mousex = mx;
			mousey = my;
		}
#ifdef OGLR
		draw_parts_fbo();
#endif		
		if (zoom_en)
			render_zoom(vid_buf);

		if (da)
			/*switch (db)//various mouseover messages, da is the alpha
			{
			case 256:
				drawtext(vid_buf, 16, YRES-24, "Add simulation tags.", 255, 255, 255, da*5);
				break;
			case 257:
				drawtext(vid_buf, 16, YRES-24, "Add and remove simulation tags.", 255, 255, 255, da*5);
				break;
			case 258:
				drawtext(vid_buf, 16, YRES-24, "Save the simulation under the current name.", 255, 255, 255, da*5);
				break;
			case 259:
				drawtext(vid_buf, 16, YRES-24, "Save the simulation under a new name.", 255, 255, 255, da*5);
				break;
			case 260:
				drawtext(vid_buf, 16, YRES-24, "Sign into the Simulation Server.", 255, 255, 255, da*5);
				break;
			case 261:
				drawtext(vid_buf, 16, YRES-24, "Sign into the Simulation Server under a new name.", 255, 255, 255, da*5);
				break;
			case 262:
				drawtext(vid_buf, 16, YRES-24, "Find & open a simulation", 255, 255, 255, da*5);
				break;
			case 263:
				drawtext(vid_buf, 16, YRES-24, "Pause the simulation", 255, 255, 255, da*5);
				break;
			case 264:
				drawtext(vid_buf, 16, YRES-24, "Resume the simulation", 255, 255, 255, da*5);
				break;
			case 265:
				drawtext(vid_buf, 16, YRES-24, "Reload the simulation", 255, 255, 255, da*5);
				break;
			case 266:
				drawtext(vid_buf, 16, YRES-24, "Erase all particles and walls", 255, 255, 255, da*5);
				break;
			case 267:
				drawtext(vid_buf, 16, YRES-24, "Change display mode", 255, 255, 255, da*5);
				break;
			case 268:
				drawtext(vid_buf, 16, YRES-24, "Annuit C\245ptis", 255, 255, 255, da*5);
				break;
			case 269:
				drawtext(vid_buf, 16, YRES-24, "Click-and-drag to specify a rectangle to copy (right click = cancel).", 255, 216, 32, da*5);
				break;
			case 270:
				drawtext(vid_buf, 16, YRES-24, "Simulation options", 255, 255, 255, da*5);
				break;
			case 271:
				drawtext(vid_buf, 16, YRES-24, "You're a moderator", 255, 255, 255, da*5);
				break;
			case 272:
				drawtext(vid_buf, 16, YRES-24, "Like/Dislike this save.", 255, 255, 255, da*5);
				break;
			case 273:
				drawtext(vid_buf, 16, YRES-24, "You like this.", 255, 255, 255, da*5);
				break;
			case 274:
				drawtext(vid_buf, 16, YRES-24, "You dislike this.", 255, 255, 255, da*5);
				break;
			case 275:
				drawtext(vid_buf, 16, YRES-24, "You cannot vote on your own save.", 255, 255, 255, da*5);
				break;
			case 276:
				drawtext(vid_buf, 16, YRES-24, "Open a simulation from your hard drive.", 255, 255, 255, da*5);
				break;
			case 277:
				drawtext(vid_buf, 16, YRES-24, "Save the simulation to your hard drive.", 255, 255, 255, da*5);
				break;
			case 278: //Fix for Ctrl + X showing copy message
				drawtext(vid_buf, 16, YRES-24, "Click-and-drag to specify a rectangle to copy and then cut (right click = cancel).", 255, 216, 32, da*5);
				break;
			default:
				drawtext(vid_buf, 16, YRES-24, (char *)ptypes[db].descs, 255, 255, 255, da*5);
			}*/
		if (itc)//message in the middle of the screen, such as view mode changes
		{
			itc--;
			drawtext_outline(vid_buf, (XRES-textwidth(itc_msg))/2, ((YRES/2)-10), itc_msg, 255, 255, 255, itc>51?255:itc*5, 0, 0, 0, itc>51?255:itc*5);
		}

		/*if (old_version)
		{
			clearrect(vid_buf, XRES-21-old_ver_len, YRES-24, old_ver_len+9, 17);
			if (is_beta)
			{
				drawtext(vid_buf, XRES-16-old_ver_len, YRES-19, old_ver_msg_beta, 255, 216, 32, 255);
			}
			else
			{
				drawtext(vid_buf, XRES-16-old_ver_len, YRES-19, old_ver_msg, 255, 216, 32, 255);
			}
			drawrect(vid_buf, XRES-19-old_ver_len, YRES-22, old_ver_len+5, 13, 255, 216, 32, 255);
		}*/

		if (svf_messages)
		{
			sprintf(new_message_msg, "You have %d new message%s, Click to view", svf_messages, (svf_messages>1)?"s":"");
			new_message_len = textwidth(new_message_msg);
			
			clearrect(vid_buf, XRES-21-new_message_len, YRES-39, new_message_len+9, 17);
			drawtext(vid_buf, XRES-16-new_message_len, YRES-34, new_message_msg, 255, 186, 32, 255);
			drawrect(vid_buf, XRES-19-new_message_len, YRES-37, new_message_len+5, 13, 255, 186, 32, 255);
		}

		FPS++;
		currentTime = SDL_GetTicks();
		elapsedTime = currentTime-pastFPS;
		if ((FPS>2 || elapsedTime>1000*2/limitFPS) && elapsedTime && FPS*1000/elapsedTime>limitFPS)
		{
			while (FPS*1000/elapsedTime>limitFPS)
			{
				SDL_Delay(1);
				currentTime = SDL_GetTicks();
				elapsedTime = currentTime-pastFPS;
			}
		}
		if (elapsedTime>=1000)
		{
			if(FPS>limitFPS) FPSB = limitFPS;
			else FPSB = FPS;
			FPS = 0;
			pastFPS = currentTime;
		}

		if (hud_enable)
		{
            sprintf(uitext, "Performance: %d%%, Particles: %d", (int)((float)FPSB/limitFPS*100.0f), NUM_PARTS);
            //printf("FPS:%d\n",FPSB);
			if (REPLACE_MODE)
				strappend(uitext, " [REPLACE MODE]");
			if (sdl_mod&(KMOD_CAPS))
				strappend(uitext, " [CAPS LOCK]");
			if (GRID_MODE)
			{
				char gridtext[15];
				sprintf(gridtext, " [GRID: %d]", GRID_MODE);
				strappend(uitext, gridtext);
			}
#ifdef INTERNAL
			if (vs)
				strappend(uitext, " [FRAME CAPTURE]");
#endif
			//quickoptions_tooltip_fade_invert = 255 - (quickoptions_tooltip_fade*20);
#if 0
//do poprawienia pozycji tekstu
			if (sdl_zoom_trig||zoom_en)
			{
				if (zoom_x<XRES/2)
				{
					fillrect(vid_buf, XRES-20-textwidth(heattext), 266, textwidth(heattext)+8, 15, 0, 0, 0, 128);
					drawtext(vid_buf, XRES-16-textwidth(heattext), 270, heattext, 255, 255, 255, 190);
					if (DEBUG_MODE)
					{
						fillrect(vid_buf, XRES-20-textwidth(coordtext), 280, textwidth(coordtext)+8, 13, 0, 0, 0, 128);
						drawtext(vid_buf, XRES-16-textwidth(coordtext), 282, coordtext, 255, 255, 255, 190);
					}
					/*if (wavelength_gfx)
						draw_wavelengths(vid_buf,XRES-20-textwidth(heattext),265,2,wavelength_gfx);*/
				}
				else
				{
					fillrect(vid_buf, 12, 266, textwidth(heattext)+8, 15, 0, 0, 0, 128);
					drawtext(vid_buf, 16, 270, heattext, 255, 255, 255, 190);
					if (DEBUG_MODE)
					{
						fillrect(vid_buf, 12, 280, textwidth(coordtext)+8, 13, 0, 0, 0, 128);
						drawtext(vid_buf, 16, 282, coordtext, 255, 255, 255, 190);
					}
					/*if (wavelength_gfx)
						draw_wavelengths(vid_buf,12,265,2,wavelength_gfx);*/
				}
			}
			else
#endif
			{
				fillrect(vid_buf, XRES-textwidth(heattext), 8, textwidth(heattext)+8, 15, 0, 0, 0, 128);
				drawtext(vid_buf, XRES+5-textwidth(heattext), 12, heattext, 255, 255, 255, 190);
                
                //lulz
				if (DEBUG_MODE)
				{
					fillrect(vid_buf, XRES-20-textwidth(coordtext), 26, textwidth(coordtext)+8, 11, 0, 0, 0, 128);
					drawtext(vid_buf, XRES-16-textwidth(coordtext), 27, coordtext, 255, 255, 255, 190);
				}
				/*if (wavelength_gfx)
					draw_wavelengths(vid_buf,XRES-20-textwidth(heattext),11,2,wavelength_gfx);*/
			}
			//wavelength_gfx = 0;
			fillrect(vid_buf, 8, 8, textwidth(uitext)+8, 15, 0, 0, 0, 128);
			drawtext(vid_buf, 12, 12, uitext, 255, 255, 255, 190);
		}

		if (console_mode)
		{
#ifdef LUACONSOLE
			char *console;
			sys_pause = 1;
			console = console_ui(vid_buf, console_error, console_more);
			console = mystrdup(console);
			strcpy(console_error,"");
			if (process_command_lua(vid_buf, console, console_error)==-1)
			{
				free(console);
				break;
			}
			free(console);
#else
			char *console;
			sys_pause = 1;
			console = console_ui(vid_buf, console_error, console_more);
			console = mystrdup(console);
			strcpy(console_error,"");
			if (process_command_old(vid_buf, console, console_error)==-1)
			{
				free(console);
				break;
			}
			free(console);
#endif
		}

		sdl_blit(0, 0, XRES+BARSIZE, YRES+MENUSIZE, vid_buf, XRES+BARSIZE);

		//Setting an element for the stick man
		if (player.spwn==0)
		{
			if ((sr>0 && sr<PT_NUM && ptypes[sr].enabled && ptypes[sr].falldown>0) || sr==SPC_AIR || sr == PT_NEUT || sr == PT_PHOT || sr == PT_LIGH)
				player.elem = sr;
			else
				player.elem = PT_DUST;
		}
		if (player2.spwn==0)
		{
			if ((sr>0 && sr<PT_NUM && ptypes[sr].enabled && ptypes[sr].falldown>0) || sr==SPC_AIR || sr == PT_NEUT || sr == PT_PHOT || sr == PT_LIGH)
				player2.elem = sr;
			else
				player2.elem = PT_DUST;
		}
	}
	//save_presets(0);

	gravity_cleanup();
#ifdef LUACONSOLE
	luacon_close();
#endif
	return 0;
}
