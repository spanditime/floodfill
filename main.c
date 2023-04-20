#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

typedef struct
{
    unsigned int x, y;
} pair;

// очередь - двоичная куча
struct Queue
{
    unsigned int *arr;
    unsigned int size;
    unsigned int capacity;
};

// создает новую структуру очереди
struct Queue *create_queue(unsigned int size)
{
    struct Queue *queue = (struct Queue *)
        malloc(sizeof(struct Queue));
    queue->size = size;
    queue->capacity = 0;
    queue->arr = (unsigned int *)calloc(sizeof(unsigned int), size);
    return queue;
}

// возвращает 0 если очередь не пуста
int is_empty(struct Queue *queue)
{
    return queue->capacity == 0;
}

//добавляет элемент в очередь
void enqueue(struct Queue *queue, unsigned int item)
{
    if (queue->capacity == queue->size)
    {
        // exc
        return;
    }
    unsigned int i, parent;
    i = queue->capacity;
    queue->capacity++;
    queue->arr[i] = item;
    parent = (i - 1) / 2;
    while (parent >= 0 && i > 0)
    {
        if (queue->arr[i] > queue->arr[parent])
        {
            int temp = queue->arr[i];
            queue->arr[i] = queue->arr[parent];
            queue->arr[parent] = temp;
        }
        i = parent;
        parent = (i - 1) / 2;
    }
}

//удаляет элемент из очереди
void dequeue(struct Queue *queue)
{
    if (is_empty(queue))
        return;
    queue->capacity--;
}

// возвращает элемент в очереди
unsigned int peek(struct Queue *queue)
{
    if (is_empty(queue))
        return 0;
    return queue->arr[queue->capacity - 1];
}

typedef struct
{
    png_byte color_type;
    png_byte bit_depth;
    png_bytep *row_pointers;
    size_t width;
    size_t height;
} bitmap_t;

// создает новую пустую структуру изображения
bitmap_t *create_empty_bitmap()
{
    bitmap_t *bitmap = (bitmap_t *)malloc(sizeof(bitmap_t));
    bitmap->row_pointers = NULL;
    return bitmap;
}

// освобождает память используемую структурой bitmap
void destroy_bitmap(bitmap_t *bitmap)
{
    for (int y = 0; y < bitmap->height; y++)
    {
        free(bitmap->row_pointers[y]);
    }
    free(bitmap->row_pointers);
    free(bitmap);
}

// возвращает пиксель на позиции x,y
static png_byte *pixel_at(bitmap_t *bitmap, unsigned int x, unsigned int y)
{
    return bitmap->row_pointers[y] + x * 4u;
}

