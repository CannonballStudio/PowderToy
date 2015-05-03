/**
 * Powder Toy - user interface (header)
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
#ifndef INTERFACE_H
#define INTERFACE_H
#include "SDL/SDL.h"
#include "graphics.h"

struct menu_section
{
	char *icon;
	const char *name;
	int itemcount;
	int doshow;
};
typedef struct menu_section menu_section;

#define QM_TOGGLE	1

struct quick_option
{
	char *icon;
	const char *name;
	int type;
	int *variable;
};
typedef struct quick_option quick_option;

struct menu_wall
{
	pixel colour;
	const char *descs;
};
typedef struct menu_wall menu_wall;

#define SC_WALL 0
#define SC_ELEC1 1
#define SC_ELEC2 2
#define SC_POWERED 3
#define SC_FORCE 4
#define SC_EXPLOSIVE1 5
#define SC_EXPLOSIVE2 6
#define SC_GAS 7
#define SC_LIQUID1 8
#define SC_LIQUID2 9
#define SC_POWDERS1 10
#define SC_POWDERS2 11
#define SC_SOLIDS1 12
#define SC_SOLIDS2 13
#define SC_NUCLEAR 14
#define SC_SPECIAL 15
#define SC_LIFE1 16
#define SC_LIFE2 17
#define SC_TOOL 18

#define SC_CRACKER 19
#define SC_CRACKER2 20
#define SC_HIDDEN 21
#define SC_TOTAL 19

static menu_section msections[] = //doshow does not do anything currently.
{
	{"\xC1", "Walls", 0, 1},
	{"\xC2", "Electronics 1", 0, 1},
	{"\xC2", "Electronics 2", 0, 1},
	{"\xD6", "Powered Materials", 0, 1},
	{"\xE2", "Force Creating", 0, 1},
	{"\xC3", "Explosives 1", 0, 1},
	{"\xC3", "Explosives 2", 0, 1},
	{"\xC5", "Gases", 0, 1},
	{"\xC4", "Liquids 1", 0, 1},
	{"\xC4", "Liquids 2", 0, 1},
	{"\xD0", "Powders 1", 0, 1},
	{"\xD0", "Powders 2", 0, 1},
	{"\xD1", "Solids 1", 0, 1},
	{"\xD1", "Solids 2", 0, 1},
	{"\xC6", "Radioactive", 0, 1},
	{"\xCC", "Special", 0, 1},
	{"\xD2", "Life 1", 0, 1},
	{"\xD2", "Life 2", 0, 1},
	{"\xD7", "Tools", 0, 1},
};

/*static quick_option quickmenu[] =
{
	{"P", "Sand effect", QM_TOGGLE, &pretty_powder},
	{"G", "Draw gravity grid", QM_TOGGLE, &drawgrav_enable},
	{"D", "Show decorations", QM_TOGGLE, &decorations_enable},
	{"N", "Newtonian gravity", QM_TOGGLE, &ngrav_enable},
	{"A", "Ambient heat", QM_TOGGLE, &aheat_enable},
	//{"C", "Show Console", QM_TOGGLE, &console_mode},
	{NULL}
};*/

static menu_section colorsections[] = //doshow does not do anything currently.
{
	{"\xC4", "Colors", 7, 1},
	{"\xD7", "Tools", 0, 1},
};
#define DECO_SECTIONS 2

static menu_wall colorlist[] =
{
	{PIXPACK(0xFF0000), "Red"},
	{PIXPACK(0x00FF00), "Green"},
	{PIXPACK(0x0000FF), "Blue"},
	{PIXPACK(0xFFFF00), "Yellow"},
	{PIXPACK(0xFF00FF), "Pink"},
	{PIXPACK(0x00FFFF), "Cyan"},
	{PIXPACK(0xFFFFFF), "White"},
};

#define DECO_DRAW 0
#define DECO_LIGHTEN 1
#define DECO_DARKEN 2
#define DECO_SMUDGE 3

static menu_wall toollist[] =
{
	{PIXPACK(0xFF0000), "Draw"},
	{PIXPACK(0xDDDDDD), "Lighten"},
	{PIXPACK(0x111111), "Darken"},
	{PIXPACK(0x00FF00), "Smudge"},
};

struct ui_edit
{
	int x, y, w, nx, h;
	char str[256],*def;
	int focus, cursor, hide, multiline;
};
typedef struct ui_edit ui_edit;

struct ui_list
{
	int x, y, w, h;
	char str[256],*def,**items;
	int selected, focus, count;
};
typedef struct ui_list ui_list;

struct ui_copytext
{
	int x, y, width, height;
	char text[256];
	int state, hover;
};
typedef struct ui_copytext ui_copytext;

struct save_info
{
	char *title;
	char *name;
	char *author;
	char *date;
	char *description;
	int publish;
	int voteup;
	int votedown;
	int vote;
	int myvote;
	int downloadcount;
	int myfav;
	char *tags;
	int comment_count;
	char *comments[6];
	char *commentauthors[6];
};
typedef struct save_info save_info;

struct ui_checkbox
{
	int x, y;
	int focus, checked;
};
typedef struct ui_checkbox ui_checkbox;

struct ui_richtext
{
	int x, y;
	char str[512];
	char printstr[512];
	int regionss[6];
	int regionsf[6];
	char action[6];
	char actiondata[6][256];
	char actiontext[6][256];
};
typedef struct ui_richtext ui_richtext;

