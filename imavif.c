#define IMAGER_NO_CONTEXT

#include "imavif.h"
#include "avif/avif.h"
#include "imext.h"
#include <errno.h>
#include <string.h>

i_img   *
i_readavif(io_glue *ig, int page) {
  return NULL;
}

i_img  **
i_readavif_multi(io_glue *ig, int *count) {
  return NULL;
}

undef_int
i_writeavif(i_img *im, io_glue *ig) {
  return i_writeavif_multi(ig, &im, 1);
}

undef_int
i_writeavif_multi(io_glue *ig, i_img **imgs, int count) {
  dIMCTXim(imgs[0]);
  avifEncoder *avif = NULL;
  avifRWData out = AVIF_DATA_EMPTY;
  int i;
  avifResult result;
  avifImage **saved_images = NULL;
  int saved_image_count = 0;

  im_clear_error(aIMCTX);

  saved_images = mymalloc(sizeof(avifImage*) * count);

  avif = avifEncoderCreate(); /* aborts on failure */

  for (i = 0; i < count; ++i) {
    i_img *im = imgs[i];
    avifRGBImage rgb;
    avifImage *avif_image = NULL;
    
    memset(&rgb, 0, sizeof(rgb));
    
    /* FIXME: support 16-bit depths too, that may wait until
       i_img_data() support in Imager.
    */
    int depth = 8;

    switch (im->channels) {
    case 0: /* Coming Soon(tm) */
      im_push_errorf(aIMCTX, 0, "Image %d has no color channels", i);
      goto fail;
      
    case 1:
    case 2:
      /* FIXME: implement these */
      im_push_errorf(aIMCTX, 0, "%d channel images not implemented", im->channels);
      goto fail;
      
    case 3:
    case 4:
      {
        unsigned char *pdata;
        i_img_dim y;
        avif_image = avifImageCreate(im->xsize, im->ysize, depth, AVIF_PIXEL_FORMAT_YUV444);
        avifRGBImageSetDefaults(&rgb, avif_image);
        rgb.format = im->channels == 3 ? AVIF_RGB_FORMAT_RGB : AVIF_RGB_FORMAT_RGBA;
        avifRGBImageAllocatePixels(&rgb);
        for (y = 0, pdata = rgb.pixels; y < im->ysize; ++y, pdata += rgb.rowBytes) {
          i_gsamp(im, 0, im->xsize, y, pdata, NULL, im->channels);
        }
        avif_image->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_SRGB;
        result = avifImageRGBToYUV(avif_image, &rgb);
        if (result != AVIF_RESULT_OK) {
          im_push_errorf(aIMCTX, result, "Failed to convert RGBA to YUVA: %s", avifResultToString(result));
        failimage:
          if (rgb.pixels) {
            avifRGBImageFreePixels(&rgb);
          }
          avifImageDestroy(avif_image);
          goto fail;
        }
        result = avifEncoderAddImage(avif, avif_image, 1, count == 1 ? AVIF_ADD_IMAGE_FLAG_SINGLE : 0);
        if (result != AVIF_RESULT_OK) {
          im_push_errorf(aIMCTX, result, "Failed to add image to encoder: %s", avifResultToString(result));
          goto failimage;
        }
        saved_images[saved_image_count++] = avif_image;
        break;
      }
    }
    
  }

  result = avifEncoderFinish(avif, &out);
  if (result != AVIF_RESULT_OK) {
    im_push_errorf(aIMCTX, result, "encoder finish failed: %s", avifResultToString(result));
    goto fail;
  }  

  if (i_io_write(ig, out.data, out.size) != out.size) {
    im_push_error(aIMCTX, errno, "Failed to write to image");
    goto fail;
  }
  if (i_io_close(ig))
    goto fail;
  while (saved_image_count > 0) {
    avifImageDestroy(saved_images[--saved_image_count]);
  }
  myfree(saved_images);
  avifRWDataFree(&out);
  avifEncoderDestroy(avif);
  return 1;

 fail:
  while (saved_image_count > 0) {
    avifImageDestroy(saved_images[--saved_image_count]);
  }
  avifRWDataFree(&out);
  avifEncoderDestroy(avif);
  return 0;
}

#define _str(x) #x
#define str(x) _str(x)

static const char
build_ver[] =
  "" str(AVIF_VERSION_MAJOR) "." str(AVIF_VERSION_MINOR) "." str(AVIF_VERSION_PATCH) "." str(AVIF_VERSION_DEVEL);

char const *
i_avif_buildversion(void) {
  return build_ver;
}

char const *
i_avif_libversion(void) {
  return avifVersion();
}

static char codecs[256];

char const *
i_avif_codecs(void) {
  if (!*codecs) {
    avifCodecVersions(codecs);
  }

  return codecs;
}
