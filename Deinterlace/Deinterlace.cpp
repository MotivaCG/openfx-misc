/*
 OFX Deinterlace plugin.

 Copyright (C) 2014 INRIA

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice, this
 list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 Neither the name of the {organization} nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 INRIA
 Domaine de Voluceau
 Rocquencourt - B.P. 105
 78153 Le Chesnay Cedex - France

 The skeleton for this source file is from:
 OFX Genereator example plugin, a plugin that illustrates the use of the OFX Support library.

 Copyright (C) 2004-2005 The Open Effects Association Ltd
 Author Bruno Nicoletti bruno@thefoundry.co.uk

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 * Neither the name The Open Effects Association Ltd, nor the names of its
 contributors may be used to endorse or promote products derived from this
 software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 The Open Effects Association Ltd
 1 Wardour St
 London W1D 6PA
 England
 
 
 */

/*
 TODO:
 implement most methods.
 */

#include <cstring>
#include <cmath>
#include <algorithm>
#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsProcessing.H"
#include "ofxsMacros.h"

#define kPluginName          "DeinterlaceOFX"
#define kPluginGrouping      "Time"
#define kPluginDescription \
"Deinterlace input stream.\n" \
"The following deinterlacing algorithms are supported:\n" \
"- Weave: This is what 100fps.com calls \"do nothing\". Other names: \"disabled\" or \"no deinterlacing\". Should be used for PsF content.\n" \
"- Blend: Blender (full resolution). Each line of the picture is created as the average of a line from the odd and a line from the even half-pictures. This ignores the fact that they are supposed to be displayed at different times.\n" \
"- Bob: Doubler. Display each half-picture like a full picture, by simply displaying each line twice. Preserves temporal resolution of interlaced video.\n" \
"- Discard: Only display one of the half-pictures, discard the other. Other name: \"single field\". Both temporal and vertical spatial resolutions are halved. Can be used for slower computers or to give interlaced video movie-like look with characteristic judder.\n" \
"- Linear: Doubler. Bob with linear interpolation: instead of displaying each line twice, line 2 is created as the average of line 1 and 3, etc.\n" \
"- Mean: Blender (half resolution). Display a half-picture that is created as the average of the two original half-pictures.\n" \
"- Yadif: Interpolator (Yet Another DeInterlacing Filter) from MPlayer by Michael Niedermayer (http://www.mplayerhq.hu). It checks pixels of previous, current and next frames to re-create the missed field by some local adaptive method (edge-directed interpolation) and uses spatial check to prevent most artifacts.\n" \
"- W3F: Martin Weston three field deinterlace, as described in BBC patent <https://www.google.com/patents/US4789893>. These filters use three consecutive fields to compute each interpolated line. The simple filter is described in Fig. 3, and the complex filter is described in Fig. 5 in the patent."

#define kPluginIdentifier    "net.sf.openfx.Deinterlace"
#define kPluginVersionMajor 1 // Incrementing this number means that you have broken backwards compatibility of the plug-in.
#define kPluginVersionMinor 0 // Increment this when you have fixed a bug or made it faster.

#define kSupportsTiles 0
#define kSupportsMultiResolution 0
#define kSupportsRenderScale 0 // are images still fielded at any renderscale?
#define kRenderThreadSafety eRenderFullySafe

