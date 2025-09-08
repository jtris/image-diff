#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#define ERROR_MESSAGES_ENABLED 1
#define ERROR_MESSAGE(...) do {                             \
        if (ERROR_MESSAGES_ENABLED) {                       \
            fprintf(stderr, "%s:%d : ", __FILE__, __LINE__);\
            fprintf(stderr, __VA_ARGS__);                   \
        }                                                   \
    } while(0)
#define ASSERT(_e, ...) if (!(_e)) { fprintf(stderr, __VA_ARGS__); exit(1); }

typedef int Errno;

typedef struct {
    uint8_t (*pixel_map)[3];
    size_t width, height;
    size_t resolution;
} image;


Errno get_file_extension_from_path(const char *filepath, char *buffer);
Errno read_png_or_jpg(image *save_image, const char *filepath); // TO-DO
Errno export_as_png_or_jpg(image *save_image, const char *filepath);
uint16_t get_two_pixel_difference(const uint8_t pixel1[3], const uint8_t pixel2[3]);
Errno create_pixel_difference_heatmap(image *save_image, const image *image1, const image *image2);
Errno read_ppm(image *save_image, const char *filepath);
Errno export_as_ppm(const image *save_image, const char *filepath);
Errno create_sample_ppm();
void destroy_image_pixel_map(image *image_object);


int main()
{
    // ...
    return 0;
}


Errno get_file_extension_from_path(const char *filepath, char *buffer)
{
    size_t filepath_length = 0;
    for (int i = 0; ; ++i) {
        if (filepath[i] == 0) break;
        filepath_length++;
    }
    
    if (filepath_length <= 5) { ERROR_MESSAGE("filepath is too short\n"); return 1; }
    if (filepath[filepath_length-4] != '.') { ERROR_MESSAGE("couldn't find a file extension\n"); return 1; }

    // load last 3 characters into buffer
    for (int i = 0; i < 3; ++i) {
        buffer[i] = filepath[filepath_length-3+i];
    }
    buffer[3] = 0;
    return 0;
}


Errno export_as_png_or_jpg(image *save_image, const char *filepath)
{
    char file_extension[4] = {[3] = 0};
    Errno err = get_file_extension_from_path(filepath, file_extension);
    if (err) { ERROR_MESSAGE("an error occured while trying to read the file extension\n"); return 1; }

    // png
    if (strcmp(file_extension, "png") == 0) {
        err = stbi_write_png(filepath, save_image->width, save_image->height, 3, save_image->pixel_map, save_image->width*3);
        if (err == 0) { ERROR_MESSAGE("couldn't write a PNG image\n"); return 1; } // 0 means failure here
        return 0;
    }

    // jpg
    if (strcmp(file_extension, "jpg") == 0) {
        err = stbi_write_jpg(filepath, save_image->width, save_image->height, 3, save_image->pixel_map, 100);
        if (err == 0) { ERROR_MESSAGE("couldn't write a JPG image\n"); return 1; } // 0 means failure here
        return 0;
    }

    ERROR_MESSAGE("save-path's file extension is neither of type PNG, nor JPG.\n");
    return 1;
}


Errno read_png_or_jpg(image *save_image, const char *filepath)
{
    int width, height;
    unsigned char *data = stbi_load(filepath, &width, &height, NULL, 3);
    if (data == NULL) { ERROR_MESSAGE("couldn't load file\n"); return 1; }

    save_image->width = width;
    save_image->height = height;
    save_image->resolution = save_image->width * save_image->height;

    // map the retrieved data to our image struct's pixel map
    save_image->pixel_map = malloc(sizeof(uint8_t[save_image->resolution][3]));

    for (size_t i = 0; i < save_image->resolution; ++i) {
        save_image->pixel_map[i][0] = data[i*3+0];
        save_image->pixel_map[i][1] = data[i*3+1];
        save_image->pixel_map[i][2] = data[i*3+2];
    }

    stbi_image_free(data);
    return 0;
}

Errno create_pixel_difference_heatmap(image *save_image, const image *image1, const image *image2)
{
    // verify image compability
    if (image1->width != image2->width || image1->height != image2->height) return 1;
    
    // initialize the save_image
    save_image->width = image1->width;
    save_image->height = image1->height;
    save_image->resolution = image1->resolution;

    // compute all "pixel difference" values
    uint16_t *pixel_differences = malloc(save_image->resolution * sizeof(uint16_t));
    if (!pixel_differences) { ERROR_MESSAGE("could not allocate\n"); return 1; }

    uint16_t highest_difference_value = 0;
    for (size_t i = 0; i < save_image->resolution; ++i) {
        pixel_differences[i] = get_two_pixel_difference(image1->pixel_map[i], image2->pixel_map[i]);
        
        if (pixel_differences[i] > highest_difference_value)
            highest_difference_value = pixel_differences[i];
    }

    // scale difference values to convert them to pixels
    uint8_t *scaled_pixel_differences = malloc(save_image->resolution * sizeof(uint8_t));
    if (!scaled_pixel_differences) { ERROR_MESSAGE("could not allocate\n"); return 1; }

    for (size_t p = 0; p < save_image->resolution; ++p) {
        scaled_pixel_differences[p] = 255 * (1.0f - ((float)pixel_differences[p] / (float)highest_difference_value));
    }

    // convert scaled pixel difference values into image data
    save_image->pixel_map = malloc(sizeof(uint8_t[save_image->resolution][3]));

    for (size_t n = 0; n < save_image->resolution; ++n) {
        uint8_t pixel_value = scaled_pixel_differences[n];

        for (size_t m = 0; m < 3; ++m) {
            save_image->pixel_map[n][m] = pixel_value;
        }
    }

    free(pixel_differences);
    free(scaled_pixel_differences);
    return 0;
}


uint16_t get_two_pixel_difference(const uint8_t pixel1[3], const uint8_t pixel2[3])
{
    uint16_t difference = 0;

    for (int i = 0; i < 3; ++i) {
        difference += abs(pixel1[i] - pixel2[i]);
    }
    return difference;
}


Errno export_as_ppm(const image *save_image, const char *filepath)
{
    FILE *f = NULL;
    f = fopen(filepath, "wb");
    if (f == NULL) { ERROR_MESSAGE("could not open file\n"); return 1; }

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
    if (f == NULL) { ERROR_MESSAGE("could not open file (%s)\n", filepath); return 1; }

    // verify the format (P6)
    char format_buffer[3];
    fscanf(f, "%2s", format_buffer);
    if (strcmp(format_buffer, "P6")) return 1;

    // read image resolution
    fscanf(f, "%zu %zu", &(save_image->width), &(save_image->height));
    save_image->resolution = save_image->width * save_image->height;

    // verify the color palette
    uint8_t color_palette;
    fscanf(f, "%hhu\n", &color_palette);
    if (color_palette != 255) return 1;

    // read the image data
    uint8_t (*pixels)[3] = malloc(sizeof(uint8_t[save_image->resolution][3]));
    fread(pixels, sizeof(uint8_t), save_image->resolution*3, f);
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
        ERROR_MESSAGE("couldn't open file (%s)\n", filename);
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

