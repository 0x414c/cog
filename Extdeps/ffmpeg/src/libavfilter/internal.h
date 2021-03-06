/*
 * This file is part of FFmpeg.
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

#ifndef AVFILTER_INTERNAL_H
#define AVFILTER_INTERNAL_H

/**
 * @file
 * internal API functions
 */

#include "avfilter.h"
#include "avfiltergraph.h"
#include "formats.h"
#include "video.h"

#define POOL_SIZE 32
typedef struct AVFilterPool {
    AVFilterBufferRef *pic[POOL_SIZE];
    int count;
    int refcount;
    int draining;
} AVFilterPool;

typedef struct AVFilterCommand {
    double time;                ///< time expressed in seconds
    char *command;              ///< command
    char *arg;                  ///< optional argument for the command
    int flags;
    struct AVFilterCommand *next;
} AVFilterCommand;

/**
 * Update the position of a link in the age heap.
 */
void ff_avfilter_graph_update_heap(AVFilterGraph *graph, AVFilterLink *link);

#if !FF_API_AVFILTERPAD_PUBLIC
/**
 * A filter pad used for either input or output.
 */
struct AVFilterPad {
    /**
     * Pad name. The name is unique among inputs and among outputs, but an
     * input may have the same name as an output. This may be NULL if this
     * pad has no need to ever be referenced by name.
     */
    const char *name;

    /**
     * AVFilterPad type.
     */
    enum AVMediaType type;

    /**
     * Minimum required permissions on incoming buffers. Any buffer with
     * insufficient permissions will be automatically copied by the filter
     * system to a new buffer which provides the needed access permissions.
     *
     * Input pads only.
     */
    int min_perms;

    /**
     * Permissions which are not accepted on incoming buffers. Any buffer
     * which has any of these permissions set will be automatically copied
     * by the filter system to a new buffer which does not have those
     * permissions. This can be used to easily disallow buffers with
     * AV_PERM_REUSE.
     *
     * Input pads only.
     */
    int rej_perms;

    /**
     * Callback called before passing the first slice of a new frame. If
     * NULL, the filter layer will default to storing a reference to the
     * picture inside the link structure.
     *
     * Input video pads only.
     *
     * @return >= 0 on success, a negative AVERROR on error. picref will be
     * unreferenced by the caller in case of error.
     */
    void (*start_frame)(AVFilterLink *link, AVFilterBufferRef *picref);

    /**
     * Callback function to get a video buffer. If NULL, the filter system will
     * use avfilter_default_get_video_buffer().
     *
     * Input video pads only.
     */
    AVFilterBufferRef *(*get_video_buffer)(AVFilterLink *link, int perms, int w, int h);

    /**
     * Callback function to get an audio buffer. If NULL, the filter system will
     * use avfilter_default_get_audio_buffer().
     *
     * Input audio pads only.
     */
    AVFilterBufferRef *(*get_audio_buffer)(AVFilterLink *link, int perms,
                                           int nb_samples);

    /**
     * Callback called after the slices of a frame are completely sent. If
     * NULL, the filter layer will default to releasing the reference stored
     * in the link structure during start_frame().
     *
     * Input video pads only.
     *
     * @return >= 0 on success, a negative AVERROR on error.
     */
    int (*end_frame)(AVFilterLink *link);

    /**
     * Slice drawing callback. This is where a filter receives video data
     * and should do its processing.
     *
     * Input video pads only.
     *
     * @return >= 0 on success, a negative AVERROR on error.
     */
    int (*draw_slice)(AVFilterLink *link, int y, int height, int slice_dir);

    /**
     * Samples filtering callback. This is where a filter receives audio data
     * and should do its processing.
     *
     * Input audio pads only.
     *
     * @return >= 0 on success, a negative AVERROR on error. This function
     * must ensure that samplesref is properly unreferenced on error if it
     * hasn't been passed on to another filter.
     */
    int (*filter_samples)(AVFilterLink *link, AVFilterBufferRef *samplesref);

    /**
     * Frame poll callback. This returns the number of immediately available
     * samples. It should return a positive value if the next request_frame()
     * is guaranteed to return one frame (with no delay).
     *
     * Defaults to just calling the source poll_frame() method.
     *
     * Output pads only.
     */
    int (*poll_frame)(AVFilterLink *link);

    /**
     * Frame request callback. A call to this should result in at least one
     * frame being output over the given link. This should return zero on
     * success, and another value on error.
     *
     * Output pads only.
     */
    int (*request_frame)(AVFilterLink *link);

    /**
     * Link configuration callback.
     *
     * For output pads, this should set the link properties such as
     * width/height. This should NOT set the format property - that is
     * negotiated between filters by the filter system using the
     * query_formats() callback before this function is called.
     *
     * For input pads, this should check the properties of the link, and update
     * the filter's internal state as necessary.
     *
     * For both input and output filters, this should return zero on success,
     * and another value on error.
     */
    int (*config_props)(AVFilterLink *link);

    /**
     * The filter expects a fifo to be inserted on its input link,
     * typically because it has a delay.
     *
     * input pads only.
     */
    int needs_fifo;
};
#endif

/** default handler for freeing audio/video buffer when there are no references left */
void ff_avfilter_default_free_buffer(AVFilterBuffer *buf);

/** Tell is a format is contained in the provided list terminated by -1. */
int ff_fmt_is_in(int fmt, const int *fmts);

/**
 * Return a copy of a list of integers terminated by -1, or NULL in
 * case of copy failure.
 */
int *ff_copy_int_list(const int * const list);

/**
 * Return a copy of a list of 64-bit integers, or NULL in case of
 * copy failure.
 */
int64_t *ff_copy_int64_list(const int64_t * const list);