// читает изображение из указанного файла
int read_png_file(bitmap_t *bitmap, char *filename)
{

    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        return 1;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
    {
        return 2;
    }

    png_infop info = png_create_info_struct(png);
    if (!info)
    {
        return 2;
    }

    if (setjmp(png_jmpbuf(png)))
    {
        return 2;
    }

    png_init_io(png, fp);

    png_read_info(png, info);

    bitmap->width = png_get_image_width(png, info);
    bitmap->height = png_get_image_height(png, info);
    bitmap->color_type = png_get_color_type(png, info);
    bitmap->bit_depth = png_get_bit_depth(png, info);

    if (bitmap->bit_depth == 16)
        png_set_strip_16(png);

    if (bitmap->color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if (bitmap->color_type == PNG_COLOR_TYPE_GRAY && bitmap->bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if (bitmap->color_type == PNG_COLOR_TYPE_RGB ||
        bitmap->color_type == PNG_COLOR_TYPE_GRAY ||
        bitmap->color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if (bitmap->color_type == PNG_COLOR_TYPE_GRAY ||
        bitmap->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    if (bitmap->row_pointers)
    {
        return 2;
    }

    bitmap->row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * bitmap->height);
    for (int y = 0; y < bitmap->height; y++)
    {
        bitmap->row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
    }

    png_read_image(png, bitmap->row_pointers);

    fclose(fp);

    png_destroy_read_struct(&png, &info, NULL);
    return 0;
}

// сохраненяет изображение с указанным именем
int write_png_file(bitmap_t *bitmap, char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        return 1;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
    {
        return 2;
    }

    png_infop info = png_create_info_struct(png);
    if (!info)
    {
        return 2;
    }

    if (setjmp(png_jmpbuf(png)))
    {
        return 2;
    }

    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png,
        info,
        bitmap->width, bitmap->height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    // png_set_filler(png, 0, PNG_FILLER_AFTER);

    if (!bitmap->row_pointers)
    {
        return 2;
    }

    png_write_image(png, bitmap->row_pointers);
    png_write_end(png, NULL);

    fclose(fp);

    png_destroy_write_struct(&png, &info);
    return 0;
}

// изменяет цвет пиксела x,y на new
void change_color(bitmap_t *bitmap, unsigned int x, unsigned int y, png_bytep new)
{
    png_bytep pixel = pixel_at(bitmap, x, y);
    pixel[0] = new[0];
    pixel[1] = new[1];
    pixel[2] = new[2];
    pixel[3] = new[3];
}
// сравнивает два пикселя по компонентам возвращает 0 если они не равны
int equal(png_bytep left, png_bytep right)
{
    return left[0] == right[0] && left[1] == right[1] && left[2] == right[2] && left[3] == right[3];
}

// заполняет последовательно область цветом new начиная с точки x y
void flood_fill(bitmap_t *bitmap, int x, int y, png_bytep new)
{
    unsigned int queue_size = bitmap->width * bitmap->height;
    struct Queue *queue = create_queue(queue_size);         // создаем новую очередь
    png_bytep old = (png_bytep)calloc(sizeof(png_byte), 4); // сохраняем старый цвет
    {
        png_bytep _old = pixel_at(bitmap, x, y);
        old[0] = _old[0];
        old[1] = _old[1];
        old[2] = _old[2];
        old[3] = _old[3];
    }
    enqueue(queue, x + y * bitmap->width); // добавляем первый пиксел в очередь
    pair curr;
    while (!is_empty(queue)) // до тех пор пока очередь не пуста
    {
        unsigned int idx = peek(queue);
        curr.x = idx % bitmap->width;
        curr.y = idx / bitmap->width;
        dequeue(queue);                                              // удаляем жлемент из очереди
        change_color(bitmap, curr.x, curr.y, new);                   // перекрашиваем его в новый цвет
        for (unsigned int direction = 0; direction < 4; direction++) // для всех четырех направлений
        {
            pair next;
            next.x = curr.x;
            next.y = curr.y;
            switch (direction)
            {
            case 0:
                if (next.x + 1 >= bitmap->width) // вправо
                    continue;                    // если с краю то пропускаем
                next.x++;
                break;
            case 1:
                if (next.x == 0) // влево
                    continue;    // если с краю то пропускаем
                next.x--;
                break;
            case 2:
                if (next.y + 1 >= bitmap->height) // вниз
                    continue;                     // если с краю то пропускаем
                next.y++;
                break;
            case 3:
                if (next.y == 0) // вверх
                    continue;    // если с краю то пропускаем
                next.y--;
                break;
            default:
                break;
            }
            if (equal(old, pixel_at(bitmap, next.x, next.y))) // если цвет смежного рассматриваемого пиксела равен изначальному
            {
                enqueue(queue, next.x + next.y * bitmap->width); // добавляем его в очередь
            }
        }
    }
}

// a.exe input x y r g b (a) output
int main(int argc, char **argv)
{
    bitmap_t *bitmap = create_empty_bitmap();
    int newRed, newGreen, newBlue, newAlpha = 255;
    png_bytep new = (png_bytep)calloc(sizeof(png_byte), 4);
    unsigned int x, y;
    char *infn;
    char *ofn;
    // проверяем аргументы переданные пользователем на правильность
    if (argc == 9)
    {
        if (sscanf(argv[7], "%d", &newAlpha) != 1 || newAlpha > 255 || newAlpha < 0)
        {
            printf("Parameter 7 a - color alpha should be a number in range of [0;255]\n");
            abort();
        }
        ofn = argv[8];
    }
    else if (argc == 8)
    {
        ofn = argv[7];
    }
    else
    {
        printf("Expected 7 to 8 parameters: input x y r g b (a) output, got %d\n", argc - 1);
        abort();
    }
    // считываем входное изображение
    infn = argv[1];
    {
        int res = read_png_file(bitmap, infn);
        switch (res)
        {
        case 1: // в случае ошибок выводим сообщение об ошибке
            printf("An error occured trying to read %s, check that the file is exists and accessible", infn);
            abort();
            break;
        case 2:
            printf("An error occured trying to read image from %s", infn);
            abort();
            break;
        default:
            break;
        }
    }
    if (sscanf(argv[2], "%d", &x) != 1 || x >= bitmap->width || x < 0)
    {
        printf("Parameter 2 x - x coordinate that the fill will be started in, should be a number in range of [0;image width=%d)\n", bitmap->width);
        abort();
    }
    if (sscanf(argv[3], "%d", &y) != 1 || y >= bitmap->height || y < 0)
    {
        printf("Parameter 3 y - y coordinate that the fill will be started in, should be a number in range of [0;image height=%d)\n", bitmap->height);
        abort();
    }
    if (sscanf(argv[4], "%d", &newRed) != 1 || newRed > 255 || newRed < 0)
    {
        printf("Parameter 4 r - color red should be a number in range of [0;255]\n");
        abort();
    }
    if (sscanf(argv[5], "%d", &newGreen) != 1 || newGreen > 255 || newGreen < 0)
    {
        printf("Parameter 5 g - color green should be a number in range of [0;255]\n");
        abort();
    }
    if (sscanf(argv[6], "%d", &newBlue) != 1 || newBlue > 255 || newBlue < 0)
    {
        printf("Parameter 6 b - color blue should be a number in range of [0;255]\n");
        abort();
    }
    new[0] = newRed;
    new[1] = newBlue;
    new[2] = newGreen;
    new[3] = newAlpha;
    // заполняем изображение
    flood_fill(bitmap, x, y, new);
    // и сохраняем его
    int err = write_png_file(bitmap, ofn);
    switch (err)
    { // в случае ошибки выводим сообщение
    case 1:
        printf("An error occured trying to open or create %s, check that the file is accessible", infn);
        abort();
        break;
    case 2:
        printf("An error occured trying to write image to %s", infn);
        abort();
        break;
    default:
        break;
    }

    destroy_bitmap(bitmap);
    return err;
}