#define kParamMode "mode"
#define kParamModeLabel "Deinterlacing Mode"
#define kParamModeHint "Choice of the deinterlacing mode/algorithm"
#define kParamModeOptionWeave "Weave"
#define kParamModeOptionWeaveHint "This is what 100fps.com calls \"do nothing\". Other names: \"disabled\" or \"no deinterlacing\". Should be used for PsF content."
#define kParamModeOptionBlend "Blend"
#define kParamModeOptionBlendHint "Blender (full resolution). Each line of the picture is created as the average of a line from the odd and a line from the even half-pictures. This ignores the fact that they are supposed to be displayed at different times."
#define kParamModeOptionBob "Bob"
#define kParamModeOptionBobHint "Doubler. Display each half-picture like a full picture, by simply displaying each line twice. Preserves temporal resolution of interlaced video."
#define kParamModeOptionDiscard "Discard"
#define kParamModeOptionDiscardHint "Only display one of the half-pictures, discard the other. Other name: \"single field\". Both temporal and vertical spatial resolutions are halved. Can be used for slower computers or to give interlaced video movie-like look with characteristic judder."
#define kParamModeOptionLinear "Linear"
#define kParamModeOptionLinearHint "Doubler. Bob with linear interpolation: instead of displaying each line twice, line 2 is created as the average of line 1 and 3, etc."
#define kParamModeOptionMean "Mean"
#define kParamModeOptionMeanHint "Blender (half resolution). Display a half-picture that is created as the average of the two original half-pictures."
#define kParamModeOptionYadif "Yadif"
#define kParamModeOptionYadifHint "Interpolator (Yet Another DeInterlacing Filter) from MPlayer by Michael Niedermayer <http://www.mplayerhq.hu>. It checks pixels of previous, current and next frames to re-create the missed field by some local adaptive method (edge-directed interpolation) and uses spatial check to prevent most artifacts."
#define kParamModeOptionW3F "W3F"
#define kParamModeOptionW3FHint "Martin Weston three field deinterlace, as described in BBC patent <https://www.google.com/patents/US4789893>. These filters use three consecutive fields to compute each interpolated line. The simple filter is described in Fig. 3 in the patent. The complex filter is described in Fig. 5 in the patent."

enum DeinterlaceModeEnum {
    eDeinterlaceModeWeave,
    eDeinterlaceModeBlend,
    eDeinterlaceModeBob,
    eDeinterlaceModeDiscard,
    eDeinterlaceModeLinear,
    eDeinterlaceModeMean,
    eDeinterlaceModeYadif,
    eDeinterlaceModeW3F,
};

#define kParamFieldOrder "fieldOrder"
#define kParamFieldOrderLabel "Field Order"
#define kParamFieldOrderHint "Interlaced field order"
#define kParamFieldOrderOptionLower "Lower field first"
#define kParamFieldOrderOptionLowerHint "Lower/bottom field first"
#define kParamFieldOrderOptionUpper "Upper field first"
#define kParamFieldOrderOptionUpperHint "Upper/top field first"
#define kParamFieldOrderOptionAuto "Auto"
#define kParamFieldOrderOptionAutoHint "HD=upper,SD=lower"

enum FieldOrderEnum {
    eFieldOrderLower,
    eFieldOrderUpper,
    eFieldOrderAuto,
};

#define kParamDoubleFramerate "doubleFramerate"
#define kParamDoubleFramerateLabel "Double Framerate"
#define kParamDoubleFramerateHint "Each input frame produces two output frames, and the framerate is doubled."

#define kParamYadifMode "yadifMode"
#define kParamYadifModeLabel "Yadif Processing Mode"
#define kParamYadifModeHint "Mode of checking fields"
#define kParamYadifModeOptionTemporalSpatial "Temporal & spatial"
#define kParamYadifModeOptionTemporalSpatialHint "temporal and spatial interlacing check (default)."
#define kParamYadifModeOptionTemporal "Temporal only"
#define kParamYadifModeOptionTemporalHint "skips spatial interlacing check."

enum YadifModeEnum {
    eYadifModeTemporalSpatial,
    eYadifModeTemporal,
};

#define kParamW3FFilter "w3fFilter"
#define kParamW3FFilterLabel "W3F Filter"
#define kParamW3FFilterHint "Specify the filter. "
#define kParamW3FFilterOptionSimple "Simple"
#define kParamW3FFilterOptionSimpleHint "The simple filter is described in Fig. 3 in the patent."
#define kParamW3FFilterOptionComplex "Complex"
#define kParamW3FFilterOptionComplexHint "The complex filter is described in Fig. 5 in the patent."

enum W3FFilterEnum {
    eW3FFilterSimple,
    eW3FFilterComplex,
};

class DeinterlacePlugin : public OFX::ImageEffect
{
public:
    DeinterlacePlugin(OfxImageEffectHandle handle) : ImageEffect(handle), dstClip_(0), srcClip_(0)
    {
        dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
        srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);

