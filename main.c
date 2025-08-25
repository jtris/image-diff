#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

typedef int Errno;

typedef struct {
    uint8_t (*pixel_map)[3];
    size_t width, height;
} image;


Errno read_ppm(image *save_image, const char *filepath);
Errno create_ppm(image *save_image, const char *filepath);
Errno create_sample_ppm();
void destroy_image_pixel_map(image *image_object);


int main()
{
    image image_object;

    // ...

    destroy_image_pixel_map(&image_object);
    return 0;
}


Errno create_ppm(image *save_image, const char *filepath)
{
    FILE *f = NULL;
    f = fopen(filepath, "wb");
    if (f == NULL) return 1;

    // write header
    fprintf(f, "P6\n%zu %zu\n255\n", save_image->width, save_image->height); 

    // write the image data
    size_t image_resolution = save_image->width*save_image->height;
    fwrite(save_image->pixel_map, sizeof(uint8_t), image_resolution*3, f); //?

    fclose(f);
    return 0;
}


Errno read_ppm(image *save_image, const char *filepath)
{
    FILE *f = NULL;
    f = fopen(filepath, "rb");
    if (f == NULL) return 1;

    // verify the format (P6)
    char format_buffer[3];
    fscanf(f, "%2s", format_buffer);
    if (strcmp(format_buffer, "P6")) return 1;

    // read image resolution
    fscanf(f, "%zu %zu", &(save_image->width), &(save_image->height));

    // verify the color palette
    uint8_t color_palette;
    fscanf(f, "%hhu\n", &color_palette);
    if (color_palette != 255) return 1;

    // read the image data
    size_t image_resolution = save_image->height*save_image->width;

    uint8_t (*pixels)[3] = malloc(sizeof(uint8_t[image_resolution][3]));
    fread(pixels, sizeof(uint8_t), image_resolution*3, f);
    save_image->pixel_map = pixels;

    fclose(f);
    return 0;
}


Errno create_sample_ppm()
{
    const char *filename = "striped.ppm";
    const size_t width = 600;
    const size_t height = 400;
    const size_t image_resolution = width * height;
    uint8_t pixel1[3] = {255, 225, 0};
    uint8_t pixel2[3] = {0, 0, 0};
    FILE *f = NULL;

    f = fopen(filename, "wb");
    if (f == NULL) {
        return 1;
    }
    
    fprintf(f, "P6\n%zu %zu\n255\n", width, height); // write header
    
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < image_resolution/4; ++j) {
            fwrite(pixel1, sizeof(uint8_t), 3, f);
        }
        for (size_t k = 0; k < image_resolution/4; ++k) {
            fwrite(pixel2, sizeof(uint8_t), 3, f);
        }
    }

    fclose(f);
    return 0;
}


void destroy_image_pixel_map(image *image_object)
{
    free(image_object->pixel_map);
    image_object->pixel_map = NULL;
}

