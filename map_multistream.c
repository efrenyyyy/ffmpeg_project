#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/avutil.h>
#include <libavutil/fifo.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct StreamMap {
	int disabled;           /* 1 is this mapping is disabled by a negative map */
	int file_index;
	int stream_index;
	int sync_file_index;
	int sync_stream_index;
	char *linklabel;       /* name of an output link, for mapping lavfi outputs */
} StreamMap;

typedef struct InputFile {
	AVFormatContext *ctx;
	int eof_reached;      /* true if eof reached */
	int eagain;           /* true if last read attempt returned EAGAIN */
	int ist_index;        /* index of first stream in input_streams */
	int loop;             /* set number of times input stream should be looped */
	int64_t duration;     /* actual duration of the longest stream in a file
						  at the moment when looping happens */
	AVRational time_base; /* time base of the duration */
	int64_t input_ts_offset;

	int64_t ts_offset;
	int64_t last_ts;
	int64_t start_time;   /* user-specified start time in AV_TIME_BASE or AV_NOPTS_VALUE */
	int seek_timestamp;
	int64_t recording_time;
	int nb_streams;       /* number of stream that ffmpeg is aware of; may be different
						  from ctx.nb_streams if new streams appear during av_read_frame() */
	int nb_streams_warn;  /* number of streams that the user was warned of */
	int rate_emu;
	int accurate_seek;

#if HAVE_PTHREADS
	AVThreadMessageQueue *in_thread_queue;
	pthread_t thread;           /* thread reading from this file */
	int non_blocking;           /* reading packets from the thread should not block */
	int joined;                 /* the thread has been joined */
	int thread_queue_size;      /* maximum number of queued packets */
#endif
} InputFile;

typedef struct InputStream {
	int file_index;
	AVStream *st;
	int discard;             /* true if stream data should be discarded */
	int user_set_discard;
	int decoding_needed;     /* non zero if the packets must be decoded in 'raw_fifo', see DECODING_FOR_* */
#define DECODING_FOR_OST    1
#define DECODING_FOR_FILTER 2

	AVCodecContext *dec_ctx;
	AVCodec *dec;
	AVFrame *decoded_frame;
	AVFrame *filter_frame; /* a ref of decoded_frame, to be sent to filters */

	int64_t       start;     /* time when read started */
							 /* predicted dts of the next packet read for this stream or (when there are
							 * several frames in a packet) of the next frame in current packet (in AV_TIME_BASE units) */
	int64_t       next_dts;
	int64_t       dts;       ///< dts of the last packet read for this stream (in AV_TIME_BASE units)

	int64_t       next_pts;  ///< synthetic pts for the next decode frame (in AV_TIME_BASE units)
	int64_t       pts;       ///< current pts of the decoded frame  (in AV_TIME_BASE units)
	int           wrap_correction_done;

	int64_t filter_in_rescale_delta_last;

	int64_t min_pts; /* pts with the smallest value in a current stream */
	int64_t max_pts; /* pts with the higher value in a current stream */

					 // when forcing constant input framerate through -r,
					 // this contains the pts that will be given to the next decoded frame
	int64_t cfr_next_pts;

	int64_t nb_samples; /* number of samples in the last decoded audio frame before looping */

	double ts_scale;
	int saw_first_ts;
	AVDictionary *decoder_opts;
	AVRational framerate;               /* framerate forced with -r */
	int top_field_first;
	int guess_layout_max;

	int autorotate;

	int fix_sub_duration;
	struct { /* previous decoded subtitle and related variables */
		int got_output;
		int ret;
		AVSubtitle subtitle;
	} prev_sub;

	struct sub2video {
		int64_t last_pts;
		int64_t end_pts;
		//AVFifoBuffer *sub_queue;    ///< queue of AVSubtitle* before filter init
		AVFrame *frame;
		int w, h;
	} sub2video;

	int dr1;

	/* decoded data from this stream goes into all those filters
	* currently video and audio only */
	//InputFilter **filters;
	int        nb_filters;

	int reinit_filters;

	/* hwaccel options */
	enum HWAccelID hwaccel_id;
	char  *hwaccel_device;
	enum AVPixelFormat hwaccel_output_format;

	/* hwaccel context */
	enum HWAccelID active_hwaccel_id;
	void  *hwaccel_ctx;
	void(*hwaccel_uninit)(AVCodecContext *s);
	int(*hwaccel_get_buffer)(AVCodecContext *s, AVFrame *frame, int flags);
	int(*hwaccel_retrieve_data)(AVCodecContext *s, AVFrame *frame);
	enum AVPixelFormat hwaccel_pix_fmt;
	enum AVPixelFormat hwaccel_retrieved_pix_fmt;
	AVBufferRef *hw_frames_ctx;

	/* stats */
	// combined size of all the packets read
	uint64_t data_size;
	/* number of packets successfully read for this stream */
	uint64_t nb_packets;
	// number of frames/samples retrieved from the decoder
	uint64_t frames_decoded;
	uint64_t samples_decoded;

	int64_t *dts_buffer;
	int nb_dts_buffer;

	int got_output;
} InputStream;

