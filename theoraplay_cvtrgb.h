/**
 * TheoraPlay; multithreaded Ogg Theora/Ogg Vorbis decoding.
 *
 * Please see the file LICENSE_THEORAPLAY.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#if !THEORAPLAY_INTERNAL
#error Do not include this in your app. It is used internally by TheoraPlay.
#endif

static unsigned char *THEORAPLAY_CVT_FNNAME_420(const th_info *tinfo,
                                                const th_ycbcr_buffer ycbcr)
{
    const int w = tinfo->pic_width;
    const int h = tinfo->pic_height;
    unsigned char *pixels = (unsigned char *) malloc(w * h * 4);
    if (pixels)
    {
        unsigned char *dst = pixels;
        const int ystride = ycbcr[0].stride;
        const int cbstride = ycbcr[1].stride;
        const int crstride = ycbcr[2].stride;
        const int yoff = (tinfo->pic_x & ~1) + ystride * (tinfo->pic_y & ~1);
        const int cboff = (tinfo->pic_x / 2) + (cbstride) * (tinfo->pic_y / 2);
        const unsigned char *py = ycbcr[0].data + yoff;
        const unsigned char *pcb = ycbcr[1].data + cboff;
        const unsigned char *pcr = ycbcr[2].data + cboff;
        int posx, posy;

        for (posy = 0; posy < h; posy++)
        {
            for (posx = 0; posx < w; posx++)
            {
                // http://www.theora.org/doc/Theora.pdf, 1.1 spec,
                //  chapter 4.2 (Y'CbCr -> Y'PbPr -> R'G'B')
                // These constants apparently work for NTSC _and_ PAL/SECAM.
                const float yoffset = 16.0f;
                const float yexcursion = 219.0f;
                const float cboffset = 128.0f;
                const float cbexcursion = 224.0f;
                const float croffset = 128.0f;
                const float crexcursion = 224.0f;
                const float kr = 0.299f;
                const float kb = 0.114f;

                const float y = (((float) py[posx]) - yoffset) / yexcursion;
                const float pb = (((float) pcb[posx / 2]) - cboffset) / cbexcursion;
                const float pr = (((float) pcr[posx / 2]) - croffset) / crexcursion;
                const float r = (y + (2.0f * (1.0f - kr) * pr)) * 255.0f;
                const float g = (y - ((2.0f * (((1.0f - kb) * kb) / ((1.0f - kb) - kr))) * pb) - ((2.0f * (((1.0f - kr) * kr) / ((1.0f - kb) - kr))) * pr)) * 255.0f;
                const float b = (y + (2.0f * (1.0f - kb) * pb)) * 255.0f;

                *(dst++) = (unsigned char) ((r < 0.0f) ? 0.0f : (r > 255.0f) ? 255.0f : r);
                *(dst++) = (unsigned char) ((g < 0.0f) ? 0.0f : (g > 255.0f) ? 255.0f : g);
                *(dst++) = (unsigned char) ((b < 0.0f) ? 0.0f : (b > 255.0f) ? 255.0f : b);
                #if THEORAPLAY_CVT_RGB_ALPHA
                *(dst++) = 0xFF;
                #endif
            } // for

            // adjust to the start of the next line.
            py += ystride;
            pcb += cbstride * (posy % 2);
            pcr += crstride * (posy % 2);
        } // for
    } // if

    return pixels;
} // THEORAPLAY_CVT_FNNAME_420

// end of theoraplay_cvtrgb.h ...