        _mode = fetchChoiceParam(kParamMode);
        _fieldOrder = fetchChoiceParam(kParamFieldOrder);
        _yadifMode = fetchChoiceParam(kParamYadifMode);
        _w3fFilter = fetchChoiceParam(kParamW3FFilter);
        _doubleFramerate = fetchBooleanParam(kParamDoubleFramerate);
        assert(_mode && _fieldOrder && _yadifMode && _w3fFilter && _doubleFramerate);
    }

private:
    virtual void render(const OFX::RenderArguments &args) OVERRIDE FINAL;

    /** @brief get the clip preferences */
    virtual void getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences) OVERRIDE FINAL;

    // override the rod call
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args, OfxRectD &rod) OVERRIDE FINAL;

    /* override is identity */
    virtual bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime) OVERRIDE FINAL;

    virtual bool getTimeDomain(OfxRangeD &range) OVERRIDE FINAL;

private:
    // do not need to delete these, the ImageEffect is managing them for us
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;

    OFX::ChoiceParam *_mode;
    OFX::ChoiceParam *_fieldOrder;
    OFX::ChoiceParam *_yadifMode;
    OFX::ChoiceParam *_w3fFilter;
    OFX::BooleanParam *_doubleFramerate;

};



// =========== GNU Lesser General Public License code start =================
namespace Yadif {
// Yadif (yet another deinterlacing filter)
// http://avisynth.org.ru/yadif/yadif.html
// http://mplayerhq.hu

// Original port to OFX/Vegas by George Yohng http://yohng.com
// Rewritten after yadif relicensing to LGPL:
// http://git.videolan.org/?p=ffmpeg.git;a=commit;h=194ef56ba7e659196fe554782d797b1b45c3915f


// Copyright notice and licence from the original libavfilter/vf_yadif.c file:
/*
 * Copyright (C) 2006-2011 Michael Niedermayer <michaelni@gmx.at>
 *               2010      James Darnley <james.darnley@gmail.com>
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

//#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
//#define FFMAX(a,b) ((a) < (b) ? (b) : (a))
//#define FFABS(a) ((a) > 0 ? (a) : (-(a)))
#define FFMIN(a,b) std::min(a,b)
#define FFMAX(a,b) std::max(a,b)
#define FFABS(a) std::abs(a)

#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)

inline float halven(float f) { return f*0.5f; }
inline int halven(int i) { return i>>1; }

inline int one1(unsigned char *) { return 1; }
inline int one1(unsigned short *) { return 1; }
inline float one1(float *) { return 0.; }

#define CHECK(j)\
    {   Diff score = FFABS(cur[mrefs - ch + (j)] - cur[prefs - ch - (j)])\
                 + FFABS(cur[mrefs   +(j)] - cur[prefs   -(j)])\
                 + FFABS(cur[mrefs + ch + (j)] - cur[prefs + ch - (j)]);\
        if(score < spatial_score){\
            spatial_score= score;\
            spatial_pred= halven(cur[mrefs  +(j)] + cur[prefs  -(j)]);\

/* The is_not_edge argument here controls when the code will enter a branch
 * which reads up to and including x-3 and x+3. */

#define FILTER(start, end, is_not_edge) \
    for (x = start;  x < end; x++) { \
        Diff c= cur[mrefs]; \
        Diff d= halven(prev2[0] + next2[0]); \
        Diff e= cur[prefs]; \
        Diff temporal_diff0= FFABS(prev2[0] - next2[0]); \
        Diff temporal_diff1=halven( FFABS(prev[mrefs] - c) + FFABS(prev[prefs] - e) ); \
        Diff temporal_diff2=halven( FFABS(next[mrefs] - c) + FFABS(next[prefs] - e) ); \
        Diff diff= FFMAX3(halven(temporal_diff0), temporal_diff1, temporal_diff2); \
        Diff spatial_pred= halven(c+e); \
 \
        if (is_not_edge) {\
            Diff spatial_score = FFABS(cur[mrefs-ch] - cur[prefs-ch]) + FFABS(c-e) \
                              + FFABS(cur[mrefs+ch] - cur[prefs+ch]) - one1((Comp*)0); \
            CHECK(-ch) CHECK(-(ch*2)) }} }} \
            CHECK( ch) CHECK( (ch*2)) }} }} \
        }\
 \
        if (!(mode&2)) { \
            Diff b = halven(prev2[2 * mrefs] + next2[2 * mrefs]); \
            Diff f = halven(prev2[2 * prefs] + next2[2 * prefs]); \
            Diff max = FFMAX3(d - e, d - c, FFMIN(b - c, f - e)); \
            Diff min = FFMIN3(d - e, d - c, FFMAX(b - c, f - e)); \
 \
            diff = FFMAX3(diff, min, -max); \
        } \
 \
        if (spatial_pred > d + diff) \
           spatial_pred = d + diff; \
        else if (spatial_pred < d - diff) \
           spatial_pred = d - diff; \
 \
        dst[0] = (Comp)spatial_pred; \
 \
        dst += ch; \
        cur += ch; \
        prev += ch; \
        next += ch; \
        prev2 += ch; \
        next2 += ch; \
    }

