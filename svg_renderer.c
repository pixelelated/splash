#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "svg_renderer.h"
#include "pixelelated-wordmark.h"

#define MAX_INTERSECTIONS 1000
#define SUBPIXEL_PRECISION 1  // Crisp LCD: one sample at each pixel center

/* Original SVG dimensions used for scaling calculations */
static const float BASE_SVG_WIDTH = PIXELELATED_MARK_WIDTH;
static const float BASE_SVG_HEIGHT = PIXELELATED_MARK_HEIGHT;

/* Structure to track path intersections with scanlines */
typedef struct {
    float x;                 // Floating-point X-coordinate of intersection
    bool is_hole_edge;       // Whether this intersection is from a hole
} Intersection;

/* Pre-calculated cosine values for common rotation angles */
static const float rotation_cos[] = {
    1.0f,   // 0 degrees
    0.0f,   // 90 degrees
    -1.0f,  // 180 degrees
    0.0f    // 270 degrees
};

/* Pre-calculated sine values for common rotation angles */
static const float rotation_sin[] = {
    0.0f,   // 0 degrees
    1.0f,   // 90 degrees
    0.0f,   // 180 degrees
    -1.0f   // 270 degrees
};

/* Comparison function for sorting intersections by x-coordinate */
static int compare_intersections(const void *a, const void *b) {
    float diff = ((Intersection*)a)->x - ((Intersection*)b)->x;
    return (diff < 0) ? -1 : (diff > 0) ? 1 : 0;
}

/* Calculate bounding box of SVG path */
static void calculate_svg_bounds(SVGPath *svg, float *min_x, float *max_x, float *min_y, float *max_y) {
    *min_x = *min_y = 1e6f;
    *max_x = *max_y = -1e6f;

    for (uint32_t i = 0; i < svg->num_paths; i++) {
        Path *path = &svg->paths[i];
        for (uint32_t j = 0; j < path->num_points; j++) {
            if (path->points[j].x < *min_x) *min_x = path->points[j].x;
            if (path->points[j].x > *max_x) *max_x = path->points[j].x;
            if (path->points[j].y < *min_y) *min_y = path->points[j].y;
            if (path->points[j].y > *max_y) *max_y = path->points[j].y;
        }
    }
}

/* Rotate an SVG path by a specified angle
 * Uses pre-calculated sine and cosine values for efficiency
 */
void rotate_svg_path(SVGPath *svg, int angle) {
    // Calculate center of rotation based on original SVG dimensions
    float center_x = BASE_SVG_WIDTH / 2.0f;
    float center_y = BASE_SVG_HEIGHT / 2.0f;

    int angle_index = (angle / 90) % 4;
    float cos_angle = rotation_cos[angle_index];
    float sin_angle = rotation_sin[angle_index];

    // Rotate each point in each path
    for (uint32_t i = 0; i < svg->num_paths; i++) {
        Path *path = &svg->paths[i];
        for (uint32_t j = 0; j < path->num_points; j++) {
            // Translate to origin
            float x = path->points[j].x - center_x;
            float y = path->points[j].y - center_y;

            // Rotate
            float new_x = x * cos_angle - y * sin_angle;
            float new_y = x * sin_angle + y * cos_angle;

            // Translate back
            path->points[j].x = new_x + center_x;
            path->points[j].y = new_y + center_y;
        }
    }
}