typedef struct OutputStream {
	int file_index;          /* file index */
	int index;               /* stream index in the output file */
	int source_index;        /* InputStream index */
	AVStream *st;            /* stream in the output file */
	int encoding_needed;     /* true if encoding needed for this stream */
	int frame_number;
	/* input pts and corresponding output pts
	for A/V sync */
	struct InputStream *sync_ist; /* input stream to sync against */
	int64_t sync_opts;       /* output frame counter, could be changed to some true timestamp */ // FIXME look at frame_number
																								 /* pts of the first frame encoded for this stream, used for limiting
																								 * recording time */
	int64_t first_pts;
	/* dts of the last packet sent to the muxer */
	int64_t last_mux_dts;
	// the timebase of the packets sent to the muxer
	AVRational mux_timebase;
	AVRational enc_timebase;

	int                    nb_bitstream_filters;
	AVBSFContext            **bsf_ctx;

	AVCodecContext *enc_ctx;
	AVCodecParameters *ref_par; /* associated input codec parameters with encoders options applied */
	AVCodec *enc;
	int64_t max_frames;
	AVFrame *filtered_frame;
	AVFrame *last_frame;
	int last_dropped;
	int last_nb0_frames[3];

	void  *hwaccel_ctx;

	/* video only */
	AVRational frame_rate;
	int is_cfr;
	int force_fps;
	int top_field_first;
	int rotate_overridden;
	double rotate_override_value;

	AVRational frame_aspect_ratio;

	/* forced key frames */
	int64_t *forced_kf_pts;
	int forced_kf_count;
	int forced_kf_index;
	char *forced_keyframes;
	//AVExpr *forced_keyframes_pexpr;
	//double forced_keyframes_expr_const_values[FKF_NB];

	/* audio only */
	int *audio_channels_map;             /* list of the channels id to pick from the source stream */
	int audio_channels_mapped;           /* number of channels in audio_channels_map */

	char *logfile_prefix;
	FILE *logfile;

	//OutputFilter *filter;
	char *avfilter;
	char *filters;         ///< filtergraph associated to the -filter option
	char *filters_script;  ///< filtergraph script associated to the -filter_script option

	AVDictionary *encoder_opts;
	AVDictionary *sws_dict;
	AVDictionary *swr_opts;
	AVDictionary *resample_opts;
	char *apad;
	//OSTFinished finished;        /* no more packets should be written for this stream */
	int unavailable;                     /* true if the steram is unavailable (possibly temporarily) */
	int stream_copy;

	// init_output_stream() has been called for this stream
	// The encoder and the bitstream filters have been initialized and the stream
	// parameters are set in the AVStream.
	int initialized;

	int inputs_done;

	const char *attachment_filename;
	int copy_initial_nonkeyframes;
	int copy_prior_start;
	char *disposition;

	int keep_pix_fmt;

	AVCodecParserContext *parser;
	AVCodecContext       *parser_avctx;

	/* stats */
	// combined size of all the packets written
	uint64_t data_size;
	// number of packets send to the muxer
	uint64_t packets_written;
	// number of frames/samples sent to the encoder
	uint64_t frames_encoded;
	uint64_t samples_encoded;

	/* packet quality factor */
	int quality;

	int max_muxing_queue_size;

	/* the packets are buffered here until the muxer is ready to be initialized */
	AVFifoBuffer *muxing_queue;

	/* packet picture type */
	int pict_type;

	/* frame encode sum of squared error values */
	int64_t error[4];
} OutputStream;

typedef struct OutputFile {
	AVFormatContext *ctx;
	AVDictionary *opts;
	int ost_index;       /* index of the first stream in output_streams */
	int64_t recording_time;  ///< desired length of the resulting file in microseconds == AV_TIME_BASE units
	int64_t start_time;      ///< start time in microseconds == AV_TIME_BASE units
	uint64_t limit_filesize; /* filesize limit expressed in bytes */

	int shortest;

	int header_written;
} OutputFile;

OutputFile **output_files;
OutputStream **output_streams = NULL;
int		   nb_output_streams = 0;
InputFile **input_files;
InputStream **input_streams = NULL;
int        nb_input_streams = 0;
AVFormatContext *input_fmtctx;
AVFormatContext *output_fmtctx;
AVInputFormat *input_fmt;
AVOutputFormat *output_fmt;
StreamMap **streammaps;
int			nb_stream_maps = 0;