int SLALT;
extern SDL_Keymod sdl_mod;
extern int sdl_key, sdl_rkey, sdl_wheel, sdl_ascii, sdl_zoom_trig;

extern char *shift_0;
extern char *shift_1;
extern int svf_messages;
extern int svf_login;
extern int svf_admin;
extern int svf_mod;
extern char svf_user[64];
extern char svf_pass[64];
extern char svf_user_id[64];
extern char svf_session_id[64];


extern char svf_filename[255];
extern int svf_fileopen;
extern int svf_open;
extern int svf_own;
extern int svf_myvote;
extern int svf_publish;
extern char svf_id[16];
extern char svf_name[64];
extern char svf_tags[256];
extern char svf_description[255];
extern void *svf_last;
extern int svf_lsize;

extern char *search_ids[GRID_X*GRID_Y];
extern char *search_dates[GRID_X*GRID_Y];
extern int   search_votes[GRID_X*GRID_Y];
extern int   search_publish[GRID_X*GRID_Y];
extern int	  search_scoredown[GRID_X*GRID_Y];
extern int	  search_scoreup[GRID_X*GRID_Y];
extern char *search_names[GRID_X*GRID_Y];
extern char *search_owners[GRID_X*GRID_Y];
extern void *search_thumbs[GRID_X*GRID_Y];
extern int   search_thsizes[GRID_X*GRID_Y];

extern int search_own;
extern int search_fav;
extern int search_date;
extern int search_page;
extern char search_expr[256];

extern char *tag_names[TAG_MAX];
extern int tag_votes[TAG_MAX];

extern int zoom_en;
extern int zoom_x, zoom_y;
extern int zoom_wx, zoom_wy;

void menu_count(void);

//void quickoptions_menu(pixel *vid_buf, int b, int bq, int x, int y);

void prop_edit_ui(pixel *vid_buf, int x, int y);

void get_sign_pos(int i, int *x0, int *y0, int *w, int *h);

void add_sign_ui(pixel *vid_buf, int mx, int my);

void ui_edit_draw(pixel *vid_buf, ui_edit *ed);

void ui_edit_process(int mx, int my, int mb, ui_edit *ed);

void ui_list_draw(pixel *vid_buf, ui_list *ed);

void ui_list_process(pixel * vid_buf, int mx, int my, int mb, ui_list *ed);

void ui_checkbox_draw(pixel *vid_buf, ui_checkbox *ed);

void ui_checkbox_process(int mx, int my, int mb, int mbq, ui_checkbox *ed);

void ui_copytext_draw(pixel *vid_buf, ui_copytext *ed);

void ui_copytext_process(int mx, int my, int mb, int mbq, ui_copytext *ed);

void ui_richtext_draw(pixel *vid_buf, ui_richtext *ed);

void ui_richtext_settext(char *text, ui_richtext *ed);

void ui_richtext_process(int mx, int my, int mb, int mbq, ui_richtext *ed);

void draw_svf_ui(pixel *vid_buf, int alternate);

void error_ui(pixel *vid_buf, int err, char *txt);

void element_search_ui(pixel *vid_buf, int * sl, int * sr);

void info_ui(pixel *vid_buf, char *top, char *txt);

void copytext_ui(pixel *vid_buf, char *top, char *txt, char *copytxt);

void info_box(pixel *vid_buf, char *msg);

void info_box_overlay(pixel *vid_buf, char *msg);

char *input_ui(pixel *vid_buf, char *title, char *prompt, char *text, char *shadow);

int confirm_ui(pixel *vid_buf, char *top, char *msg, char *btn);

void login_ui(pixel *vid_buf);

int stamp_ui(pixel *vid_buf);

void tag_list_ui(pixel *vid_buf);

int save_name_ui(pixel *vid_buf);

int save_filename_ui(pixel *vid_buf);

void menu_ui(pixel *vid_buf, int i, int *sl, int *sr);

void menu_ui_v3(pixel *vid_buf, int i, int *sl, int *su, int mx, int my);

int color_menu_ui(pixel *vid_buf, int i, int *cr, int *cg, int *cb, int b, int bq, int mx, int my, int * tool);

int sdl_poll(void);

void stickmen_keys();

void set_cmode(int cm);

void catalogue_ui(pixel * vid_buf);

int info_parse(char *info_data, save_info *info);

int search_results(char *str, int votes);

void execute_save(pixel *vid_buf);

int execute_delete(pixel *vid_buf, char *id);

int execute_report(pixel *vid_buf, char *id, char *reason);

void execute_submit(pixel *vid_buf, char *id, char *message);

void execute_fav(pixel *vid_buf, char *id);

void execute_unfav(pixel *vid_buf, char *id);

int execute_vote(pixel *vid_buf, char *id, char *action);

int report_ui(pixel *vid_buf, char *save_id);

char *console_ui(pixel *vid_buf, char error[255],char console_more);

void render_ui(pixel *vid_buf, int xcoord, int ycoord, int orientation);

void simulation_ui(pixel *vid_buf);

unsigned int decorations_ui(pixel *vid_buf, int *bsx, int *bsy, unsigned int savedColor);

Uint8 mouse_get_state(int *x, int *y);

void mouse_coords_window_to_sim(int *sim_x, int *sim_y, int window_x, int window_y);

#endif