/* Functions to parse audio format arguments */

/**
 * Parse a pixel format.
 *
 * @param ret pixel format pointer to where the value should be written
 * @param arg string to parse
 * @param log_ctx log context
 * @return 0 in case of success, a negative AVERROR code on error
 */
int ff_parse_pixel_format(enum PixelFormat *ret, const char *arg, void *log_ctx);

/**
 * Parse a sample rate.
 *
 * @param ret unsigned integer pointer to where the value should be written
 * @param arg string to parse
 * @param log_ctx log context
 * @return 0 in case of success, a negative AVERROR code on error
 */
int ff_parse_sample_rate(int *ret, const char *arg, void *log_ctx);

/**
 * Parse a time base.
 *
 * @param ret unsigned AVRational pointer to where the value should be written
 * @param arg string to parse
 * @param log_ctx log context
 * @return 0 in case of success, a negative AVERROR code on error
 */
int ff_parse_time_base(AVRational *ret, const char *arg, void *log_ctx);

/**
 * Parse a sample format name or a corresponding integer representation.
 *
 * @param ret integer pointer to where the value should be written
 * @param arg string to parse
 * @param log_ctx log context
 * @return 0 in case of success, a negative AVERROR code on error
 */
int ff_parse_sample_format(int *ret, const char *arg, void *log_ctx);

/**
 * Parse a channel layout or a corresponding integer representation.
 *
 * @param ret 64bit integer pointer to where the value should be written.
 * @param arg string to parse
 * @param log_ctx log context
 * @return 0 in case of success, a negative AVERROR code on error
 */
int ff_parse_channel_layout(int64_t *ret, const char *arg, void *log_ctx);

void ff_update_link_current_pts(AVFilterLink *link, int64_t pts);

void ff_free_pool(AVFilterPool *pool);

void ff_command_queue_pop(AVFilterContext *filter);

/* misc trace functions */

/* #define FF_AVFILTER_TRACE */

#ifdef FF_AVFILTER_TRACE
#    define ff_tlog(pctx, ...) av_log(pctx, AV_LOG_DEBUG, __VA_ARGS__)
#else
#    define ff_tlog(pctx, ...) do { if (0) av_log(pctx, AV_LOG_DEBUG, __VA_ARGS__); } while (0)
#endif

#define FF_TPRINTF_START(ctx, func) ff_tlog(NULL, "%-16s: ", #func)

char *ff_get_ref_perms_string(char *buf, size_t buf_size, int perms);

void ff_tlog_ref(void *ctx, AVFilterBufferRef *ref, int end);

void ff_tlog_link(void *ctx, AVFilterLink *link, int end);

/**
 * Insert a new pad.
 *
 * @param idx Insertion point. Pad is inserted at the end if this point
 *            is beyond the end of the list of pads.
 * @param count Pointer to the number of pads in the list
 * @param padidx_off Offset within an AVFilterLink structure to the element
 *                   to increment when inserting a new pad causes link
 *                   numbering to change
 * @param pads Pointer to the pointer to the beginning of the list of pads
 * @param links Pointer to the pointer to the beginning of the list of links
 * @param newpad The new pad to add. A copy is made when adding.
 */
void ff_insert_pad(unsigned idx, unsigned *count, size_t padidx_off,
                   AVFilterPad **pads, AVFilterLink ***links,
                   AVFilterPad *newpad);

/** Insert a new input pad for the filter. */
static inline void ff_insert_inpad(AVFilterContext *f, unsigned index,
                                   AVFilterPad *p)
{
    ff_insert_pad(index, &f->nb_inputs, offsetof(AVFilterLink, dstpad),
                  &f->input_pads, &f->inputs, p);
#if FF_API_FOO_COUNT
    f->input_count = f->nb_inputs;
#endif
}

/** Insert a new output pad for the filter. */
static inline void ff_insert_outpad(AVFilterContext *f, unsigned index,
                                    AVFilterPad *p)
{
    ff_insert_pad(index, &f->nb_outputs, offsetof(AVFilterLink, srcpad),
                  &f->output_pads, &f->outputs, p);
#if FF_API_FOO_COUNT
    f->output_count = f->nb_outputs;
#endif
}

/**
 * Poll a frame from the filter chain.
 *
 * @param  link the input link
 * @return the number of immediately available frames, a negative
 * number in case of error
 */
int ff_poll_frame(AVFilterLink *link);

/**
 * Request an input frame from the filter at the other end of the link.
 *
 * @param link the input link
 * @return     zero on success
 */
int ff_request_frame(AVFilterLink *link);

#define AVFILTER_DEFINE_CLASS(fname)            \
    static const AVClass fname##_class = {      \
        .class_name = #fname,                   \
        .item_name  = av_default_item_name,     \
        .option     = fname##_options,          \
        .version    = LIBAVUTIL_VERSION_INT,    \
        .category   = AV_CLASS_CATEGORY_FILTER, \
    }

AVFilterBufferRef *ff_copy_buffer_ref(AVFilterLink *outlink,
                                      AVFilterBufferRef *ref);

/**
 * Find the index of a link.
 *
 * I.e. find i such that link == ctx->(in|out)puts[i]
 */
#define FF_INLINK_IDX(link)  ((int)((link)->dstpad - (link)->dst->input_pads))
#define FF_OUTLINK_IDX(link) ((int)((link)->srcpad - (link)->src->output_pads))

int ff_buffersink_read_compat(AVFilterContext *ctx, AVFilterBufferRef **buf);
int ff_buffersink_read_samples_compat(AVFilterContext *ctx, AVFilterBufferRef **pbuf,
                                      int nb_samples);

#endif /* AVFILTER_INTERNAL_H */