void dump_metadata(AVFormatContext *fmt_ctx)
{
	AVDictionaryEntry *m = NULL;
	while (m = av_dict_get(fmt_ctx->metadata, NULL, m, AV_DICT_IGNORE_SUFFIX))
	{
		printf("%s\t:%s\r\n", m->key, m->value);
	}
}

void add_input_stream(int *nb_input_stream, int nb_input_files, AVFormatContext *ic)
{
	int ;
	for (int i = 0; i < ic->nb_streams; i++)
	{
		AVStream *stream = ic->streams[i];
		InputStream *is = av_malloc(sizeof(InputStream));
		AVCodecParameters *par = stream->codecpar;

		input_streams[*nb_input_stream] = is;
		is->dec = avcodec_find_decoder(stream->codecpar->codec_id);
		is->dec_ctx = avcodec_alloc_context3(is->dec);
		is->st = stream;
		is->file_index = nb_input_files;
		int ret = avcodec_parameters_to_context(is->dec_ctx, par);
		if (ret < 0)
		{
			printf("avcodec parameters copy to context error %d\n", ret);
			return;
		}
		switch (par->codec_type)
		{
		case AVMEDIA_TYPE_AUDIO:
			is->guess_layout_max = INT_MAX;
			break;
		case AVMEDIA_TYPE_VIDEO:
			is->framerate = stream->avg_frame_rate;
			break;
		default:
			break;
		}
		*nb_input_stream++;
	}
}

void open_output_file(char *outputs, int nb_output_files)
{
	char *output_file;
	OutputFile *of;
	InputStream *is;
	OutputStream *os;
	AVFormatContext *output_fmt_ctx = NULL;
	AVOutputFormat *output_fmt = NULL;

	for (int i = 0; i < nb_output_files; i++)
	{
		output_file = outputs[i];
		of = av_malloc(sizeof(*of));
		if (!of)
		{
			printf("malloc of error\n");
			return;
		}
		output_files[i] = of;
		of->ctx = output_fmt_ctx;
		of->ost_index = nb_output_files;
		
		int ret = avformat_alloc_output_context2(&output_fmt_ctx, NULL, NULL, output_file);
		if (ret < 0)
		{
			printf("avformat alloc output context error %d\n", ret);
			return;
		}
		output_fmt = output_fmt_ctx->oformat;
		for (int j = 0; j < nb_stream_maps; j++)
		{
			StreamMap *sm = streammaps[j];
			int type;
			int idx = input_files[sm->file_index]->ist_index + sm->stream_index;
			switch (input_streams[idx]->st->codecpar->codec_type)
			{
			case AVMEDIA_TYPE_AUDIO:
				type = AVMEDIA_TYPE_AUDIO;
				break;
			case AVMEDIA_TYPE_VIDEO:
				type = AVMEDIA_TYPE_VIDEO;
				break;
			default:
				break;
			}
			os = av_malloc(sizeof(OutputStream));
			output_streams[j] = os;
			os->file_index = i;
			os->index = j;
			os->st = avformat_new_stream(output_fmt_ctx, NULL);
			os->st->codecpar = type;
			os->stream_copy = 1;
			os->encoding_needed = !os->stream_copy;
			os->enc_ctx = avcodec_alloc_context3(os->enc);
			if (!os->enc_ctx)
			{
				printf("avcodec alloc failed\n");
				return;
			}
			os->enc_ctx->codec_type = type;
			os->ref_par = avcodec_parameters_alloc();
			if (!os->ref_par) {
				av_log(NULL, AV_LOG_ERROR, "Error allocating the encoding parameters.\n");
				return;
			}
			os->max_muxing_queue_size = 128;
			os->max_muxing_queue_size *= sizeof(AVPacket);
			os->max_frames = INT_MAX;
			os->source_index = idx;
			if (idx >= 0)
			{
				os->sync_ist = input_streams[idx];
			}
			os->last_mux_dts = AV_NOPTS_VALUE;
			os->muxing_queue = av_fifo_alloc(8 * sizeof(AVPacket));
			if (!os->muxing_queue)
				return;
			if (avio_open2(output_fmt_ctx->pb, output_file, AVIO_FLAG_WRITE, NULL, NULL) < 0)
				return;
			av_dict_set(&output_fmt_ctx->metadata, "creation_time", NULL, 0);
			for (i = 0; i < nb_output_streams; i++)
			{
				AVStream *stream;
				stream = input_streams[i]->st;
				av_dict_copy(output_streams[i]->st->metadata, stream->metadata, AV_DICT_DONT_OVERWRITE);
			}
		}
	}
}