template<int ch,typename Comp,typename Diff>
inline void filter_line_c(Comp *dst1,
                          const Comp *prev1, const Comp *cur1, const Comp *next1,
                          int w, int prefs, int mrefs, int parity, int mode)
{
    Comp *dst  = dst1;
    const Comp *prev = prev1;
    const Comp *cur  = cur1;
    const Comp *next = next1;
    int x;
    const Comp *prev2 = parity ? prev : cur ;
    const Comp *next2 = parity ? cur  : next;

    /* The function is called with the pointers already pointing to data[3] and
     * with 6 subtracted from the width.  This allows the FILTER macro to be
     * called so that it processes all the pixels normally.  A constant value of
     * true for is_not_edge lets the compiler ignore the if statement. */
    FILTER(0, w, 1)
}

#if 0 // filter_edges is only necessary if we are using the assembly version of filter_line
#define MAX_ALIGN 8
template<int ch,typename Comp,typename Diff>
inline void filter_edges(Comp *dst1,
                         const Comp *prev1, const Comp *cur1, const Comp *next1,
                         int w, int prefs, int mrefs, int parity, int mode)
{
    Comp *dst  = dst1;
    const Comp *prev = prev1;
    const Comp *cur  = cur1;
    const Comp *next = next1;
    int x;
    const Comp *prev2 = parity ? prev : cur ;
    const Comp *next2 = parity ? cur  : next;

    /* Only edge pixels need to be processed here.  A constant value of false
     * for is_not_edge should let the compiler ignore the whole branch. */
    FILTER(0, 3, 0)

    dst  = dst1  + (w - (MAX_ALIGN/sizeof(Comp)-1))*ch;
    prev = prev1 + (w - (MAX_ALIGN/sizeof(Comp)-1))*ch;
    cur  = cur1  + (w - (MAX_ALIGN/sizeof(Comp)-1))*ch;
    next = next1 + (w - (MAX_ALIGN/sizeof(Comp)-1))*ch;
    prev2 = parity ? prev : cur;
    next2 = parity ? cur  : next;

    FILTER(w - (MAX_ALIGN/sizeof(Comp)-1), w - 3, 1)
    FILTER(w - 3, w, 0)
}
#endif // 0

inline void interpolate(unsigned char *dst, const unsigned char *cur0,  const unsigned char *cur2, int w)
{
    int x;
    for (x=0; x<w; x++) {
        dst[x] = (cur0[x] + cur2[x] + 1)>>1; // simple average
    }
}

inline void interpolate(float *dst, const float *cur0,  const float *cur2, int w)
{
    int x;
    for (x=0; x<w; x++) {
        dst[x] = (cur0[x] + cur2[x] )*0.5f; // simple average
    }
}


template<int ch,typename Comp,typename Diff>
static void filter_plane(int mode, Comp *dst, int dst_stride,
                         const Comp *prev0, const Comp *cur0, const Comp *next0,
                         int refs, int w, int h, int parity, int tff)
{
    for (int y = 0; y < h; ++y) {
        if (((y ^ parity) & 1)) {
            const Comp *prev= prev0 + y*refs;
            const Comp *cur = cur0 + y*refs;
            const Comp *next= next0 + y*refs;
            Comp *dst2= dst + y*dst_stride;

            for (int c = 0; c < ch; ++c) {
                filter_line_c<ch,Comp,Diff>(dst2 + c, prev + c, cur + c, next + c, w,
                                            y + 1 < h ? refs : -refs,
                                            y ? -refs : refs,
                                            parity ^ tff, mode);
            }
        } else {
            memcpy(&dst[y * dst_stride],
                   &cur0[y * refs], w * ch * sizeof(Comp)); // copy original
        }
    }
    
}

