/* Private state for the separately selected segmented exporter. */
#define PART_BYTES (1024U * 1024U)
#define MAX_PARTS (MAX_IMAGE / PART_BYTES)
static char *segment_stage;
static int segment_stage_owned;
static void segments_cleanup(void);
static void segments_run(int check_only, int verify_only, uint32_t entry);