void open_input_file(char *inputs, int nb_input_files)
{
	char *input_file;
	InputFile *f;
	AVFormatContext *input_fmt_ctx = NULL;
	AVInputFormat *input_fmt = NULL;

	for (int i = 0; i < nb_input_files; i++)
	{
		input_file = inputs[i];
		input_fmt_ctx = avformat_alloc_context();
		if (!input_fmt_ctx)
		{
			printf("avformat alloc context error!\n");
			return;
		}
		input_fmt_ctx->flags |= AVFMT_FLAG_KEEP_SIDE_DATA;
		input_fmt_ctx->flags |= AVFMT_FLAG_NONBLOCK;

		int ret = avformat_open_input(&input_fmt_ctx, input_file, input_fmt, NULL);
		if (ret < 0)
		{
			printf("avformat open input error %d\n", ret);
			return;
		}
		ret = avformat_find_stream_info(input_fmt_ctx, NULL);
		if (ret < 0)
		{
			printf("avformat find stream info error %d\n", ret);
			return ;
		}
		add_input_stream(&nb_input_streams, i, input_fmt_ctx);

		av_dump_format(input_fmt_ctx, i, input_file, 0);

		f = av_malloc(sizeof(*f));
		if (!f)
		{
			printf("malloc InputFile struct error!");
			return;
		}
		input_files[i] = f;

		f->ctx = input_fmt_ctx;
		f->ist_index = nb_input_streams - input_fmt_ctx->nb_streams;
		f->nb_streams = input_fmt_ctx->nb_streams;
		f->start_time = AV_NOPTS_VALUE;
		f->duration = 0;
		f->time_base = (AVRational){ 1,1 };
		f->ts_offset = input_fmt_ctx->start_time == AV_NOPTS_VALUE ? 0 : input_fmt_ctx->start_time;
	}
}

OutputStream *choose_output()
{
	OutputStream *os_min = NULL;
	int64_t dts_min = INT_MAX;

	for (int i = 0; i < nb_output_streams; i++)
	{
		OutputStream *os = output_streams[i];
		int dts = os->st->cur_dts == AV_NOPTS_VALUE ? INT_MIN :
			av_rescale_q(os->st->cur_dts, os->st->time_base, AV_TIME_BASE_Q);

		if (os->st->cur_dts == AV_NOPTS_VALUE)
		{
			av_log(NULL, AV_LOG_DEBUG, "cur_dts is invalid (this is harmless if it occurs once at the start per stream)\n");
		}

		if (dts_min > dts)
		{
			dts_min = dts;
			os_min = os;
		}
	}
	return os_min;
}

void process_input_packet()
{
	OutputStream *output;
	InputStream *input;
	InputFile *infile;
	AVFormatContext *ctx;
	AVPacket pkt;
	int idx = 0;

	while (1)
	{
		output = choose_output();
		if (!output)
		{
			return;
		}
		idx = output->source_index;
		//input = input_streams[idx];
		infile = input_files[idx];
		av_read_frame(infile->ctx, &pkt);
		input = input_streams[pkt.stream_index + infile->ist_index];
		input->data_size += pkt.size;
		input->nb_packets++;

		/* add the stream-global side data to the first packet */
		if (input->nb_packets == 1)
		{
			for (int i = 0; i < input->st->nb_side_data; i++)
			{
				AVPacketSideData *sidedata = &input->st->side_data;
				uint8_t *dstdata;

				if (av_packet_get_side_data(&pkt, sidedata->type, sidedata->size))
				{
					continue;
				}

				dstdata = av_packet_new_side_data(&pkt, sidedata->type, sidedata->size);
				if (!dstdata)
				{
					return;
				}
				memcpy(dstdata, sidedata->data, sidedata->size);
			}
		}

		if (pkt.pts != AV_NOPTS_VALUE)
		{
			pkt.pts += av_rescale_q(infile->ts_offset, AV_TIME_BASE_Q, input->st->time_base);
		}
		if (pkt.dts != AV_NOPTS_VALUE)
		{
			pkt.dts += av_rescale_q(infile->ts_offset, AV_TIME_BASE_Q, input->st->time_base);
		}
		
	}
}

int main(int args, char **argv)
{
	char *srcfile[2], *dstfile;
	int output_nb_streams = 0, input_nb_files = 0;
	
	if (args < 3)
	{
		printf("please input <main input file> <second input file> <output file>");
		return 0;
	}

	for (int i = 0; i < args; i++)
	{
		StreamMap *sm = av_malloc(sizeof(StreamMap));
		sm->disabled = 0;
		sm->file_index = (i < 1 ? 0 : i - 1);
		sm->linklabel = 0;
		sm->stream_index = (i == 0 ? 0 : 1);
		sm->sync_file_index = sm->file_index;
		sm->sync_stream_index = i;
		streammaps[i] = sm;
		nb_stream_maps++;
	}

	for (int i = 1; i <= args - 1; i++)
	{
		srcfile[i] = argv[i];
		output_nb_streams ++;
		input_nb_files++;
	}
	//n audio stream + 1 video stream
	output_nb_streams++;
	dstfile = argv[args];

}
