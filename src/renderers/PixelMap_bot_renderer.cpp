/*
 * renders a PixelMap
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//#include <gtk/gtk.h>
//#include <GL/gl.h>
//#include <GL/glu.h>

#include <bot_vis/bot_vis.h>
#include <lcm/lcm.h>
#include <bot_core/bot_core.h>

#include <occ_map/PixelMap.hpp>

#include "occ_map_renderers.h"

#define PARAM_MAP_MODE "Map Mode"
#define PARAM_ALPHA "Alpha"
#define PARAM_COLOR "COLOR"
#define PARAM_COLOR_MAP "COLOR MAP"

#define PARAM_Z_OFFSET "Z-Offset"
//#define PARAM_FOLLOW_Z "Follow Vehicle Z"

#define DEFAULT_RENDERER_NAME "PixelMap"

using namespace occ_map;

typedef struct _OccMapRendererPixelMap OccMapRendererPixelMap;

struct _OccMapRendererPixelMap {
  BotRenderer renderer;
  BotEventHandler ehandler;
  lcm_t *lc;
  BotGtkParamWidget *pw;
  GtkWidget *label;
  BotViewer *viewer;
  Uint8PixelMap *pix_map;
  BotGlTexture *map2dtexture;
  int textureSize[2];
};

static void upload_map_texture(OccMapRendererPixelMap *self);

uint8_t floatToUint8(float v)
{
  return v * 255.0;
}

static void pixel_map_handler(const lcm_recv_buf_t *rbuf, const char *channel, const occ_map_pixel_map_t *msg,
    void *user)
{
  OccMapRendererPixelMap *self = (OccMapRendererPixelMap*) user;
  if (self->pix_map != NULL)
    delete self->pix_map;
  if (msg->data_type == OCC_MAP_PIXEL_MAP_T_TYPE_FLOAT) {
    FloatPixelMap * fmap = new FloatPixelMap(msg);
    self->pix_map = new Uint8PixelMap(fmap, floatToUint8);
    delete fmap;
  }
  else if (msg->data_type == OCC_MAP_PIXEL_MAP_T_TYPE_UINT8) {
    self->pix_map = new Uint8PixelMap(msg);
  }
  else if (msg->data_type == 0) {
    fprintf(stderr, "Warning, pixmap datatype = 0, assuming it's a float!\n");
    FloatPixelMap * fmap = new FloatPixelMap(msg);
    self->pix_map = new Uint8PixelMap(fmap, floatToUint8);
    delete fmap;
  }
  else {
    fprintf(stderr, "pixmap datatype %d not handled\n", msg->data_type);
  }

  upload_map_texture(self);
  bot_viewer_request_redraw(self->viewer);
}

static float shift_unexplored(float v)
{
  if (fabs(v - .5) < .1)
    return .15;
  else
    return v;
}

static float shift_unexplored_and_invert(float v)
{
  return 1 - shift_unexplored(v);
}

static uint8_t invert(uint8_t v)
{
  return 255 - v;
}

static void upload_map_texture(OccMapRendererPixelMap *self)
{

  if (self->pix_map != NULL) {
    Uint8PixelMap * drawMap = NULL;

    //TBD - we shouldn't be doing this anymore
    // makes assumptions about pixelmap format
    //    if (!bot_gtk_param_widget_get_bool(self->pw, PARAM_COLOR_MAP)) {
    //      drawMap = new FloatPixelMap(self->pix_map,shift_unexplored_and_invert);
    //    }
    //    else {
    //      drawMap = new FloatPixelMap(self->pix_map,shift_unexplored);
    //    }

    drawMap = new Uint8PixelMap(self->pix_map, invert);

    // create the texture object if necessary
    if (self->map2dtexture == NULL || (drawMap->dimensions[0] != self->textureSize[0] || drawMap->dimensions[1]
        != self->textureSize[1])) {
      if (self->map2dtexture != NULL)
        bot_gl_texture_free(self->map2dtexture);
      int data_size = drawMap->num_cells * sizeof(uint8_t);
      self->map2dtexture = bot_gl_texture_new(drawMap->dimensions[0], drawMap->dimensions[1], data_size);
      self->textureSize[0] = drawMap->dimensions[0];
      self->textureSize[1] = drawMap->dimensions[1];
    }
    bot_gl_texture_upload(self->map2dtexture, GL_LUMINANCE, GL_UNSIGNED_BYTE, drawMap->dimensions[0] * sizeof(uint8_t),
        drawMap->data);
    delete drawMap;
  }

}

static void PixelMap_draw(BotViewer *viewer, BotRenderer *renderer)
{
  OccMapRendererPixelMap *self = (OccMapRendererPixelMap*) renderer;

  if (self->map2dtexture) {

    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_COLOR_MAP)) {
      float * colorV = bot_color_util_jet(bot_gtk_param_widget_get_double(self->pw, PARAM_COLOR));
      glColor4f(colorV[0], colorV[1], colorV[2], bot_gtk_param_widget_get_double(self->pw, PARAM_ALPHA));
    }
    else {
      glColor4f(1, 1, 1, bot_gtk_param_widget_get_double(self->pw, PARAM_ALPHA));
    }

    //    glEnable(GL_DEPTH_TEST);
    glPushMatrix();

    double z_offset = bot_gtk_param_widget_get_double(self->pw, PARAM_Z_OFFSET);
    //    if (bot_gtk_param_widget_get_bool(self->pw,PARAM_FOLLOW_Z))
    //      glTranslatef(0, 0, cur_pose.pos[2] + z_offset);
    //    else
    glTranslatef(0, 0, z_offset);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bot_gl_texture_draw_coords(self->map2dtexture, self->pix_map->xy0[0], self->pix_map->xy0[1], 0,
        self->pix_map->xy0[0], self->pix_map->xy1[1], 0, self->pix_map->xy1[0], self->pix_map->xy1[1], 0,
        self->pix_map->xy1[0], self->pix_map->xy0[1], 0);

    glPopMatrix();
    glDisable(GL_BLEND);
    //    glDisable(GL_DEPTH_TEST);
  }

}

static void PixelMap_free(BotRenderer *renderer)
{
  free(renderer);
}

static int mouse_press(BotViewer *viewer, BotEventHandler *ehandler, const double ray_start[3],
    const double ray_dir[3], const GdkEventButton *event)
{
  return 1;
}

static int mouse_release(BotViewer *viewer, BotEventHandler *ehandler, const double ray_start[3],
    const double ray_dir[3], const GdkEventButton *event)
{
  return 0;
}

static int mouse_motion(BotViewer *viewer, BotEventHandler *ehandler, const double ray_start[3],
    const double ray_dir[3], const GdkEventMotion *event)
{
  return 1;
}

static int key_press(BotViewer *viewer, BotEventHandler *ehandler, const GdkEventKey *event)
{
  return 0;
}

static void on_load_preferences(BotViewer *viewer, GKeyFile *keyfile, void *user)
{
  if (!viewer)
    return;

  OccMapRendererPixelMap *self = (OccMapRendererPixelMap*) user;
  bot_gtk_param_widget_load_from_key_file(self->pw, keyfile, self->renderer.name);
}

static void on_save_preferences(BotViewer *viewer, GKeyFile *keyfile, void *user)
{
  if (!viewer)
    return;

  OccMapRendererPixelMap *self = (OccMapRendererPixelMap*) user;
  bot_gtk_param_widget_save_to_key_file(self->pw, keyfile, self->renderer.name);
}

static void on_param_widget_changed(BotGtkParamWidget *pw, const char *name, void *user)
{
  OccMapRendererPixelMap *self = (OccMapRendererPixelMap*) user;
  upload_map_texture(self);
  bot_viewer_request_redraw(self->viewer);
}

static BotRenderer*
renderer_pixel_map_new(BotViewer *viewer, int render_priority, const char* lcm_channel, const char* renderer_name)
{
  OccMapRendererPixelMap *self = (OccMapRendererPixelMap*) calloc(1, sizeof(OccMapRendererPixelMap));
  BotRenderer *renderer = &self->renderer;
  self->viewer = viewer;
  self->lc = bot_lcm_get_global(NULL);

  renderer->draw = PixelMap_draw;
  renderer->destroy = PixelMap_free;
  renderer->widget = gtk_vbox_new(FALSE, 0);
  renderer->name = (char *) renderer_name; //(char *) DEFAULT_RENDERER_NAME;
  renderer->user = self;
  renderer->enabled = 1;

  BotEventHandler *ehandler = &self->ehandler;
  ehandler->name = (char *) self->renderer.name;
  ehandler->enabled = 1;
  ehandler->pick_query = NULL;
  ehandler->key_press = key_press;
  ehandler->hover_query = NULL;
  ehandler->mouse_press = mouse_press;
  ehandler->mouse_release = mouse_release;
  ehandler->mouse_motion = mouse_motion;
  ehandler->user = self;

  // --- SETUP SIDE BOX WIDGET
  self->pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());
  gtk_box_pack_start(GTK_BOX(renderer->widget), GTK_WIDGET(self->pw), TRUE, TRUE, 0);

  bot_gtk_param_widget_add_double(self->pw, PARAM_ALPHA, BOT_GTK_PARAM_WIDGET_SLIDER, 0, 1, 0.05, 0.7);
  bot_gtk_param_widget_add_booleans(self->pw, BOT_GTK_PARAM_WIDGET_CHECKBOX, PARAM_COLOR_MAP, 0, NULL);
  bot_gtk_param_widget_add_double(self->pw, PARAM_COLOR, BOT_GTK_PARAM_WIDGET_SLIDER, 0, 1, 0.05, 0.7);

  bot_gtk_param_widget_add_double(self->pw, PARAM_Z_OFFSET, BOT_GTK_PARAM_WIDGET_SLIDER, -3, 3, 0.05, -0.1);
  //  bot_gtk_param_widget_add_booleans(self->pw, BOT_GTK_PARAM_WIDGET_CHECKBOX, PARAM_FOLLOW_Z, 1, NULL);

  gtk_widget_show_all(renderer->widget);
  g_signal_connect(G_OBJECT(self->pw), "changed", G_CALLBACK(on_param_widget_changed), self);
  g_signal_connect(G_OBJECT(viewer), "load-preferences", G_CALLBACK(on_load_preferences), self);
  g_signal_connect(G_OBJECT(viewer), "save-preferences", G_CALLBACK(on_save_preferences), self);

  // pick a default channel if none specified
  if (!lcm_channel || !strlen(lcm_channel))
    lcm_channel = "PIXEL_MAP";
  occ_map_pixel_map_t_subscribe(self->lc, lcm_channel, pixel_map_handler, self);
  return &self->renderer;
}

extern "C" void occ_map_pixel_map_add_renderer_to_viewer(BotViewer *viewer, int render_priority,
    const char* lcm_channel, const char* renderer_name)
{
  BotRenderer* renderer = renderer_pixel_map_new(viewer, render_priority, lcm_channel, renderer_name);
  bot_viewer_add_renderer(viewer, renderer, render_priority);
}

/*
 * setup_renderer:
 * Generic renderer add function for use as a dynamically loaded plugin
 */
extern "C" void add_renderer_to_plugin_viewer(BotViewer *viewer, int render_priority)
{
  occ_map_pixel_map_add_renderer_to_viewer(viewer, render_priority, NULL, DEFAULT_RENDERER_NAME);
}
