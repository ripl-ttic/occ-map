#include <lcm/lcm.h>
#include <occ_map/PixelMap.hpp>
#include <string>

using namespace std;
using namespace occ_map;

void usage(char * name)
{
  fprintf(stderr, "usage: %s filename channel repeat_period(sec)\n", name);
  exit(1);
}

int main(int argc, char ** argv)
{
  lcm_t * lcm = lcm_create(NULL);
  float repeat_period = -1;
  if (argc < 3 || argc > 4)
    usage(argv[0]);

  char * filename = argv[1];
  char * channel = argv[2];
  fprintf(stderr, "loading map from %s, and publishing on channel %s\n",filename, channel);

  if (argc >= 4) {
    repeat_period = atof(argv[3]);
    fprintf(stderr, "republishing every %f seconds\n", repeat_period);
  }

  occ_map_pixel_map_t * pix_map = PixelMap<float>::load_pixel_map_t_from_file(filename);

  while (true) {
    occ_map_pixel_map_t_publish(lcm, channel, pix_map);
    if (repeat_period > 0)
      usleep(1.0e6 * repeat_period);
    else
      break;
  }
  occ_map_pixel_map_t_destroy(pix_map);
}