/* Render a path including holes using scanline algorithm with crisp pixel-center sampling */
static void render_path(Framebuffer *fb, SVGPath *svg, DisplayInfo *display_info) {
    float min_x, max_x, min_y, max_y;
    calculate_svg_bounds(svg, &min_x, &max_x, &min_y, &max_y);

    // Calculate scaling to maintain aspect ratio
    float scale_x = (float)display_info->svg_width / BASE_SVG_WIDTH;
    float scale_y = (float)display_info->svg_height / BASE_SVG_HEIGHT;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    // Calculate centering offsets
    float offset_x = display_info->x_offset;
    float offset_y = display_info->y_offset;

    // Adjust offset to center the scaled SVG
    offset_x += (display_info->svg_width - (BASE_SVG_WIDTH * scale)) / 2;
    offset_y += (display_info->svg_height - (BASE_SVG_HEIGHT * scale)) / 2;

    // Calculate screen space bounds with one-pixel clipping padding
    int screen_min_y = (int)((min_y * scale + offset_y) - 1);
    int screen_max_y = (int)((max_y * scale + offset_y) + 1);

    // Clip to screen bounds
    if (screen_min_y < 0) screen_min_y = 0;
    if (screen_max_y >= (int)fb->vinfo.yres) screen_max_y = fb->vinfo.yres - 1;

    // Allocate intersection array
    Intersection *intersections = malloc(MAX_INTERSECTIONS * sizeof(Intersection));
    if (!intersections) return;

    // Convert color components to 32-bit color
    uint32_t fill_color = (svg->fill_color.r << 16) |
                         (svg->fill_color.g << 8) |
                          svg->fill_color.b;

    // Create a temporary buffer to track pixel coverage
    float *coverage_buffer = calloc(fb->vinfo.xres, sizeof(float));
    if (!coverage_buffer) {
        free(intersections);
        return;
    }

    // Process each scanline at pixel centers
    for (int y = screen_min_y; y <= screen_max_y; y++) {
        // Clear coverage buffer for the current scanline
        memset(coverage_buffer, 0, fb->vinfo.xres * sizeof(float));

        // Sample once at the pixel center; preserve LCD gaps
        for (int subpixel = 0; subpixel < SUBPIXEL_PRECISION; subpixel++) {
            float subpixel_y = y + 0.5f;
            int num_intersections = 0;

            // Find intersections with all path segments
            for (uint32_t i = 0; i < svg->num_paths; i++) {
                Path *path = &svg->paths[i];
                for (uint32_t j = 0; j < path->num_points; j++) {
                    uint32_t k = (j + 1) % path->num_points;

                    float y1 = path->points[j].y * scale + offset_y;
                    float y2 = path->points[k].y * scale + offset_y;

                    // Check if segment crosses current scanline
                    if ((y1 <= subpixel_y && y2 > subpixel_y) || (y2 <= subpixel_y && y1 > subpixel_y)) {
                        float x1 = path->points[j].x * scale + offset_x;
                        float x2 = path->points[k].x * scale + offset_x;

                        if (num_intersections < MAX_INTERSECTIONS) {
                            float x;
                            if (y1 == y2) {
                                x = x1;
                            } else {
                                // Calculate intersection x-coordinate with floating-point precision
                                x = x1 + (subpixel_y - y1) * (x2 - x1) / (y2 - y1);
                            }

                            intersections[num_intersections].x = x;
                            intersections[num_intersections].is_hole_edge = path->is_hole;
                            num_intersections++;
                        }
                    }
                }
            }

            if (num_intersections > 0) {
                // Sort intersections by x-coordinate
                qsort(intersections, num_intersections, sizeof(Intersection), compare_intersections);

                bool inside_main = false;
                bool inside_hole = false;

                // Accumulate coverage between pairs of intersections
                for (int i = 0; i < num_intersections - 1; i++) {
                    if (intersections[i].is_hole_edge) {
                        inside_hole = !inside_hole;
                    } else {
                        inside_main = !inside_main;
                    }

                    // Only fill if inside main path and not inside hole
                    if (inside_main && !inside_hole) {
                        float x_start = intersections[i].x;
                        float x_end = intersections[i + 1].x;

                        // Process each pixel with crisp pixel-center sampling
                        int ix_start = (int)ceilf(x_start - 0.5f);
                        int ix_end = (int)ceilf(x_end - 0.5f) - 1;

                        // Clip to screen bounds
                        if (ix_start < 0) ix_start = 0;
                        if (ix_end >= (int)fb->vinfo.xres) ix_end = fb->vinfo.xres - 1;

                        // Accumulate coverage for each pixel
                        for (int x = ix_start; x <= ix_end; x++) {
                            coverage_buffer[x] = 1.0f;
                        }
                    }
                }
            }
        }

        // Render the scanline using the accumulated coverage
        for (uint32_t x = 0; x < fb->vinfo.xres; x++) {
            if (coverage_buffer[x] > 0.0f) {
                // Clamp coverage to [0, 1]
                if (coverage_buffer[x] > 1.0f) coverage_buffer[x] = 1.0f;

                // LCD artwork has hard RGB555 bands: sample coverage without
                // blending with black, which would invent intermediate colors.
                if (coverage_buffer[x] >= 0.5f) {
                    set_pixel(fb, x, y, fill_color);
                }
            }
        }
    }

    // Clean up
    free(coverage_buffer);
    free(intersections);
}

/* Render an SVG path to the framebuffer with crisp pixel-center sampling */
void render_svg_path(Framebuffer *fb, SVGPath *svg, DisplayInfo *display_info) {
    static bool first_path = true;

    // Clear screen before rendering first path
    if (first_path) {
        for (uint32_t y = 0; y < fb->vinfo.yres; y++) {
            for (uint32_t x = 0; x < fb->vinfo.xres; x++) {
                set_pixel(fb, x, y, 0x00000000);
            }
        }
        first_path = false;
    }

    render_path(fb, svg, display_info);
}
