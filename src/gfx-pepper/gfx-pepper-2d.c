/*
  * UAE - The Un*x Amiga Emulator
  *
  * Pepper 2D graphics to be used for Native Client builds.
  *
  * Copyright 2013 Christian Stefansen
  *
  */

#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_graphics_2d.h"
#include "ppapi/c/ppb_image_data.h"

/* guidep == gui-html is currently the only way to build with Pepper. */
#include "guidep/ppapi.h"
#include "options.h"
#include "xwin.h"

#include <stdio.h> // cstef*****
#include <string.h> // cstef memcpy
#ifndef DEBUG_LOG
#ifdef DEBUG
#define DEBUG_LOG write_log
#else
#define DEBUG_LOG(...) do ; while(0)
#endif
#endif /* DEBUG_LOG */

static PPB_Core *ppb_core_interface;
static PPB_Graphics2D *ppb_g2d_interface;
static PPB_ImageData *ppb_image_data_interface;

static PP_ImageDataFormat preferredFormat;
static PP_Instance pp_instance;
static PP_Resource image_data;
static PP_Resource graphics_context;
static struct PP_Size canvasSize;

STATIC_INLINE void pepper_graphics2d_flush_screen(struct vidbuf_description *gfxinfo,
                         int first_line, int last_line) {
    /* Draw canvas. */
    //printf("Lines %d %d\n", first_line, last_line);

    // cstef-- option 1: use paintimagedata only for first_line->last


    //cstef -- option 2: memcpy *all* to image_data and do replacecontent
    memcpy(ppb_image_data_interface->Map(image_data),
           gfxinfo->bufmem, gfxinfo->rowbytes * gfxinfo->height);
    ppb_image_data_interface->Unmap(image_data);
    ppb_g2d_interface->ReplaceContents(graphics_context, image_data);
    ppb_g2d_interface->Flush(graphics_context, PP_BlockUntilComplete());
    ppb_core_interface->ReleaseResource(image_data); //cstef: before flush?

    image_data = ppb_image_data_interface->Create(pp_instance, preferredFormat,
            &canvasSize, /* init_to_zero = */ PP_FALSE);
    if (!image_data) {
        DEBUG_LOG("Could not create image data.\n");
    }
    //      gfxinfo->bufmem = (uae_u8 *) ppb_image_data_interface->Map(image_data);
    //     gfxvidinfo.bufmem = gfxinfo->bufmem; cstef
    //init_row_map ();
}

int graphics_2d_subinit(uint32_t *Rmask, uint32_t *Gmask, uint32_t *Bmask,
                        uint32_t *Amask) {
    /* Pepper Graphics2D setup. */
    PPB_Instance *ppb_instance_interface = (PPB_Instance *) NaCl_GetInterface(PPB_INSTANCE_INTERFACE);
    if (!ppb_instance_interface) {
        DEBUG_LOG("Could not acquire PPB_Instance interface.\n");
        return 0;
    }
    ppb_g2d_interface = (PPB_Graphics2D *) NaCl_GetInterface(PPB_GRAPHICS_2D_INTERFACE);
    if (!ppb_g2d_interface) {
        DEBUG_LOG("Could not acquire PPB_Graphics2D interface.\n");
        return 0;
    }
    ppb_core_interface = (PPB_Core *) NaCl_GetInterface(PPB_CORE_INTERFACE);
    if (!ppb_core_interface) {
        DEBUG_LOG("Could not acquire PPB_Core interface.\n");
        return 0;
    }
    ppb_image_data_interface = (PPB_ImageData *) NaCl_GetInterface(PPB_IMAGEDATA_INTERFACE);
    if (!ppb_image_data_interface) {
        DEBUG_LOG("Could not acquire PPB_ImageData interface.\n");
        return 0;
    }
    pp_instance = NaCl_GetInstance();
    if (!pp_instance) {
        DEBUG_LOG("Could not find current Pepper instance.\n");
        return 0;
    }

    canvasSize.width = gfxvidinfo.width;
    canvasSize.height = gfxvidinfo.height;
    graphics_context = ppb_g2d_interface->Create(
            pp_instance,
            &canvasSize,
            PP_TRUE /* is_always_opaque */);
    if (!graphics_context) {
        DEBUG_LOG("Could not obtain a PPB_Graphics2D context.\n");
        return 0;
    }
    if (!ppb_instance_interface->BindGraphics(pp_instance, graphics_context)) {
        DEBUG_LOG("Failed to bind context to instance.\n");
        return 0;
    }

    preferredFormat = ppb_image_data_interface->GetNativeImageDataFormat();
    switch (preferredFormat) {
    case PP_IMAGEDATAFORMAT_BGRA_PREMUL:
        *Rmask = 0x00FF0000, *Gmask = 0x0000FF00, *Bmask = 0x000000FF;
        *Amask = 0xFF000000;
        break;
    case PP_IMAGEDATAFORMAT_RGBA_PREMUL:
        *Rmask = 0x000000FF, *Gmask = 0x0000FF00, *Bmask = 0x00FF0000;
        *Amask = 0xFF000000;
        break;
    default:
        DEBUG_LOG("Unrecognized preferred image data format: %d.\n",
                  preferredFormat);
        return 0;
    }
    image_data = ppb_image_data_interface->Create(pp_instance, preferredFormat,
            &canvasSize, /* init_to_zero = */ PP_FALSE);
     if (!image_data) {
         DEBUG_LOG("Could not create image data.\n");
         return 0;
     }

    /* UAE gfxvidinfo setup. */
    /* TODO(cstefansen): Implement double-buffering if this is too slow. */
    gfxvidinfo.pixbytes = 4; /* 32-bit graphics */
    gfxvidinfo.rowbytes = gfxvidinfo.width * gfxvidinfo.pixbytes;
    gfxvidinfo.bufmem = (uae_u8 *) calloc(gfxvidinfo.rowbytes, gfxvidinfo.height);
//cstef    gfxvidinfo.bufmem = (uae_u8 *) ppb_image_data_interface->Map(image_data);
    gfxvidinfo.emergmem = 0;
    gfxvidinfo.flush_screen = pepper_graphics2d_flush_screen;

    /* Draw canvas. */
//    ppb_image_data_interface->Unmap(image_data);
//     ppb_g2d_interface->ReplaceContents(graphics_context, image_data);
//     ppb_g2d_interface->Flush(graphics_context, PP_BlockUntilComplete());
//     ppb_core_interface->ReleaseResource(image_data);
//
//      image_data = ppb_image_data_interface->Create(pp_instance, preferredFormat,
//              &canvasSize, /* init_to_zero = */ PP_FALSE);
//      if (!image_data || !image_data) {
//          DEBUG_LOG("Could not create image data.\n");
//      }
//      gfxvidinfo.bufmem = (uae_u8 *) ppb_image_data_interface->Map(image_data);

     return 1;
}
