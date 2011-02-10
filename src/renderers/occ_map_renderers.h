#ifndef __OCC_MAP_RENDERER_H__
#define __OCC_MAP_RENDERER_H__

/**
 * @defgroup PixelMapViewerRenderer BotViewer renderer
 * @ingroup OccupancyMap
 * @brief BotVis Viewer renderer plugin
 * @include occ_map/occ_map_renderers.h
 *
 * TODO
 *
 * Linking: `pkg-config --libs occ_map_pixelmap_render`
 * @{
 */

#include <bot_vis/bot_vis.h>

#ifdef __cplusplus
extern "C" {
#endif

void occ_map_pixel_map_add_renderer_to_viewer(BotViewer *viewer, int render_priority);
void occ_map_voxel_map_add_renderer_to_viewer(BotViewer *viewer, int render_priority);

#ifdef __cplusplus
}
#endif

#endif /* RENDERER_PIXEL_MAP_H_ */