template<int ch,typename Comp,typename Diff>
static void filter_plane_ofx(int mode,
                             OFX::Image *dst_,
                             const OFX::Image *srcp,
                             const OFX::Image *src,
                             const OFX::Image *srcn,
                             int parity, int tff)
{
    Comp *dst = (Comp*)dst_->getPixelData(); // change this when we support renderWindow
    int dst_stride = dst_->getRowBytes() / sizeof(Comp);
    const Comp *prev0 = (const Comp*)(srcp ? srcp->getPixelData() : src->getPixelData());
    const Comp *cur0 = (const Comp*)src->getPixelData();
    const Comp *next0 = (const Comp*)(srcn ? srcn->getPixelData() : src->getPixelData());
    int refs = src->getRowBytes() / sizeof(Comp);
    const OfxRectI bounds = dst_->getBounds();
    filter_plane<ch, Comp, Diff>(mode, dst, dst_stride,
                                 prev0, cur0, next0,
                                 refs,
                                 bounds.x2 - bounds.x1, bounds.y2 - bounds.y1,
                                 parity, tff);
}

// =========== GNU Lesser General Public License code end =================

void deinterlace(const OFX::Image* srcp, const OFX::Image* src, const OFX::Image* srcn,
                 FieldOrderEnum fieldOrder, int field,
                 YadifModeEnum yadifMode,
                 OFX::Image* dst)
{
    int imode;
    switch (yadifMode) {
        case eYadifModeTemporalSpatial:
            imode = 0;
            break;
        case eYadifModeTemporal:
            imode = 2;
    }

    OFX::PixelComponentEnum dstComponents  = dst->getPixelComponents();
    OFX::BitDepthEnum       dstBitDepth    = dst->getPixelDepth();
    const OfxRectI rect = dst->getBounds();

    int width = rect.x2 - rect.x1;
    int height = rect.y2 - rect.y1;

    if (width < 3 || height < 3) {
        // Video of less than 3 columns or lines is not supported
        // just copy sc to dst
        for (int y=0; y<height; y++) {
            memcpy(dst->getPixelAddress(0,y),src->getPixelAddress(0,y),abs(src->getRowBytes()));
        }
    } else {
        int ifieldOrder;
        switch(fieldOrder) {
            case eFieldOrderLower:
                ifieldOrder = 0;
                break;
            case eFieldOrderUpper:
                ifieldOrder = 1;
                break;
            case eFieldOrderAuto:
                if (width > 1024) {
                    ifieldOrder = 1;
                } else {
                    ifieldOrder = 0;
                }
                break;
        }
        int parity = ifieldOrder ^ !field; // automatic parity setting

        if (dstComponents == OFX::ePixelComponentRGBA) {
            switch(dstBitDepth) {
                case OFX::eBitDepthUByte:
                    Yadif::filter_plane_ofx<4,unsigned char,int>(imode, // mode
                                                                 dst,
                                                                 srcp, src, srcn,
                                                                 parity, ifieldOrder); // parity, tff
                    break;

                case OFX::eBitDepthUShort:
                    Yadif::filter_plane_ofx<4,unsigned short,int>(imode, // mode
                                                                  dst,
                                                                  srcp, src, srcn,
                                                                  parity, ifieldOrder); // parity, tff
                    break;

                case OFX::eBitDepthFloat:
                    Yadif::filter_plane_ofx<4,float,float>(imode, // mode
                                                           dst,
                                                           srcp, src, srcn,
                                                           parity, ifieldOrder); // parity, tff
                    break;

                default:
                    break;
            }
        } else if (dstComponents == OFX::ePixelComponentRGB) {
            switch(dstBitDepth) {
                case OFX::eBitDepthUByte:
                    Yadif::filter_plane_ofx<3,unsigned char,int>(imode, // mode
                                                                 dst,
                                                                 srcp, src, srcn,
                                                                 parity, ifieldOrder); // parity, tff
                    break;

                case OFX::eBitDepthUShort:
                    Yadif::filter_plane_ofx<3,unsigned short,int>(imode, // mode
                                                                  dst,
                                                                  srcp, src, srcn,
                                                                  parity, ifieldOrder); // parity, tff
                    break;

                case OFX::eBitDepthFloat:
                    Yadif::filter_plane_ofx<3,float,float>(imode, // mode
                                                           dst,
                                                           srcp, src, srcn,
                                                           parity, ifieldOrder); // parity, tff
                    break;

                default:
                    break;
            }
        } else if (dstComponents == OFX::ePixelComponentAlpha) {
            switch(dstBitDepth) {
                case OFX::eBitDepthUByte:
                    Yadif::filter_plane_ofx<1,unsigned char,int>(imode, // mode
                                                                 dst,
                                                                 srcp, src, srcn,
                                                                 parity, ifieldOrder); // parity, tff
                    break;

                case OFX::eBitDepthUShort:
                    Yadif::filter_plane_ofx<1,unsigned short,int>(imode, // mode
                                                                  dst,
                                                                  srcp, src, srcn,
                                                                  parity, ifieldOrder); // parity, tff
                    break;

                case OFX::eBitDepthFloat:
                    Yadif::filter_plane_ofx<1,float,float>(imode, // mode
                                                           dst,
                                                           srcp, src, srcn,
                                                           parity, ifieldOrder); // parity, tff
                    break;

                default:
                    break;
            }
        }
    }
}

} // namespace Yadif

void DeinterlacePlugin::render(const OFX::RenderArguments &args)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    const double dstTime = args.time;

    bool doubleFramerate;
    _doubleFramerate->getValueAtTime(dstTime, doubleFramerate);

    const double srcTime = doubleFramerate ? std::floor(dstTime / 2. + 0.5) : dstTime;

    std::auto_ptr<OFX::Image> dst(dstClip_->fetchImage(dstTime));
    std::auto_ptr<const OFX::Image> src(srcClip_->fetchImage(srcTime));
    if (!src.get() || !dst.get()) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    const OfxRectI& srcBounds = src->getBounds();
    const OfxRectI& srcRod = src->getRegionOfDefinition();
    const OfxRectI& dstBounds = dst->getBounds();
    const OfxRectI& dstRod= dst->getRegionOfDefinition();

    if (!kSupportsTiles) {
        // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsTiles
        //  If a clip or plugin does not support tiled images, then the host should supply full RoD images to the effect whenever it fetches one.
        assert(srcRod.x1 == srcBounds.x1);
        assert(srcRod.x2 == srcBounds.x2);
        assert(srcRod.y1 == srcBounds.y1);
        assert(srcRod.y2 == srcBounds.y2); // crashes on Natron if kSupportsTiles=0 & kSupportsMultiResolution=1
        assert(dstRod.x1 == dstBounds.x1);
        assert(dstRod.x2 == dstBounds.x2);
        assert(dstRod.y1 == dstBounds.y1);
        assert(dstRod.y2 == dstBounds.y2); // crashes on Natron if kSupportsTiles=0 & kSupportsMultiResolution=1
    }
    if (!kSupportsMultiResolution) {
        // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
        //   Multiple resolution images mean...
        //    input and output images can be of any size
        //    input and output images can be offset from the origin
        assert(srcRod.x1 == 0);
        assert(srcRod.y1 == 0);
        assert(srcRod.x1 == dstRod.x1);
        assert(srcRod.x2 == dstRod.x2);
        assert(srcRod.y1 == dstRod.y1);
        assert(srcRod.y2 == dstRod.y2); // crashes on Natron if kSupportsMultiResolution=0
    }


    int mode_i;
    int fieldOrder_i;

    _mode->getValueAtTime(dstTime, mode_i);
    DeinterlaceModeEnum mode = (DeinterlaceModeEnum)mode_i;
    _fieldOrder->getValueAtTime(dstTime, fieldOrder_i);
    FieldOrderEnum fieldOrder = (FieldOrderEnum)fieldOrder_i;

    int field = doubleFramerate ? ((int)dstTime & 1) : 0;

    switch (mode) {
        case eDeinterlaceModeYadif: {
            std::auto_ptr<OFX::Image> srcp(srcClip_->fetchImage(srcTime-1.0));
            std::auto_ptr<OFX::Image> srcn(srcClip_->fetchImage(srcTime+1.0));

            if (!srcp.get() || !srcn.get()) {
                OFX::throwSuiteStatusException(kOfxStatFailed);
            }

            int yadifMode_i;
            _yadifMode->getValueAtTime(dstTime, yadifMode_i);
            YadifModeEnum yadifMode = (YadifModeEnum)yadifMode_i;

            Yadif::deinterlace(srcp.get(), src.get(), srcn.get(),
                               fieldOrder, field,
                               yadifMode,
                               dst.get());
        }
            break;
        default:
            OFX::throwSuiteStatusException(kOfxStatFailed);
    }
}

/* Override the clip preferences */
void
DeinterlacePlugin::getClipPreferences(OFX::ClipPreferencesSetter &clipPreferences)
{
    // set the fielding of dstClip_
    clipPreferences.setOutputFielding(OFX::eFieldNone);
    bool doubleFramerate;
    _doubleFramerate->getValue(doubleFramerate);
    if (doubleFramerate) {
        double frameRate = srcClip_->getFrameRate();
        clipPreferences.setOutputFrameRate(2 * frameRate);
    }
}

bool
DeinterlacePlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments &args,
                                         OfxRectD &/*rod*/)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    return false;
}

bool
DeinterlacePlugin::isIdentity(const OFX::IsIdentityArguments &args,
                              OFX::Clip * &/*identityClip*/,
                              double &/*identityTime*/)
{
    if (!kSupportsRenderScale && (args.renderScale.x != 1. || args.renderScale.y != 1.)) {
        OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    return false;
}

/* override the time domain action, only for the general context */
bool
DeinterlacePlugin::getTimeDomain(OfxRangeD &range)
{
    // this should only be called in the general context, ever!
    if (getContext() == OFX::eContextGeneral) {
        bool doubleFramerate;
        _doubleFramerate->getValue(doubleFramerate);
        if (doubleFramerate) {
            // how many frames on the input clip
            OfxRangeD srcRange = srcClip_->getFrameRange();

            range.min = srcRange.min * 2;
            range.max = srcRange.max * 2;
        }
        return true;
    }

    return false;
}

mDeclarePluginFactory(DeinterlacePluginFactory, {}, {});

using namespace OFX;

void DeinterlacePluginFactory::describe(OFX::ImageEffectDescriptor &desc)
{
    // basic labels
    desc.setLabels(kPluginName, kPluginName, kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);

    // add supported context
    desc.addSupportedContext(eContextFilter);
    desc.addSupportedContext(eContextGeneral);

    // add supported pixel depths
    desc.addSupportedBitDepth(eBitDepthUByte);
    desc.addSupportedBitDepth(eBitDepthUShort);
    desc.addSupportedBitDepth(eBitDepthFloat);

    // set a few flags
    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setSupportsMultiResolution(kSupportsMultiResolution);
    desc.setSupportsTiles(kSupportsTiles);
    desc.setTemporalClipAccess(true);
    desc.setRenderTwiceAlways(false);
    desc.setSupportsMultipleClipPARs(false);
    desc.setRenderThreadSafety(kRenderThreadSafety);

}

void DeinterlacePluginFactory::describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/)
{
    // Source clip only in the filter context
    // create the mandated source clip
    ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->addSupportedComponent(ePixelComponentRGB);
    srcClip->addSupportedComponent(ePixelComponentAlpha);
    srcClip->setTemporalClipAccess(true);
    srcClip->setSupportsTiles(kSupportsTiles);
    srcClip->setIsMask(false);
    srcClip->setFieldExtraction(OFX::eFieldExtractBoth);

    // create the mandated output clip
    ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
    dstClip->addSupportedComponent(ePixelComponentRGBA);
    srcClip->addSupportedComponent(ePixelComponentRGB);
    dstClip->addSupportedComponent(ePixelComponentAlpha);
    dstClip->setSupportsTiles(kSupportsTiles);


    PageParamDescriptor *page = desc.definePageParam("Controls");

    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamMode);
        param->setLabels(kParamModeLabel, kParamModeLabel, kParamModeLabel);
        param->setHint(kParamModeHint);
        assert(param->getNOptions() == eDeinterlaceModeWeave);
        param->appendOption(kParamModeOptionWeave, kParamModeOptionWeaveHint);
        assert(param->getNOptions() == eDeinterlaceModeBlend);
        param->appendOption(kParamModeOptionBlend, kParamModeOptionBlendHint);
        assert(param->getNOptions() == eDeinterlaceModeBob);
        param->appendOption(kParamModeOptionBob, kParamModeOptionBobHint);
        assert(param->getNOptions() == eDeinterlaceModeDiscard);
        param->appendOption(kParamModeOptionDiscard, kParamModeOptionDiscardHint);
        assert(param->getNOptions() == eDeinterlaceModeLinear);
        param->appendOption(kParamModeOptionLinear, kParamModeOptionLinearHint);
        assert(param->getNOptions() == eDeinterlaceModeMean);
        param->appendOption(kParamModeOptionMean, kParamModeOptionMeanHint);
        assert(param->getNOptions() == eDeinterlaceModeYadif);
        param->appendOption(kParamModeOptionYadif, kParamModeOptionYadifHint);
        param->setDefault(int(eDeinterlaceModeYadif));
        param->setAnimates(true); // can animate
        param->setIsSecret(true); // Not yet implemented!
        page->addChild(*param);
    }
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamFieldOrder);
        param->setLabels(kParamFieldOrderLabel, kParamFieldOrderLabel, kParamFieldOrderLabel);
        param->setHint(kParamFieldOrderHint);
        assert(param->getNOptions() == eFieldOrderLower);
        param->appendOption(kParamFieldOrderOptionLower, kParamFieldOrderOptionLowerHint);
        assert(param->getNOptions() == eFieldOrderUpper);
        param->appendOption(kParamFieldOrderOptionUpper, kParamFieldOrderOptionUpperHint);
        assert(param->getNOptions() == eFieldOrderAuto);
        param->appendOption(kParamFieldOrderOptionAuto, kParamFieldOrderOptionAutoHint);
        param->setDefault(int(eFieldOrderAuto));
        param->setAnimates(true); // can animate
        page->addChild(*param);
    }

    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamYadifMode);
        param->setLabels(kParamYadifModeLabel, kParamYadifModeLabel, kParamYadifModeLabel);
        param->setHint(kParamYadifModeHint);
        assert(param->getNOptions() == eYadifModeTemporalSpatial);
        param->appendOption(kParamYadifModeOptionTemporalSpatial, kParamYadifModeOptionTemporalSpatialHint);
        assert(param->getNOptions() == eYadifModeTemporal);
        param->appendOption(kParamYadifModeOptionTemporal, kParamYadifModeOptionTemporalHint);
        param->setDefault(eYadifModeTemporalSpatial);
        param->setAnimates(true); // can animate
        page->addChild(*param);
    }

    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(kParamW3FFilter);
        param->setLabels(kParamW3FFilterLabel, kParamW3FFilterLabel, kParamW3FFilterLabel);
        param->setHint(kParamW3FFilterHint);
        assert(param->getNOptions() == eW3FFilterSimple);
        param->appendOption(kParamW3FFilterOptionSimple, kParamW3FFilterOptionSimpleHint);
        assert(param->getNOptions() == eW3FFilterComplex);
        param->appendOption(kParamW3FFilterOptionComplex, kParamW3FFilterOptionComplexHint);
        param->setDefault(eYadifModeTemporalSpatial);
        param->setAnimates(true); // can animate
        param->setIsSecret(true); // Not yet implemented!
        page->addChild(*param);
    }

    {
        BooleanParamDescriptor *param = desc.defineBooleanParam(kParamDoubleFramerate);
        param->setLabels(kParamDoubleFramerateLabel, kParamDoubleFramerateLabel, kParamDoubleFramerateLabel);
        param->setHint(kParamDoubleFramerateHint);
        param->setAnimates(false);
    }
}

ImageEffect* DeinterlacePluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum /*context*/)
{
    return new DeinterlacePlugin(handle);
}

void getDeinterlacePluginID(OFX::PluginFactoryArray &ids) 
{
    static DeinterlacePluginFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
} 
